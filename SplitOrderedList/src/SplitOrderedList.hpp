#pragma once

#include <iterator>
#include <thread>
#include <atomic>
#include <array>
#include <assert.h>
#include <bit>

#include "MarkedPointers.hpp"

namespace lf
{

#define ENABLE_DIFF_CL_FOR_HP_OPTIMIZATION

template <typename T>
struct HPStorageUniqList
{
    constexpr static inline unsigned s_hp_per_thread = 2;
    using HPsBase = std::array<std::atomic<T*>, s_hp_per_thread>;
    #ifndef ENABLE_DIFF_CL_FOR_HP_OPTIMIZATION
        using HPs = HPsBase;
    #else
        struct alignas(64)
        HPs : public HPsBase
        {};
    #endif

    std::vector<std::atomic<std::thread::id>> m_thread_id;
    std::vector<HPs> m_hazard_pointers;

    HPStorageUniqList(unsigned num_threads)
        : m_thread_id(num_threads)
        , m_hazard_pointers(num_threads)
    {}

    ~HPStorageUniqList()
    {
        ReleaseOwnerless();
    }

    unsigned GetMaxNumThreads() const noexcept { return m_thread_id.size(); }

    template <typename Iter>
    void AddOwnerlessPointers(Iter begin, Iter end);

private:
    void ReleaseOwnerless();

    // Linked-list of objects, that connot be removed in HPLocalManagerUniqList destructor
    struct Node
    {
        Node* m_next = nullptr;
        T* m_data;

        Node(T* data) :
            m_data{ data }
        {}
    };

    std::atomic<Node*> m_ownersless;
};

template <typename T>
template <typename Iter>
void HPStorageUniqList<T>::AddOwnerlessPointers(Iter begin, Iter end)
{
    if (begin == end)
        return;

    Node* head = new Node{*begin++};

    Node* curr = head;
    while(begin != end)
    {
        curr = curr->m_next = new Node{*begin++};
    }
    Node* tail = curr;

    tail->m_next = m_ownersless.load();

    while(!m_ownersless.compare_exchange_strong(tail->m_next, head));
}

template <typename T>
void HPStorageUniqList<T>::ReleaseOwnerless()
{
    Node* curr = m_ownersless.load();
    while (curr != nullptr)
    {
        Node* next = curr->m_next;
        delete curr->m_data;
        delete curr;
        curr = next;
    }
}

template <typename T>
class HPLocalManagerUniqList
{
    HPStorageUniqList<T>& m_hp_storage;

    using HPs = typename HPStorageUniqList<T>::HPs;
    constexpr static inline unsigned s_hp_per_thread = HPStorageUniqList<T>::s_hp_per_thread;

    HPs& m_hps;
    std::vector<T*> m_retired_ptrs;
    unsigned m_num_retired_ptrs = 0;

    static HPs& FindAndTakeHPs(HPStorageUniqList<T>& hp_storage)
    {
        auto& ids = hp_storage.m_thread_id;
        std::thread::id empty_id, local_id = std::this_thread::get_id();

        const unsigned num_ids = ids.size();
        for (unsigned i = 0; i < num_ids; ++i)
        {
            auto& id = ids[i];
            if (id.load() == empty_id)
            {
                if (id.compare_exchange_strong(empty_id, local_id))
                    return hp_storage.m_hazard_pointers[i];

                empty_id = std::thread::id{};
            }
        }

        throw std::runtime_error("Failed to add new thread in HPStorage!");
    }

    constexpr static inline unsigned s_batch_coef = 2;
    static unsigned GetBatchSize(unsigned num_threads) noexcept
    {
        return s_batch_coef * s_hp_per_thread * num_threads;
    }

