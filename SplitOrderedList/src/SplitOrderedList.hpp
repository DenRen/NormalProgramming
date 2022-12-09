#pragma once

#include <iterator>
#include <thread>
#include <atomic>

namespace lf
{

template <typename T>
T* MarkPointer(T* ptr) noexcept
{
    return reinterpret_cast<T*>(
               reinterpret_cast<unsigned long long>(ptr) | 1ull
           );
}

template <typename T>
T* UnmarkPointer(T* ptr) noexcept
{
    return reinterpret_cast<T*>(
               reinterpret_cast<unsigned long long>(ptr) & ~1ull
           );
}

template <typename T>
bool IsMarkedPointer(T* ptr) noexcept
{
    return !!reinterpret_cast<T*>(
               reinterpret_cast<unsigned long long>(ptr) & 1ull
           );
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

    Node* Insert(T value);
    Node* Erase(T value);
    Node* Find(T value) const;

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
typename SortedList<T>::Node* SortedList<T>::Insert(T value)
{
    Node* new_node = new Node{nullptr, value};

    while (true)
    {
        Node* head = m_head.load();
        if (head == nullptr)
        {
            Node* expected = nullptr;
            if (m_head.compare_exchange_strong(expected, new_node))
                break;
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
typename SortedList<T>::Node* SortedList<T>::Find(T value) const
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
typename SortedList<T>::Node* SortedList<T>::Erase(T value)
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
