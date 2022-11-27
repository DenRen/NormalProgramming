#pragma once

#include <iterator>

/*
namespace lf
{

template <typename T>
class List
{
    struct Node
    {
        using value_type = T;
        using pointer = Node*;

        Node(value_type val, pointer prev, pointer next)
            : m_val{std::move(val)}
            , m_prev{prev}
            , m_next{next}
        {}

        value_type m_val;
        mutable pointer m_prev = nullptr;
        mutable pointer m_next = nullptr;
    };

    template <typename Iterator>
    class IteratorBase
    {
        using IteratorValue = typename std::iterator_traits<Iterator>::value_type;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = typename IteratorValue::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        IteratorBase(Iterator node_ptr) : m_node{node_ptr} {}

        template <typename Iter>
        IteratorBase(const IteratorBase<Iter> rhs) : m_node{rhs.base()} {}

        reference operator*() const noexcept { return m_node->m_val; }
        pointer operator->() const noexcept { return &m_node->m_val; }
        bool operator!=(IteratorBase rhs) const noexcept { return m_node != rhs.m_node; };
        IteratorBase& operator++() noexcept
        {
            m_node = m_node->m_next;
            return *this;
        }
        Iterator base() const noexcept { return m_node; }

        bool operator==(const IteratorBase& rhs)
        {
            // return (m_node == rhs.m_node) || (m_node);
        }
    private:
        Iterator m_node;
    };

public:
    using Iterator = IteratorBase<Node*>;
    using ConstIterator = IteratorBase<const Node*>;

    Iterator begin() noexcept { return m_head; }
    Iterator end() noexcept { return nullptr; }

    ConstIterator cbegin() const noexcept { return m_head; }
    ConstIterator cend() const noexcept { return nullptr; }

    Iterator Insert(ConstIterator pos, T value);
    void Erase(ConstIterator pos);

private:
    Node* m_head = nullptr;
};

template <typename T>
typename List<T>::Iterator List<T>::Insert(ConstIterator pos, T value)
{
    auto& pos_next = pos.base()->m_next;
    auto new_node = new Node{std::move(value), const_cast<Node*>(pos.base()), pos_next};
    
    if (pos_next)
        pos_next->m_prev = new_node;

    pos_next = new_node;
    return new_node;
}

// Just for train
template <typename T>
class List
{
    struct NodeBase
    {
        NodeBase* m_next = nullptr;
        NodeBase* m_prev = nullptr;

        NodeBase(NodeBase* next, NodeBase* prev)
            : m_next{next}
            , m_prev{prev}
        {}
    };

public:
    struct Node : public NodeBase
    {
        T m_val;

        Node(T val, NodeBase* next, NodeBase* prev)
            : NodeBase{next, prev}
            , m_val{std::move(val)}
        {}

        Node& Next() noexcept { return static_cast<Node&>(*this->m_next); };
        Node& Prev() noexcept { return static_cast<Node&>(*this->m_prev); };
    };

    List() noexcept
        : m_head{&m_head, &m_head}
    {}

    ~List();

    class Iterator
    {
        NodeBase* m_node;

    public:
        Iterator(NodeBase* node)
            : m_node{node}
        {}

        Node& operator*() { return *static_cast<Node*>(m_node); }
        Node* operator->() noexcept { return static_cast<Node*>(m_node); }
        Iterator& operator++() { m_node = m_node->m_next; return *this; }
        Iterator& operator--() { m_node = m_node->m_prev; return *this; }
        Iterator operator++(int) { auto node = m_node; m_node = m_node->m_next; return node; }
        Iterator operator--(int) { auto node = m_node; m_node = m_node->m_prev; return node; }
        bool operator==(const Iterator& rhs) noexcept { return m_node == rhs.m_node; }
        bool operator!=(const Iterator& rhs) noexcept { return m_node != rhs.m_node; }
    };

    Iterator begin() noexcept
    {
        return static_cast<Node*>(m_head.m_next);
    }
    Iterator end() noexcept
    {
        return static_cast<Node*>(&m_head);
    }

    Iterator InsertPrev(Iterator pos, T value);
    // void Erase(Node* pos);

private:
    NodeBase m_head;
};

template <typename T>
typename List<T>::Iterator List<T>::InsertPrev(Iterator pos, T value)
{
    NodeBase* new_node = new Node{std::move(value), &*pos, pos->m_prev};
    pos->m_prev->m_next = new_node;
    pos->m_prev = new_node;
    return static_cast<Node*>(new_node);
}

template <typename T>
List<T>::~List()
{
    auto it = begin();
    const auto end_it = end();
    while(it != end_it)
        delete &*it++;
}

template <typename T>
class ForwardListNext
{
    struct NodeBase
    {
        NodeBase* m_next = nullptr;

        NodeBase(NodeBase* next)
            : m_next{next}
        {}
    };

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

    ForwardListNext() noexcept
        : m_head{nullptr}
        , m_tail{nullptr}
    {}

    ~ForwardListNext();

    class Iterator
    {
        NodeBase* m_node;

    public:
        Iterator(NodeBase* node)
            : m_node{node}
        {}

        T& operator*() { return static_cast<Node*>(m_node->m_next)->m_val; }
        T* operator->() noexcept { return &static_cast<Node*>(m_node->m_next)->m_val; }
        Iterator& operator++() { m_node = m_node->m_next; return *this; }
        Iterator operator++(int) { auto node = m_node; m_node = m_node->m_next; return node; }
        bool operator==(const Iterator& rhs) noexcept { return m_node->m_next == rhs.m_node->m_next; }
        bool operator!=(const Iterator& rhs) noexcept { return m_node->m_next != rhs.m_node->m_next; }

        NodeBase* base() const noexcept { return m_node; };
    };

    Iterator begin() noexcept
    {
        return &m_head;
    }
    Iterator end() noexcept
    {
        return &m_tail;
    }

    Iterator Insert(Iterator pos, T value);
    // void Erase(Node* pos);

private:
    NodeBase m_head, m_tail;
};

template <typename T>
typename ForwardListNext<T>::Iterator ForwardListNext<T>::Insert(Iterator pos, T value)
{
    NodeBase* left = pos.base();
    NodeBase* new_node = new Node{ std::move(value), left->m_next };
    left->m_next = new_node;
    return new_node;
}

template <typename T>
ForwardListNext<T>::~ForwardListNext()
{

    for(NodeBase* cur = m_head.m_next; cur != nullptr; )
    {
        NodeBase* save_next = cur->m_next;
        delete static_cast<Node*>(cur);
        cur = save_next;
    }
}

} // namespace lf
*/