    void ReleaseRetired()
    {
        // Store all hazard pointers of other threads
        const unsigned max_num_threads = m_hp_storage.GetMaxNumThreads();
        unsigned num_protected_ptrs = 0;
        std::vector<T*> protected_ptrs((max_num_threads - 1) * s_hp_per_thread);
        for (const auto& hptrs : m_hp_storage.m_hazard_pointers)
            for (auto& hptr : hptrs)
                if (T* ptr = hptr.load(); ptr != nullptr)
                    protected_ptrs[num_protected_ptrs++] = ptr;

        // Sort for prepare for binary search
        protected_ptrs.resize(num_protected_ptrs);
        std::sort(protected_ptrs.begin(), protected_ptrs.end());

        // Release all unprotected pointers
        auto it_write = m_retired_ptrs.begin();
        auto it_end = std::next(it_write, m_num_retired_ptrs);
        for (auto it_read = m_retired_ptrs.cbegin(); it_read != it_end; ++it_read)
        {
            if (std::binary_search(protected_ptrs.cbegin(), protected_ptrs.cend(), *it_read))
                *it_write++ = *it_read;
            else
                delete *it_read;
        }

        m_num_retired_ptrs = std::distance(m_retired_ptrs.begin(), it_write);
    }

public:
    HPLocalManagerUniqList(HPStorageUniqList<T>& hp_storage)
        : m_hp_storage{ hp_storage }
        , m_hps{ FindAndTakeHPs( hp_storage ) }
        , m_retired_ptrs(GetBatchSize(hp_storage.GetMaxNumThreads()))
    {}

    ~HPLocalManagerUniqList()
    {
        for (auto& hp : m_hps)
            hp.store(nullptr);

        ReleaseRetired();

        auto it_retired_begin = m_retired_ptrs.begin();
        auto it_retired_end = std::next(it_retired_begin, m_num_retired_ptrs);
        m_hp_storage.AddOwnerlessPointers(it_retired_begin, it_retired_end);

        const std::thread::id local_id = std::this_thread::get_id();
        for (auto& id : m_hp_storage.m_thread_id)
            if (id.load() == local_id)
            {
                id.store(std::thread::id{});
                return;
            }
    }

    HPs& GetHPs() const noexcept { return m_hps; }

    void ResetHPs() noexcept
    {
        for (auto& hp : m_hps)
            hp.store(nullptr, std::memory_order_release);
    }

    // I use UnmarkedPointers when ptr.load() special to wait-free:
    // in this case we go to the next(for example in find),
    // but other case we can spinning on current erased node
    static T* Protect(std::atomic<T*>& ptr, std::atomic<T*>& hptr)
    {
        T* tmp = nullptr;
        T* save_ptr = UnmarkPointer(ptr.load(std::memory_order::relaxed));
        do {
            tmp = save_ptr;
            hptr.store(save_ptr);
            save_ptr = UnmarkPointer(ptr.load(std::memory_order::acquire));
        } while(save_ptr != tmp);

        return save_ptr;
    }

    void TryRelease(T* ptr)
    {
        m_retired_ptrs[m_num_retired_ptrs] = ptr;

        if (++m_num_retired_ptrs == m_retired_ptrs.size())
            ReleaseRetired();
    }
};

template <typename T>
class UniqList
{
    class Node
    {
    public:
        Node(Node* next, T value)
            : m_next{ next }
            , m_value{ value }
        {}

        std::atomic<Node*> m_next;
        T m_value;
    };

    HPStorageUniqList<Node> m_hp_storage;

    using HPManager = HPLocalManagerUniqList<Node>;
    HPManager& GetHPManager()
    {
        /*
            TODO:
                Сейчас, чтобы работать с этой структурой данных, нужно создавать
                исполнителя с временем жизни меньшим, чем эта структура данных,
                т.е. структуру данных, нужно создавать в родительском потоке.

                Это нужно исправить более продуманным проектированием.
        */

        thread_local static HPLocalManagerUniqList<Node> hp_mgr{ m_hp_storage };
        return hp_mgr;
    }

    std::atomic<Node*> m_head{ nullptr };

public:
    UniqList(unsigned max_num_threads)
        : m_hp_storage{ max_num_threads }
    {}

    ~UniqList()
    {
        Node* node = m_head.load();
        while (node != nullptr)
        {
            Node* tmp = node->m_next.load();
            delete node;
            node = tmp;
        }
    }

