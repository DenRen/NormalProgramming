#include <atomic>
#include <memory>
#include <thread>
#include <array>
#include <functional>
#include <iostream>

struct HazardPointer
{
    std::atomic<std::thread::id> m_id;
    std::atomic<void*> m_pointer;
};

constexpr std::size_t s_num_hazard_pointers = 20;
std::array<HazardPointer, s_num_hazard_pointers> s_hp_storage;

class HazardPointerOwner
{
public:
    HazardPointerOwner()
    {
        const auto self_thread_id = std::this_thread::get_id();
        for (auto& hp : s_hp_storage)
        {
            std::thread::id expected;
            if (hp.m_id.compare_exchange_strong(expected, self_thread_id))
            {
                m_hp = &hp;
                return;
            }
        }

        throw std::runtime_error("Hazard pointers arrays is overflowed!");
    }

    std::atomic<void*>& get() const noexcept
    {
        return m_hp->m_pointer;
    }

    ~HazardPointerOwner()
    {
        m_hp->m_pointer.store(nullptr);
        m_hp->m_id.store(std::thread::id{});
    }

private:
    HazardPointer* m_hp = nullptr;
};

std::atomic<void*>& GetHazardPointer()
{
    thread_local static HazardPointerOwner hp;
    return hp.get();
}

bool is_hp_used_other_threads(void* finded_ptr)
{
    for(const auto&[thread_id, ptr] : s_hp_storage)
        if (ptr.load() == finded_ptr)
            return true;

    return false;
}

template <typename T>
void DataDeleter(void* data)
{
    delete static_cast<T*>(data);
}

struct DataToDelete
{
    template <typename T>
    DataToDelete(T* data)
        : m_data{ data }
        , m_deleter{ DataDeleter<T> }
    {}

    DataToDelete* m_next{ nullptr };
    void* m_data;
    std::function<void(void*)> m_deleter;
};

std::atomic<DataToDelete*> g_delete_list;

template <typename T>
void add_to_no_hazard_list(T* ptr)
{
    DataToDelete* data = new DataToDelete{ ptr };
    data->m_next = g_delete_list.load();
    while(!g_delete_list.compare_exchange_strong(data->m_next, data));
}

template <>
void add_to_no_hazard_list<DataToDelete>(DataToDelete* data)
{
    data->m_next = g_delete_list.load();
    while(!g_delete_list.compare_exchange_strong(data->m_next, data));
}

void delete_no_hazard_objects()
{
    DataToDelete* curr = g_delete_list.exchange(nullptr);
    while (curr)
    {
        DataToDelete* next = curr->m_next;
        if (is_hp_used_other_threads(curr->m_data))
            add_to_no_hazard_list(curr);
        else
            curr->m_deleter(curr->m_data);

        curr = next;
    }
}

template <typename T>
class LockFreeStack
{
public:
    struct Node
    {
        Node* m_next = nullptr;
        std::shared_ptr<T> m_data;

        Node(const T& data)
            : m_data(std::make_shared<T>(data))    // Correct according to the C++ standard
        {}
    };

    std::atomic<Node*> m_head{ nullptr };

public:
    void Push(const T& data)
    {
        Node* const new_node = new Node{ data };
        new_node->m_next = m_head.load();
        while (!m_head.compare_exchange_weak(new_node->m_next, new_node));
    }

    std::shared_ptr<T> Pop()
    {
        std::atomic<void*>& hp = GetHazardPointer();
        Node* old_head = m_head.load();

        do {
            Node* save_old_head = nullptr;
            do {
                save_old_head = old_head;
                hp.store(old_head);
                old_head = m_head.load();
                /*
                        Before the previous m_head.load() and current hp.store()
                    the head could have been removed (before hp.store() m_head
                    not protected). Because, after the set hp to head we nead to
                    check did the head still exist. If yes, the head node is
                    protected from delete;
                */
            } while(save_old_head != old_head);
        } while(old_head != nullptr &&
                !m_head.compare_exchange_weak(old_head, old_head->m_next));

        /*
                Even if other threads pop() and have a pointer to this
            node, they will not be able to get to this point, because
            (1) they protected it, therefore it cannot be physically removed,
            (2) the head has already been replaced by another and the last
                CAS won't work anymore.
        */

        hp.store(nullptr);

        std::shared_ptr<T> result;
        if (old_head != nullptr)
        {
            result.swap(old_head->m_data);
            if (is_hp_used_other_threads(old_head))
                add_to_no_hazard_list(old_head);
            else
                delete old_head;
        }
        delete_no_hazard_objects();

        return result;
    }
};

int main()
{
    std::size_t num_trheads = -1;
    std::cin >> num_trheads;

    LockFreeStack<int> stack;

    std::vector<std::thread> threads;
    for (std::size_t i = 0; i < num_trheads; ++i)
    {
        threads.emplace_back([&stack, i]() {
            for (int j = 0; j < 5000; ++j)
            {
                stack.Push(10 * j + i);
                std::this_thread::yield();
                stack.Push(10 * j + i);
            }
        });
    }

    for (auto& th : threads)
        th.join();

    for(auto val = stack.Pop(); val.get() != nullptr; val = stack.Pop())
    {
        // std::cout << *val << "\n";
    }
    std::cout << std::endl;
}