#pragma once

#include "set.h"
#include "set_iterator.h"
#include "utils.h"

#include <cassert>
#include <iterator>
#include <type_traits>

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
class bimap : tools::set<Left, CompareLeft, tools::left_tag>, tools::set<Right, CompareRight, tools::right_tag> {
public:
  using left_iterator = tools::set_iterator<Left, Right, tools::left_tag>;
  using right_iterator = tools::set_iterator<Left, Right, tools::right_tag>;

private:
  using binode_t = tools::binode<Left, Right>;

  using left_set_t = tools::set<Left, CompareLeft, tools::left_tag>;
  using right_set_t = tools::set<Right, CompareRight, tools::right_tag>;

  template <typename Tag>
  using rev_tag_t = std::conditional_t<std::is_same_v<Tag, tools::left_tag>, tools::right_tag, tools::left_tag>;

  template <typename Tag>
  using tag_iterator_t = std::conditional_t<std::is_same_v<Tag, tools::left_tag>, left_iterator, right_iterator>;

  template <typename Tag>
  using tag_set_t = std::conditional_t<std::is_same_v<Tag, tools::left_tag>, left_set_t, right_set_t>;

  template <typename Tag>
  using tag_value_t = std::conditional_t<std::is_same_v<Tag, tools::left_tag>, Left, Right>;

private:
  template <typename Tag>
  constexpr const tools::node_base* get_root() const noexcept {
    if constexpr (std::is_same_v<Tag, tools::left_tag>) {
      return &_root_l;
    } else {
      return &_root_r;
    }
  }

  template <typename Tag>
  static constexpr binode_t* node_to_binode(tools::node_base* node) noexcept {
    return static_cast<binode_t*>(static_cast<tools::tagged_node<tag_value_t<Tag>, Tag>*>(node));
  }

  template <typename Tag>
  static constexpr tools::node<tag_value_t<Tag>>* binode_to_node(binode_t* binode) noexcept {
    return static_cast<tools::tagged_node<tag_value_t<Tag>, Tag>*>(binode);
  }

  static void recursive_destrustor(tools::node_base* del_el) {
    if (del_el->left != nullptr) {
      recursive_destrustor(del_el->left);
    }
    if (del_el->right != nullptr) {
      recursive_destrustor(del_el->right);
    }
    delete node_to_binode<tools::left_tag>(del_el);
  }

  void clear() {
    if (!empty()) {
      recursive_destrustor(std::exchange(_root_l.left, nullptr));
      _root_r.left = nullptr;
    }
  }