    Node* Insert(const T& value)
    {
        HPManager& hp_mgr = GetHPManager();
        auto& hps = hp_mgr.GetHPs();

        Node* new_node = new Node{nullptr, value};

        while (true)
        {
            // Set head to hazard pointer
            Node* head = HPManager::Protect(m_head, hps[0]);
            Node* prev = nullptr;   // Protected by hps[0]
            Node* curr = head;      // Protected by hps[1]
            while (curr != nullptr && curr->m_value < value)
            {
                // Go to next
                hps[0].store(curr);
                prev = curr;

                curr = HPManager::Protect(curr->m_next, hps[1]);
            }

            if (curr != nullptr && !(value < curr->m_value))
            {
                delete new_node;
                new_node = nullptr;
                break;
            }

            new_node->m_next = curr;
            auto& node = prev == nullptr ? m_head : prev->m_next;
            Node* expected = curr;
            if (node.compare_exchange_strong(expected, new_node,std::memory_order_release,
                                                                std::memory_order_relaxed))
                break;
        }

        hp_mgr.ResetHPs();

        return new_node;    // Only for sential nodes, they cannot be removed
    }

    bool Find(const T& value)
    {
        HPManager& hp_mgr = GetHPManager();
        auto& hps = hp_mgr.GetHPs();

        Node* head = HPManager::Protect(m_head, hps[0]);
        if (head == nullptr)
            return false;

        Node* curr = head;
        bool is_find = false;
        while (curr != nullptr)
        {
            if (curr->m_value == value)
            {
                is_find = true;
                break;
            }

            // TODO: Я не могу защщать HP маркерованный указатель, мне
            // нужно добавить в защиту демаркировку указателей [complete]
            curr = HPManager::Protect(curr->m_next, hps[1]);
            hps[0].store(curr, std::memory_order_release);
        }

        hp_mgr.ResetHPs();

        return is_find;
    }

    void Erase(const T& value)
    {
        HPManager& hp_mgr = GetHPManager();
        auto& hps = hp_mgr.GetHPs();

        Node* erased_node = nullptr;
        while (true)
        {
            // Set head to hazard pointer
            Node* head = HPManager::Protect(m_head, hps[0]);
            Node* prev = nullptr;   // Protected by hps[0]
            Node* curr = head;      // Protected by hps[1]
            while (curr != nullptr && curr->m_value < value)
            {
                // Go to next
                hps[0].store(curr);
                prev = curr;

                curr = HPManager::Protect(curr->m_next, hps[1]);
            }

            if (curr == nullptr || value < curr->m_value)
                break;

            // Mark next pointer that insert cannot insert new node between
            // curr->m_next and curr->m_next->m_next
            bool is_already_marked = false;
            Node* next = nullptr;
            do {
                next = UnmarkPointer(curr->m_next.load(std::memory_order_relaxed));
                Node* marked_next = MarkPointer(next);
                // (1) Try to LOGIC remove
                bool changed = curr->m_next.compare_exchange_strong(next, marked_next,
                                                                    std::memory_order_release,
                                                                    std::memory_order_relaxed);
                if (changed)
                    break;

                if (IsMarkedPointer(next))
                {
                    is_already_marked = true;
                    break;
                }
            } while (true);

            if (is_already_marked)
                continue;

            auto& node = prev == nullptr ? m_head : prev->m_next;
            // (2) Try from STRUCTURE remove
            Node* expected = curr;
            if (node.compare_exchange_strong(expected, next))
            {
                erased_node = curr;
                break;
            }

            curr->m_next.store(next);
        }

        hp_mgr.ResetHPs();

        // (3) Try to PHYSIC remove
        if (erased_node != nullptr)
            hp_mgr.TryRelease(erased_node);
    }

    void Dump()
    {
        for (Node* node = m_head.load(); node != nullptr; node = node->m_next)
        {
            std::cout << node->m_value << std::endl;
        }
    }
};

template <typename T>
class SplitOrderedList
{
    // Hash-table
    constexpr static inline unsigned s_log_size_table = 8;
    constexpr static inline unsigned s_size_table = 1u << s_log_size_table;
    using hash_t = uint32_t;

