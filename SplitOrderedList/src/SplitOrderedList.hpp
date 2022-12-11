#pragma once

#include <iterator>
#include <thread>
#include <atomic>

#include "MarkedPointers.hpp"

namespace lf
{

constexpr std::size_t g_num_hazard_pointers_per_thread = 5;
constexpr std::size_t g_num_threads_max = 16;

struct LocalHazardPointers
{
    std::atomic<std::thread::id> m_id;
    std::array<std::atomic<void*>, g_num_hazard_pointers_per_thread> m_pointers;
};

std::array<LocalHazardPointers, g_num_threads_max> g_hp_storage;

class HazardPointerOwner
{
public:
    HazardPointerOwner()
    {
        const auto self_thread_id = std::this_thread::get_id();
        for (auto& hp : g_hp_storage)
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

    std::atomic<void*>& get() const
    {
        for (auto& ptr : m_hp->m_pointers)
            if (ptr.load() == nullptr)
                return ptr;

        throw std::runtime_error("Local hazard pointers are over!");
    }

    ~HazardPointerOwner()
    {
        for (auto& ptr : m_hp->m_pointers)
            ptr.store(nullptr);

        m_hp->m_id.store(std::thread::id{});
    }

private:
    LocalHazardPointers* m_hp = nullptr;
};

std::atomic<void*>& GetHazardPointer()
{
    thread_local static HazardPointerOwner hp;
    return hp.get();
}

bool is_hp_used_other_threads(void* finded_ptr)
{
    for(const auto&[thread_id, ptrs] : g_hp_storage)
        for (auto& ptr : ptrs)
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
class SortedList
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

public:
    ~SortedList();

    Node* Insert(const T& value);
    Node* Erase(const T& value);
    Node* Find(const T& value) const;

private:
    std::atomic<Node*> m_head{ nullptr };
};

template <typename T>
SortedList<T>::~SortedList()
{
    Node* curr = m_head;
    while(curr != nullptr)
    {
        Node* next = curr->m_next.load();   // todo
        delete curr;
        curr = next;
    }
}

template <typename T>
typename SortedList<T>::Node* SortedList<T>::Insert(const T& value)
{
    Node* new_node = new Node{nullptr, value};

    while (true)
    {
        // Set head to hazard pointer
        Node* head = m_head.load();
        std::atomic<void*>& hp = GetHazardPointer();
        Node* save_head = nullptr;
        do {
            save_head = head;
            hp.store(head);
            head = m_head.load();
        } while (save_head != head);

        if (head == nullptr)
        {
            Node* expected = nullptr;
            if (m_head.compare_exchange_strong(expected, new_node))
            {
                hp.store(nullptr);
                break;
            }
            else
                continue;
        }

        if (head->m_value == value)
            return head;

        Node* prev = head;
        Node* curr = UnmarkPointer(head->m_next.load());

        
        while (curr != nullptr)
        {
            if (curr->m_value == value)
            {
                delete new_node;
                return curr;
            }

            prev = curr;
            curr = UnmarkPointer(curr->m_next.load());
        }

        new_node->m_next.store(curr);
        if (prev->m_next.compare_exchange_strong(curr, new_node))
            break;
    }

    return new_node;
}

template <typename T>
typename SortedList<T>::Node* SortedList<T>::Find(const T& value) const
{
    Node* head = m_head.load();
    if (head == nullptr)
        return nullptr;

    Node* curr = head;
    while (curr != nullptr)
    {
        if (curr->m_value == value)
            return curr;
        curr = UnmarkPointer(curr->m_next.load());
    }

    return nullptr;
}

template <typename T>
typename SortedList<T>::Node* SortedList<T>::Erase(const T& value)
{
    while (1)
    {
        Node* head = m_head.load();
        if (head == nullptr)
            return nullptr;

        if (head->m_value == value)
        {
            Node* next = UnmarkPointer(head->m_next.load());
            Node* next_marked = MarkPointer(next);
            if (!head->m_next.compare_exchange_strong(next, next_marked))
                continue;

            if (!m_head.compare_exchange_strong(head, next))
            {
                head->m_next.store(next);
                continue;
            }

            // delete head;    // Change to HazardPtr or RCU

            return next;
        }

        Node* prev = head;
        Node* curr = UnmarkPointer(head->m_next.load());
        while (curr != nullptr)
        {
            if (curr->m_value == value)
                break;

            prev = curr;
            curr = UnmarkPointer(curr->m_next.load());
        }

        if (curr == nullptr)
            return nullptr;

        Node* next = nullptr;
        while (true)
        {
            next = UnmarkPointer(curr->m_next.load());
            Node* next_marked = MarkPointer(next);
            if (curr->m_next.compare_exchange_strong(next, next_marked))
                break;
        }
        
        if (!prev->m_next.compare_exchange_strong(curr, next))
        {
            curr->m_next.store(next);
            continue;
        }

        // delete curr;    // See above

        return next;
    }

    return nullptr;
}

} // namespace lf
