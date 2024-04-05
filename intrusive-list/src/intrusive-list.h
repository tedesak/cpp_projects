#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace intrusive {

namespace tools {
struct list_element_base {
public:
  list_element_base() noexcept;
  list_element_base(const list_element_base&) noexcept;
  list_element_base& operator=(const list_element_base& other) noexcept;
  list_element_base(list_element_base&& other) noexcept;
  list_element_base& operator=(list_element_base&& other) noexcept;

  ~list_element_base() noexcept;

  void change_pointers(list_element_base& other);
  void unlink() noexcept;
  void clear_pointers() noexcept;
  list_element_base* get_prev() const;
  list_element_base* get_next() const;

  static void link(list_element_base* left, list_element_base* right) noexcept {
    left->next = right;
    right->prev = left;
  }

  static void paste(list_element_base* left, list_element_base* mid, list_element_base* right) noexcept {
    link(left, mid);
    link(mid, right);
  }

private:
  list_element_base* prev;
  list_element_base* next;
};
} // namespace tools

class default_tag;

template <typename Tag = default_tag>
class list_element : tools::list_element_base {
  template <typename T, typename ListTag>
  friend class list;
};

template <typename T, typename Tag = default_tag>
class list {
  static_assert(std::is_base_of_v<list_element<Tag>, T>, "T must derive from list_element");

private:
  using element = list_element<Tag>;

  template <typename V>
  struct list_iterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using reference = V&;
    using pointer = V*;
    using difference_type = std::ptrdiff_t;

  private:
    friend class list<T, Tag>;

    explicit list_iterator(tools::list_element_base* node) : _node(node) {}

  public:
    list_iterator() = default;

    list_iterator(V* node) = delete;

    list_iterator(const list_iterator& rhs) = default;

    ~list_iterator() = default;

    operator list_iterator<const V>() const {
      return list_iterator<const V>(_node);
    }

    V& operator*() const {
      return *operator->();
    }

    V* operator->() const {
      return static_cast<V*>(static_cast<element*>(_node));
    }

    list_iterator& operator++() {
      _node = _node->get_next();
      return *this;
    }

    list_iterator operator++(int) {
      _node = _node->get_next();
      return list_iterator(_node->get_prev());
    }

    list_iterator& operator--() {
      _node = _node->get_prev();
      return *this;
    }

    list_iterator operator--(int) {
      _node = _node->get_prev();
      return list_iterator(_node->get_next());
    }

    friend bool operator==(const list_iterator& lhs, const list_iterator& rhs) {
      return lhs._node == rhs._node;
    }

    friend bool operator!=(const list_iterator& lhs, const list_iterator& rhs) {
      return !(lhs == rhs);
    }

  private:
    tools::list_element_base* _node;
  };

public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = list_iterator<T>;
  using const_iterator = list_iterator<const T>;

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  static tools::list_element_base* as_list_element_base_ref(T* value) noexcept {
    return static_cast<tools::list_element_base*>(static_cast<element*>(value));
  }

public:
  // O(1)
  list() noexcept = default;

  list(const list&) = delete;

  list& operator=(const list&) = delete;

  // O(1)
  list(list&& other) noexcept = default;

  // O(1)
  list& operator=(list&& other) noexcept = default;

  // O(1)
  ~list() noexcept = default;

  // O(1)
  bool empty() const noexcept {
    return _root.get_next() == &_root;
  }

  // O(n)
  std::size_t size() const noexcept {
    return std::distance(begin(), end());
  }

  // O(1)
  T& front() noexcept {
    return *begin();
  }

  // O(1)
  const T& front() const noexcept {
    return *begin();
  }

  // O(1)
  T& back() noexcept {
    return *(--end());
  }

  // O(1)
  const T& back() const noexcept {
    return *(--end());
  }

  // O(1)
  void push_front(T& val) noexcept {
    insert(begin(), val);
  }

  // O(1)
  void push_back(T& val) noexcept {
    insert(end(), val);
  }

  // O(1)
  void pop_front() noexcept {
    erase(begin());
  }

  // O(1)
  void pop_back() noexcept {
    erase(--end());
  }

  // O(1)
  void clear() noexcept {
    _root.unlink();
  }

  // O(1)
  iterator begin() noexcept {
    return iterator(_root.get_next());
  }

  // O(1)
  const_iterator begin() const noexcept {
    return const_iterator(_root.get_next());
  }

  // O(1)
  iterator end() noexcept {
    return iterator(&_root);
  }

  // O(1)
  const_iterator end() const noexcept {
    return const_iterator(const_cast<tools::list_element_base*>(&_root));
  }

  // O(1)
  iterator insert(const_iterator pos, T& value) noexcept {
    tools::list_element_base* base_node = pos._node;
    tools::list_element_base* insert_el = as_list_element_base_ref(&value);
    if (base_node == insert_el) {
      return iterator(insert_el);
    }
    insert_el->unlink();
    tools::list_element_base::paste(base_node->get_prev(), insert_el, base_node);
    return iterator(insert_el);
  }

  // O(1)
  iterator erase(const_iterator pos) noexcept {
    tools::list_element_base* res = pos._node->get_next();
    pos._node->unlink();
    return iterator(res);
  }

  // O(1)
  void splice(const_iterator pos, list&, const_iterator first, const_iterator last) noexcept {
    if (first == last) {
      return;
    }
    auto first_node_prev = first._node->get_prev();
    tools::list_element_base::link(pos._node->get_prev(), first._node);
    tools::list_element_base::link(last._node->get_prev(), pos._node);
    tools::list_element_base::link(first_node_prev, last._node);
  }

private:
  tools::list_element_base _root;
};

} // namespace intrusive
