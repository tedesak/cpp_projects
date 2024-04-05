#pragma once

#include <utility>

template <typename L, typename R, typename CL, typename CR>
class bimap;

namespace tools {
class left_tag;
class right_tag;

struct node_base {
protected:
  void repair_child_links() noexcept {
    if (left != nullptr) {
      left->parent = this;
    }
    if (right != nullptr) {
      right->parent = this;
    }
  }

public:
  node_base() noexcept = default;

  node_base(const node_base& other) : parent(other.parent), left(other.left), right(other.right) {}

  node_base(node_base&& other) noexcept
      : parent(std::exchange(other.parent, nullptr)),
        left(std::exchange(other.left, nullptr)),
        right(std::exchange(other.right, nullptr)) {
    repair_child_links();
  }

  void copy_links(node_base* rhs) noexcept {
    parent = rhs->parent;
    left = rhs->left;
    right = rhs->right;
    repair_child_links();
    if (parent != nullptr) {
      if (parent->left == rhs) {
        parent->left = this;
      } else {
        parent->right = this;
      }
    }
  }

  void relink(node_base* parent_, node_base* left_, node_base* right_) noexcept {
    parent = parent_;
    left = left_;
    right = right_;
  }

  void left_link(node_base* child) noexcept {
    left = child;
    if (child != nullptr) {
      child->parent = this;
    }
  }

  void right_link(node_base* child) noexcept {
    right = child;
    if (child != nullptr) {
      child->parent = this;
    }
  }

  void swap(node_base& other) noexcept {
    std::swap(other.parent, parent);
    std::swap(other.left, left);
    std::swap(other.right, right);
    repair_child_links();
    other.repair_child_links();
  }

  node_base* parent{nullptr};
  node_base* left{nullptr};
  node_base* right{nullptr};
};

template <typename T>
struct node : node_base {
private:
  template <typename L, typename R, typename CL, typename CR>
  friend class ::bimap;

  template <typename Q, typename Comp, typename W>
  friend class set;

  template <typename Q, typename U, typename W>
  friend class set_iterator;

public:
  using node_base::node_base;

  node() noexcept : node_base(), val() {}

  node(const node& other) : node_base(other), val(other.val) {}

  node(node&& other) noexcept : node_base(std::move(other)), val(std::move(other.val)) {}

  node(T&& _val) : node_base(), val(std::move(_val)) {}

  node(const T& _val) : node_base(), val(_val) {}

  void swap(node& other) noexcept {
    std::swap(other.val, val);
    node_base::swap(other);
  }

protected:
  T val;
};

template <typename T, typename Tag>
class tagged_node : public node<T> {
public:
  tagged_node() = default;

  template <typename U>
  tagged_node(U&& val) : node<T>(std::forward<U>(val)) {}

  void swap(tagged_node& other) noexcept {
    node<T>::swap(other);
  }
};

template <typename T, typename V>
class binode : tagged_node<T, left_tag>, tagged_node<V, right_tag> {
private:
  template <typename Left, typename Right, typename CompareLeft, typename CompareRight>
  friend class ::bimap;

  template <typename Q, typename U, typename W>
  friend class set_iterator;

  using tagged_left_node_t = tagged_node<T, left_tag>;
  using tagged_right_node_t = tagged_node<V, right_tag>;

public:
  binode() = default;

  binode(const binode&) = default;

  binode(binode&&) = default;

  template <typename L, typename R>
  binode(L&& val1, R&& val2) : tagged_left_node_t(std::forward<L>(val1)),
                               tagged_right_node_t(std::forward<R>(val2)) {}

  void swap(binode& other) noexcept {
    tagged_left_node_t::swap(other);
    tagged_right_node_t::swap(other);
  }

  template <class Tag>
  auto get_node() noexcept {
    return static_cast<std::conditional_t<std::is_same_v<Tag, left_tag>, tagged_left_node_t, tagged_right_node_t>*>(
        this);
  }

  template <class Tag>
  auto get_node() const noexcept {
    return static_cast<
        const std::conditional_t<std::is_same_v<Tag, left_tag>, tagged_left_node_t, tagged_right_node_t>*>(this);
  }

  friend bool operator==(const binode& lhs, const binode& rhs) noexcept {
    return *lhs.tagged_left_node_t::val == *rhs.tagged_left_node_t::val &&
         *lhs.tagged_right_node_t::val == *rhs.tagged_right_node_t::val;
  }

  friend bool operator!=(const binode& lhs, const binode& rhs) noexcept {
    return !(lhs == rhs);
  }
};
} // namespace tools
