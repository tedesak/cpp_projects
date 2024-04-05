#pragma once

#include "utils.h"

#include <stdexcept>

template <typename L, typename R, typename CL, typename CR>
class bimap;

namespace tools {
template <typename T, typename Comp, typename Tag>
class set : Comp {
private:
  template <typename Left, typename Right, typename CompareLeft, typename CompareRight>
  friend class ::bimap;

  using node_t = node<T>;

private:
  static node_base* get_least(node_base* _node) {
    while (_node->left != nullptr) {
      _node = _node->left;
    }
    return _node;
  }

  bool node_equal(node_t* lhs, node_t* rhs) const {
    return !this->operator()(lhs->val, rhs->val) && !this->operator()(rhs->val, lhs->val);
  }

  template <typename C>
  node_base* bound_impl(const T& val, C c) const {
    if (is_empty()) {
      return end();
    }
    node_base *node_ = _root->left, *ans = nullptr;
    while (true) {
      if (c.operator()(static_cast<node_t*>(node_)->val, val)) {
        ans = node_;
        if (node_->left != nullptr) {
          node_ = node_->left;
          continue;
        }
        break;
      }
      if (node_->right != nullptr) {
        node_ = node_->right;
        continue;
      }
      break;
    }
    return ans == nullptr ? end() : ans;
  }

  bool is_empty() const {
    return _root->left == nullptr;
  }

public:
  set() = default;

  template <typename U>
  set(node_base* root, U&& comp) : Comp(std::forward<U>(comp)),
                                   _root(root) {}

  node_base* begin() const {
    node_base* beg = _root;
    while (beg->left != nullptr) {
      beg = beg->left;
    }
    return beg;
  }

  node_base* end() const {
    return _root;
  }

  node_base* find(const T& val) const {
    node_base* ans = lower_bound(val);
    return ans == end() || !(val == static_cast<node_t*>(ans)->val) ? end() : ans;
  }

  node_base* lower_bound(const T& val) const {
    return bound_impl(val, [this](const T& val1, const T& val2) { return !this->operator()(val1, val2); });
  }

  node_base* upper_bound(const T& val) const {
    return bound_impl(val, [this](const T& val1, const T& val2) { return this->operator()(val2, val1); });
  }

  std::pair<node_base*, bool> insert(node_base* node_) {
    node_base* pos = lower_bound(static_cast<node_t*>(node_)->val);
    if (pos == node_) {
      return {pos, false};
    }
    node_->relink(nullptr, nullptr, nullptr);
    if (is_empty()) {
      _root->left_link(node_);
      return {node_, true};
    }
    auto it_node = _root->left;
    while (true) {
      if (this->operator()(static_cast<node_t*>(node_)->val, static_cast<node_t*>(it_node)->val)) {
        if (it_node->left == nullptr) {
          it_node->left_link(node_);
          return {node_, true};
        }
        it_node = it_node->left;
      } else {
        if (it_node->right == nullptr) {
          it_node->right_link(node_);
          return {node_, true};
        }
        it_node = it_node->right;
      }
    }
  }

  node_base* erase(node_base* pos) {
    if (pos->left != nullptr && pos->right != nullptr) {
      auto right_least = get_least(pos->right);
      if (right_least->parent->left == right_least) {
        right_least->parent->left_link(right_least->right);
      } else {
        right_least->parent->right_link(right_least->right);
      }
      right_least->copy_links(pos);
    } else {
      auto good_child = pos->left == nullptr ? pos->right : pos->left;
      if (pos->parent->left == pos) {
        pos->parent->left_link(good_child);
      } else {
        pos->parent->right_link(good_child);
      }
    }
    return (pos->right == nullptr ? pos->parent : pos->right);
  }

public:
  node_base* _root;
};
} // namespace tools