    static hash_t ReverseBitOrder(hash_t x) noexcept
    {
        // swap odd and even bits
        x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
        // swap consecutive pairs
        x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
        // swap nibbles ...
        x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
        // swap bytes
        x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
        // swap 2-byte long pairs
        return ( x >> 16 ) | ( x << 16 );
    }

    struct NodeBase
    {
        hash_t m_shah;
        std::atomic<NodeBase*> m_next;

        NodeBase(hash_t shah, NodeBase* next = nullptr)
            : m_shah{shah}
            , m_next{next}
        {}

        bool operator<(const NodeBase& rhs) const noexcept { return m_shah < rhs.m_shah; }
        bool IsReg() const noexcept { return m_shah & 1u; }

        ~NodeBase();
    };

    struct NodeSential : public NodeBase
    {};

    struct NodeReg : public NodeBase
    {
        T m_value;

        NodeReg(const T& value, NodeBase* next = nullptr)
            : NodeBase{ReverseBitOrder(std::hash<T>{}(value)) | 1u, next}
            , m_value{value}
        {}

        bool operator==(const NodeReg& rhs) const noexcept { return m_value == rhs.m_value; }
    };

    HPStorageUniqList<NodeBase> m_hp_storage;
    using HPManager = HPLocalManagerUniqList<NodeBase>;
    HPManager& GetHPManager()
    {
        thread_local static HPLocalManagerUniqList<NodeBase> hp_mgr{ m_hp_storage };
        return hp_mgr;
    }

    using Table = std::array<std::atomic<NodeSential*>, s_size_table>;

    std::atomic <unsigned> m_num_elems, m_num_buckets;
    Table m_table;

    static NodeReg* ToReg(NodeBase* base) noexcept { return static_cast<NodeReg*>(base); }

    NodeSential* InsertSential(NodeSential* node_sent, hash_t shah)
    {
        HPManager& hp_mgr = GetHPManager();
        auto& hps = hp_mgr.GetHPs();

        NodeSential* new_node = new NodeSential{shah};
        new_node->m_shah = shah;

        auto& head_line = node_sent->m_next;
        while (true)
        {
            // Set head to hazard pointer
            NodeBase* head = HPManager::Protect(head_line, hps[0]);
            NodeBase* prev = nullptr;   // Protected by hps[0]
            NodeBase* curr = head;      // Protected by hps[1]
            while (curr != nullptr && curr->m_shah < shah)
            {
                // Go to next
                hps[0].store(curr, std::memory_order_release);
                prev = curr;

                curr = HPManager::Protect(curr->m_next, hps[1]);
            }

            if (curr != nullptr && curr->m_shah == shah)
            {
                delete new_node;
                new_node = static_cast<NodeSential*>(curr);
                break;
            }

            new_node->m_next = curr;
            auto& node = prev == nullptr ? head_line : prev->m_next;
            NodeBase* expected = curr;
            if (node.compare_exchange_strong(expected, new_node))
                break;
        }

        hp_mgr.ResetHPs();

        return new_node;
    }


    void InsertReg(NodeSential* node_sent, const T& value)
    {
        // Эта функция не реаллоцирует размер

        HPManager& hp_mgr = GetHPManager();
        auto& hps = hp_mgr.GetHPs();

        NodeReg* new_node = new NodeReg{value};
        hash_t shah = new_node->m_shah | 1;

        auto& head_line = node_sent->m_next;
        while (true)
        {
            // Set head to hazard pointer
            NodeBase* head = HPManager::Protect(head_line, hps[0]);
            NodeBase* prev = nullptr;   // Protected by hps[0]
            NodeBase* curr = head;      // Protected by hps[1]
            while (curr != nullptr && curr->m_shah < shah)
            {
                // Go to next
                hps[0].store(curr, std::memory_order_release);
                prev = curr;

                curr = HPManager::Protect(curr->m_next, hps[1]);
            }

            if (curr != nullptr && curr->IsReg() && value == ToReg(curr)->m_value)
            {
                delete new_node;
                break;
            }

            new_node->m_next = curr;
            auto& node = prev == nullptr ? head_line : prev->m_next;
            NodeBase* expected = curr;
            if (node.compare_exchange_strong(expected, new_node))
            {
                m_num_elems.fetch_add(1, std::memory_order_relaxed);
                break;
            }
        }

        hp_mgr.ResetHPs();
    }