  template <typename L, typename R>
  left_iterator insert_impl(L&& left, R&& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }
    auto new_binode = new binode_t(std::forward<L>(left), std::forward<R>(right));
    right_set_t::insert(binode_to_node<tools::right_tag>(new_binode));
    auto ans = left_set_t::insert(binode_to_node<tools::left_tag>(new_binode)).first;
    _size++;
    return ans;
  }

  template <typename Tag>
  tag_iterator_t<Tag> erase_impl(tag_iterator_t<Tag> it) {
    if (it == end_impl<Tag>()) {
      return end_impl<Tag>();
    }
    tag_set_t<rev_tag_t<Tag>>::erase(binode_to_node<rev_tag_t<Tag>>(it.get_binode()));
    auto ans = tag_set_t<Tag>::erase(binode_to_node<Tag>(it.get_binode()));
    _size--;
    delete it.get_binode();
    return ans;
  }

  template <typename Tag>
  bool erase_impl(const tag_value_t<Tag>& val) {
    auto pos = find_impl<Tag>(val);
    if (pos == end_impl<Tag>()) {
      return false;
    }
    erase_impl<Tag>(pos);
    return true;
  }

  template <typename Tag>
  tag_iterator_t<Tag> erase_impl(tag_iterator_t<Tag> first, tag_iterator_t<Tag> last) {
    while (first != last) {
      first = erase_impl<Tag>(first);
    }
    return last;
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  template <typename Tag>
  tag_iterator_t<Tag> find_impl(const tag_value_t<Tag>& val) const {
    return tag_set_t<Tag>::find(val);
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  template <typename Tag>
  const tag_value_t<rev_tag_t<Tag>>& at_impl(const tag_value_t<Tag>& key) const {
    auto pos = find_impl<Tag>(key);
    if (pos._node == get_root<Tag>()) {
      throw std::out_of_range("Wrong argument in function \'at_*\'");
    }
    return pos.get_binode()->template get_node<rev_tag_t<Tag>>()->val;
  }

  template <typename Tag>
  left_iterator insert_trans(const tag_value_t<Tag>& left, const tag_value_t<rev_tag_t<Tag>>& right) {
    if constexpr (std::is_same_v<Tag, tools::left_tag>) {
      return insert(left, right);
    } else {
      return insert(right, left);
    }
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)
  template <typename Tag>
  const tag_value_t<rev_tag_t<Tag>>& at_or_default_impl(const tag_value_t<Tag>& key) {
    auto left_iter = find_impl<Tag>(key);
    if (left_iter != end_impl<Tag>()) {
      return left_iter.get_binode()->template get_node<rev_tag_t<Tag>>()->val;
    }
    auto right_val = tag_value_t<rev_tag_t<Tag>>();
    auto right_iter = find_impl<rev_tag_t<Tag>>(right_val);
    if (right_iter == end_impl<rev_tag_t<Tag>>()) {
      return insert_trans<Tag>(key, right_val).get_binode()->template get_node<rev_tag_t<Tag>>()->val;
    }
    auto left_node = right_iter.get_binode()->template get_node<Tag>();
    tag_set_t<Tag>::erase(left_node);
    _size--;
    left_node->val = key;
    tag_set_t<Tag>::insert(left_node);
    _size++;
    return right_iter.get_binode()->template get_node<rev_tag_t<Tag>>()->val;
  }

  template <typename Tag>
  tag_iterator_t<Tag> lower_bound_impl(const tag_value_t<Tag>& val) const {
    return tag_set_t<Tag>::lower_bound(val);
  }

  template <typename Tag>
  tag_iterator_t<Tag> upper_bound_impl(const tag_value_t<Tag>& val) const {
    return tag_set_t<Tag>::upper_bound(val);
  }

  template <typename Tag>
  tag_iterator_t<Tag> begin_impl() const noexcept {
    return tag_set_t<Tag>::begin();
  }

  template <typename Tag>
  tag_iterator_t<Tag> end_impl() const noexcept {
    return const_cast<tools::node_base*>(get_root<Tag>());
  }

  bool binode_equal(binode_t* lhs, binode_t* rhs) const noexcept {
    return left_set_t::node_equal(binode_to_node<tools::left_tag>(lhs), binode_to_node<tools::left_tag>(rhs)) &&
           right_set_t::node_equal(binode_to_node<tools::right_tag>(lhs), binode_to_node<tools::right_tag>(rhs));
  }

  void init_roots() {
    _root_l.parent = &_root_r;
    _root_r.parent = &_root_l;
  }

public:
  bimap(CompareLeft compare_left = CompareLeft(), CompareRight compare_right = CompareRight())
      : left_set_t(&_root_l, std::move(compare_left)),
        right_set_t(&_root_r, std::move(compare_right)),
        _root_l(),
        _root_r(),
        _size(0) {
    init_roots();
  }

  bimap(const bimap& other)
      : left_set_t(&_root_l, static_cast<CompareLeft>(static_cast<left_set_t>(other))),
        right_set_t(&_root_r, static_cast<CompareRight>(static_cast<right_set_t>(other))),
        _root_l(),
        _root_r(),
        _size(0) {
    init_roots();
    if (other.empty()) {
      return;
    }
    for (left_iterator it_other = other.begin_left(); it_other != other.end_left(); it_other++) {
      binode_t* binode = node_to_binode<tools::left_tag>(it_other._node);
      try {
        insert(binode->template get_node<tools::left_tag>()->val, binode->template get_node<tools::right_tag>()->val);
      } catch (...) {
        clear();
        throw;
      }
    }
  }

  bimap(bimap&& other) noexcept
      : left_set_t(&_root_l, std::move(*static_cast<CompareLeft*>(static_cast<left_set_t*>(&other)))),
        right_set_t(&_root_r, std::move(*static_cast<CompareRight*>(static_cast<right_set_t*>(&other)))),
        _root_l(std::move(other._root_l)),
        _root_r(std::move(other._root_r)),
        _size(std::exchange(other._size, 0)) {
    init_roots();
  }

  bimap& operator=(const bimap& other) {
    if (&other == this) {
      return *this;
    }
    *this = bimap(other);
    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    if (&other == this) {
      return *this;
    }
    clear();
    _size = std::exchange(other._size, 0);
    _root_l.swap(other._root_l);
    _root_r.swap(other._root_r);
    return *this;
  }

  ~bimap() {
    clear();
  }

  friend void swap(bimap& lhs, bimap& rhs) noexcept {
    std::swap(lhs._size, rhs._size);
    lhs._root_l.swap(rhs._root_l);
    lhs._root_r.swap(rhs._root_r);
  }

  left_iterator insert(const Left& left, const Right& right) {
    return insert_impl(left, right);
  }

  left_iterator insert(const Left& left, Right&& right) {
    return insert_impl(left, std::move(right));
  }

  left_iterator insert(Left&& left, const Right& right) {
    return insert_impl(std::move(left), right);
  }

  left_iterator insert(Left&& left, Right&& right) {
    return insert_impl(std::move(left), std::move(right));
  }

  left_iterator erase_left(left_iterator it) {
    return erase_impl<tools::left_tag>(it);
  }

  right_iterator erase_right(right_iterator it) {
    return erase_impl<tools::right_tag>(it);
  }

  bool erase_left(const Left& left) {
    return erase_impl<tools::left_tag>(left);
  }

  bool erase_right(const Right& right) {
    return erase_impl<tools::right_tag>(right);
  }

  left_iterator erase_left(left_iterator first, left_iterator last) {
    return erase_impl<tools::left_tag>(first, last);
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
    return erase_impl<tools::right_tag>(first, last);
  }

  left_iterator find_left(const Left& left) const {
    return find_impl<tools::left_tag>(left);
  }

  right_iterator find_right(const Right& right) const {
    return find_impl<tools::right_tag>(right);
  }

  const Right& at_left(const Left& key) const {
    return at_impl<tools::left_tag>(key);
  }

  const Left& at_right(const Right& key) const {
    return at_impl<tools::right_tag>(key);
  }

  template <typename R = Right, typename = std::enable_if_t<std::is_default_constructible_v<R>>,
            typename = std::enable_if_t<std::is_same_v<R, Right>>>
  const R& at_left_or_default(const Left& key) {
    return at_or_default_impl<tools::left_tag>(key);
  }

  template <typename L = Left, typename = std::enable_if_t<std::is_default_constructible_v<L>>,
            typename = std::enable_if_t<std::is_same_v<L, Left>>>
  const L& at_right_or_default(const Right& key) {
    return at_or_default_impl<tools::right_tag>(key);
  }

  left_iterator lower_bound_left(const Left& left) const {
    return lower_bound_impl<tools::left_tag>(left);
  }

  right_iterator lower_bound_right(const Right& right) const {
    return lower_bound_impl<tools::right_tag>(right);
  }

  left_iterator upper_bound_left(const Left& left) const {
    return upper_bound_impl<tools::left_tag>(left);
  }

  right_iterator upper_bound_right(const Right& right) const {
    return upper_bound_impl<tools::right_tag>(right);
  }

  left_iterator begin_left() const noexcept {
    return begin_impl<tools::left_tag>();
  }

  right_iterator begin_right() const noexcept {
    return begin_impl<tools::right_tag>();
  }

  left_iterator end_left() const noexcept {
    return end_impl<tools::left_tag>();
  }

  right_iterator end_right() const noexcept {
    return end_impl<tools::right_tag>();
  }

  bool empty() const noexcept {
    return _size == 0;
  }

  std::size_t size() const noexcept {
    return _size;
  }

  friend bool operator==(const bimap& lhs, const bimap& rhs) noexcept {
    if (lhs.size() != rhs.size()) {
      return false;
    }
    auto itl = lhs.begin_left(), itr = rhs.begin_left();
    size_t it_size = 0;
    while (itl != lhs.end_left() && itr != rhs.end_left()) {
      if (!lhs.binode_equal(itl.get_binode(), itr.get_binode())) {
        return false;
      }
      itl++;
      itr++;
      it_size++;
    }
    return itl == lhs.end_left() && itr == rhs.end_left() && lhs.size() == it_size;
  }

  friend bool operator!=(const bimap& lhs, const bimap& rhs) noexcept {
    return !(lhs == rhs);
  }

private:
  tools::node_base _root_l;
  tools::node_base _root_r;
  size_t _size;
};
