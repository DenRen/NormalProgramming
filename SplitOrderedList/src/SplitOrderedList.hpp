#pragma once

#include <iterator>
#include <thread>
#include <atomic>

namespace lf
{

struct NodeBase
{
    std::atomic <NodeBase*> m_next;

    NodeBase(NodeBase* next)
        : m_next{next}
    {}
};

template <typename T>
class ForwardList
{
public:
    struct Node : public NodeBase
    {
        T m_val;

        Node(T val, NodeBase* next)
            : NodeBase{next}
            , m_val{std::move(val)}
        {}

        Node& Next() noexcept { return static_cast<Node&>(*this->m_next); };
        Node& Prev() noexcept { return static_cast<Node&>(*this->m_prev); };
    };

    ~ForwardList();

    class Iterator
    {
        NodeBase* m_node;

    public:
        Iterator(NodeBase* node)
            : m_node{node}
        {}

        T& operator*() { return static_cast<Node*>(m_node)->m_val; }
        T* operator->() noexcept { return &static_cast<Node*>(m_node)->m_val; }
        Iterator& operator++() { m_node = m_node->m_next; return *this; }
        Iterator operator++(int) { auto node = m_node; m_node = m_node->m_next; return node; }
        bool operator==(const Iterator& rhs) noexcept { return m_node == rhs.m_node; }
        bool operator!=(const Iterator& rhs) noexcept { return m_node != rhs.m_node; }

        NodeBase* base() const noexcept { return m_node; };
    };

    Iterator begin() noexcept
    {
        return m_head.m_next.load();
    }
    Iterator end() noexcept
    {
        return nullptr;
    }

    Iterator PushFront(T value)
    {
        return InsertBefore(std::move(value), m_head.m_next);
    }

    Iterator InsertAfter(Iterator pos, T value)
    {
        NodeBase* left = pos.base();
        return InsertBefore(std::move(value), left->m_next);
    }
    // void Erase(Node* pos);

private:
    Iterator InsertBefore(T value, std::atomic<NodeBase*>& next);

    NodeBase m_head{ nullptr };
};

template <typename T>
typename ForwardList<T>::Iterator ForwardList<T>::InsertBefore(T value, std::atomic<NodeBase*>& next)
{
    NodeBase* new_node = new Node{ std::move(value), nullptr };
    NodeBase* cur_next = nullptr;
    do {
        cur_next = next.load();
        new_node->m_next = cur_next;
    } while(!next.compare_exchange_strong(cur_next, new_node));
    return new_node;
}

template <typename T>
ForwardList<T>::~ForwardList()
{
    for(NodeBase* cur = m_head.m_next; cur != nullptr; )
    {
        NodeBase* save_next = cur->m_next;
        delete static_cast<Node*>(cur);
        cur = save_next;
    }
}

template <typename T>
class SortedList
{
public:
    struct Node
    {
        std::atomic<Node*> m_next;
        T m_val;

        Node(T val, Node* next)
            : m_next{next}
            , m_val{std::move(val)}
        {}

        Node& Next() noexcept { *this->m_next.load(); };
    };

    ~SortedList();

    class Iterator
    {
        Node* m_node;

    public:
        Iterator(Node* node)
            : m_node{node}
        {}

        T& operator*() { return m_node->m_val; }
        T* operator->() noexcept { return &m_node->m_val; }
        Iterator& operator++() { m_node = m_node->m_next; return *this; }
        Iterator operator++(int) { auto node = m_node; m_node = m_node->m_next; return node; }
        bool operator==(const Iterator& rhs) noexcept { return m_node == rhs.m_node; }
        bool operator!=(const Iterator& rhs) noexcept { return m_node != rhs.m_node; }

        Node* base() const noexcept { return m_node; };
    };

    Iterator begin() noexcept
    {
        return m_head.load();
    }
    Iterator end() noexcept
    {
        return nullptr;
    }

    Iterator Insert(T value);
    Iterator Erase(T value);

private:
    std::atomic<Node*> m_head{ nullptr };
}; // class SortedList

template <typename T>
SortedList<T>::~SortedList()
{
    for(Node* cur = m_head.load(); cur != nullptr; )
    {
        Node* save_next = cur->m_next;
        delete cur;
        cur = save_next;
    }
}

template <typename T>
typename SortedList<T>::Iterator SortedList<T>::Insert(T value)
{
    Node* new_node = new Node{ std::move(value), nullptr };

    bool changed = false;
    do {
        Node* head = m_head.load();
        if (head == nullptr)
        {
            Node* expected = nullptr;
            changed = m_head.compare_exchange_strong(expected, new_node);
        }
        else
        {
            // Find position
            auto it = Iterator{head}, end_it = end();
            auto prev_it = it;
            do
            {
                while(it != end_it && *it < new_node->m_val)
                    prev_it = it++;

                if (it != nullptr && *it == new_node->m_val)
                {
                    delete new_node;
                    return it;
                }
            } while (it != end_it);

            auto* cur_next = it.base();
            new_node->m_next = cur_next;
            changed = prev_it.base()->m_next.compare_exchange_strong(cur_next, new_node);
        }
    } while(!changed);

    return new_node;
}

template <typename T>
void MarkPointer(std::atomic<T*>& ptr) noexcept
{
    ptr.compare_exchange_strong();
}

template <typename T>
typename SortedList<T>::Iterator SortedList<T>::Erase(T value)
{
    Node* prev = nullptr;
    Node* cur = nullptr;
    Node* next = nullptr;

    bool erased = false;
    do {
        Node* head = m_head.load();
        if (head == nullptr)
            return nullptr;
        
        prev = cur = head;
        do {
            if (cur->m_val != value)
            {
                prev = cur;
                cur = cur->m_next.load();
            }
            else
            {
                break;
            }
        } while(cur != nullptr);

        if (cur == nullptr)
            return nullptr;

        // Node* expected = next;
        // if (!cur->m_next.compare_exchange_strong(expected, (Node*)((unsigned long long) next | 1ull)))
        // {
        //     continue;
        // }

        auto* expected = cur;
        erased = prev->m_next.compare_exchange_strong(expected, cur->m_next);
    } while (!erased);
    delete cur;

    return next;
}

} // namespace lf