    static size_t ParentBucket(size_t nBucket) noexcept
    {
        assert( nBucket > 0 );
        return nBucket & ~( 1 << std::countr_zero ( nBucket ) );
    }

    NodeSential* GetNodeSential(hash_t hash)
    {
        hash_t table_index = hash & (m_num_buckets.load() - 1);
        NodeSential* node_sent = m_table[table_index].load();
        if (node_sent == nullptr)
        {
            NodeSential* parent_sent = table_index ? GetNodeSential(ParentBucket(table_index))
                                                   : m_table[0].load();
            NodeSential* new_sent = InsertSential(parent_sent, ReverseBitOrder(table_index));
            m_table[table_index].store(new_sent);
            node_sent = new_sent;
            assert(node_sent != nullptr);
        }

        return node_sent;
    }

    void CheckOnRehash()
    {
        auto num_elems = m_num_elems.load();
        auto num_buckets = m_num_buckets.load();
        if (num_elems >= 2 * num_buckets)
        {
            if (m_table.size() < 2 * num_buckets)
                return;

            m_num_buckets.compare_exchange_strong(num_buckets, 2 * num_buckets);
        }
    }

public:
    SplitOrderedList(unsigned max_num_threads)
        : m_hp_storage{max_num_threads}
    {
        (void) max_num_threads;

        for (auto& sent_ptr : m_table)
            sent_ptr.store(nullptr);

        NodeSential* init_reg_node = new NodeSential{0};
        m_table[0].store(init_reg_node);

        m_num_buckets.store(1+1);
        m_num_elems.store(0);
    }

    ~SplitOrderedList()
    {
        NodeBase* curr = m_table[0].load();
        while(curr != nullptr)
        {
            NodeBase* next = curr->m_next;

            if (curr->IsReg())
                delete ToReg(curr);
            else
                delete static_cast<NodeSential*>(curr);

            curr = next;
        }
    }

    unsigned GetSize() const noexcept { return m_num_elems.load(); }

    void Insert(const T& value)
    {
        CheckOnRehash();

        hash_t hash = std::hash<T>{}(value);
        hash_t shah = ReverseBitOrder(hash);
        NodeSential* node_sent = GetNodeSential(hash);

        InsertReg(node_sent, value);
    }

    bool Find(const T& value)
    {
        HPManager& hp_mgr = GetHPManager();
        auto& hps = hp_mgr.GetHPs();

        hash_t hash = std::hash<T>{}(value);
        hash_t shah = ReverseBitOrder(hash) | 1;
        NodeSential* node_sent = GetNodeSential(hash);

        auto& head_line = node_sent->m_next;

        NodeBase* head = HPManager::Protect(head_line, hps[0]);
        NodeBase* prev = nullptr;   // Protected by hps[0]
        NodeBase* curr = head;      // Protected by hps[1]
        while (curr != nullptr && curr->m_shah < shah)
        {
            // Go to next
            hps[0].store(curr, std::memory_order_release);
            prev = curr;

            curr = HPManager::Protect(curr->m_next, hps[1]);
        }

        bool res = false;
        while (curr != nullptr && curr->m_shah == shah)
        {
            if (ToReg(curr)->m_value == value)
            {
                res = true;
                break;
            }

            // Go to next
            hps[0].store(curr, std::memory_order_release);
            prev = curr;

            curr = HPManager::Protect(curr->m_next, hps[1]);
        }

        hp_mgr.ResetHPs();

        return res;
    }

