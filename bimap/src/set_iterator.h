#pragma once

#include "utils.h"

#include <iterator>

namespace tools {
template <typename T, typename V, typename Tag>
class set_iterator {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = std::conditional_t<std::is_same_v<Tag, left_tag>, T, V>;
  using reference = const value_type&;
  using pointer = const value_type*;
  using difference_type = std::ptrdiff_t;

private:
  using node_t = node<value_type>;
  using tagged_node_t = tagged_node<value_type, Tag>;
  using binode_t = binode<T, V>;
  using reverse_tag = std::conditional_t<std::is_same_v<Tag, left_tag>, right_tag, left_tag>;
  using flip_iterator_t = set_iterator<T, V, reverse_tag>;

  friend class set_iterator<T, V, reverse_tag>;
  template <typename L, typename R, typename CL, typename CR>
  friend class ::bimap;

  set_iterator(const flip_iterator_t& other) noexcept : _node(other._node) {}

public:
  set_iterator() : _node(nullptr) {}

  set_iterator(node_base* node) noexcept : _node(node) {}

  const value_type& operator*() const {
    return *operator->();
  }

  const value_type* operator->() const {
    return &static_cast<node_t*>(_node)->val;
  }

  set_iterator& operator++() {
    node_base* act = _node;
    if (act->right != nullptr) {
      act = act->right;
      while (act->left != nullptr) {
        act = act->left;
      }
      _node = act;
      return *this;
    }
    while (act->parent->right == act) {
      act = act->parent;
    }
    _node = act->parent;
    return *this;
  }

  set_iterator operator++(int) {
    set_iterator tmp = *this;
    ++*this;
    return tmp;
  }

  set_iterator& operator--() {
    node_base* act = _node;
    if (act->left != nullptr) {
      act = act->left;
      while (act->right != nullptr) {
        act = act->right;
      }
      _node = act;
      return *this;
    }
    while (act->parent->left == act) {
      act = act->parent;
    }
    _node = act->parent;
    return *this;
  }

  set_iterator operator--(int) {
    set_iterator tmp = *this;
    --*this;
    return tmp;
  }

  template <typename Tag_>
  using rev_tag_t = std::conditional_t<std::is_same_v<Tag_, left_tag>, right_tag, left_tag>;

  template <typename Tag_>
  using tag_value_t = std::conditional_t<std::is_same_v<Tag_, left_tag>, T, V>;

  flip_iterator_t flip() const noexcept {
    if (_node->parent->parent == _node) {
      return flip_iterator_t(_node->parent);
    }
    auto tmp = static_cast<binode_t*>(static_cast<tagged_node<tag_value_t<Tag>, Tag>*>(_node))
                   ->template get_node<rev_tag_t<Tag>>();
    return flip_iterator_t(tmp);
  }

  friend bool operator==(const set_iterator& lhs, const set_iterator& rhs) noexcept {
    return lhs._node == rhs._node;
  }

  friend bool operator!=(const set_iterator& lhs, const set_iterator& rhs) noexcept {
    return !(lhs == rhs);
  }

  binode_t* get_binode() noexcept {
    return static_cast<binode_t*>(static_cast<tagged_node_t*>(_node));
  }

private:
  node_base* _node;
};
} // namespace tools