    void Erase(const T& value)
    {
        HPManager& hp_mgr = GetHPManager();
        auto& hps = hp_mgr.GetHPs();

        hash_t hash = std::hash<T>{}(value);
        hash_t shah = ReverseBitOrder(hash) | 1;
        NodeSential* node_sent = GetNodeSential(hash);

        auto& head_line = node_sent->m_next;

        NodeBase* erased_node = nullptr;
        while (true)
        {
            // Set head to hazard pointer
            NodeBase* head = HPManager::Protect(head_line, hps[0]);
            NodeBase* prev = nullptr;   // Protected by hps[0]
            NodeBase* curr = head;      // Protected by hps[1]
            while (curr != nullptr && curr->m_shah < shah)
            {
                // Go to next
                hps[0].store(curr, std::memory_order_release);
                prev = curr;

                curr = HPManager::Protect(curr->m_next, hps[1]);
            }

            bool finded = false;
            while (curr != nullptr && curr->m_shah == shah)
            {
                if (ToReg(curr)->m_value == value)
                {
                    finded = true;
                    break;
                }

                // Go to next
                hps[0].store(curr, std::memory_order_release);
                prev = curr;

                curr = HPManager::Protect(curr->m_next, hps[1]);
            }

            if (!finded)
                break;

            // Mark next pointer that insert cannot insert new node between
            // curr->m_next and curr->m_next->m_next
            NodeBase* next = nullptr;
            bool is_already_marked = false;
            while (true) {
                next = UnmarkPointer(curr->m_next.load(std::memory_order_relaxed));
                NodeBase* marked_next = MarkPointer(next);
                // (1) Try to LOGIC remove
                bool changed = curr->m_next.compare_exchange_strong(next, marked_next,
                                                                    std::memory_order_release,
                                                                    std::memory_order_relaxed);
                if (changed)
                    break;

                if (IsMarkedPointer(next))
                {
                    is_already_marked = true;
                    break;
                }
            }
            if (is_already_marked)
                break;

            auto& node = prev == nullptr ? head_line : prev->m_next;
            // (2) Try from STRUCTURE remove
            NodeBase* expected = curr;
            if (node.compare_exchange_strong(expected, next))
            {
                m_num_elems.fetch_sub(1, std::memory_order_relaxed);
                erased_node = curr;
                break;
            }

            curr->m_next.store(next, std::memory_order_release);
        }

        hp_mgr.ResetHPs();

        // (3) Try to PHYSIC remove
        if (erased_node != nullptr)
            hp_mgr.TryRelease(erased_node);
    }

    void Dump() const
    {
        std::cout << "num buckets: " << m_num_buckets << std::endl;
        std::cout << "num elems: " << m_num_elems << std::endl;

        for (unsigned i = 0; i < m_num_buckets.load(); ++i)
        if (m_table[i].load())
            std::cout << '[' << i << ']' << std::hex << m_table[i].load()->m_shah << std::endl;
        else
            std::cout << '[' << i << ']' << "   ---   " << std::endl;

        NodeBase* curr = m_table[0].load();
        while(curr != nullptr)
        {
            if (curr->IsReg())
                std::cout << "Reg: " << std::hex << curr->m_shah << std::dec << ", value: "<< ToReg(curr)->m_value << std::endl;
            else
                std::cout << "Sen: " << std::hex  << curr->m_shah << std::dec << std::endl;

            curr = curr->m_next;
        }
        {
            NodeBase* prev = m_table[0].load();
            if (prev == nullptr)
                return;

            NodeBase* curr = prev->m_next;
            while (curr != nullptr)
            {
                if (curr->m_shah > prev->m_shah == false)
                {
                    std::cerr << "prev: " << std::hex << prev->m_shah << std::endl;
                    std::cerr << "curr: " << std::hex << curr->m_shah << std::endl;
                    throw std::runtime_error("Error shah ordering");
                }

                prev = curr;
                curr = curr->m_next;
            }
        }
    }
};

template <typename T>
SplitOrderedList<T>::NodeBase::~NodeBase()
{
    if (IsReg())
    {
        static_cast<NodeReg*>(this)->m_value.~T();
    }
}

} // namespace lf
