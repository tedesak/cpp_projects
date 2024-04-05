#pragma once

#include "utils.h"
#include "variant_storage.h"
#include "visit_tools.h"

#include <compare>
#include <concepts>
#include <functional>
#include <type_traits>

template <std::size_t I, typename... Types>
constexpr decltype(auto) get(const variant<Types...>& v);

template <typename... Types>
  requires(std::conjunction_v<std::is_nothrow_destructible<Types>...>)
class variant {
private:
  template <std::size_t I, typename Var>
  friend constexpr decltype(auto) tools::get_impl(Var&& v);

  template <typename T, typename Var>
  friend constexpr decltype(auto) tools::get_impl(Var&& v);

private:
  void safe_copy_by_visit(const variant& other) {
    visit(
        [this]<typename V>(V&& val) {
          if constexpr (std::is_nothrow_copy_constructible_v<std::remove_cvref_t<V>> ||
                        !std::is_nothrow_move_constructible_v<std::remove_cvref_t<V>>) {
            this->emplace<std::remove_reference_t<V>>(std::forward<V>(val));
          } else {
            *this = variant(in_place_type_t<std::remove_reference_t<V>>(), std::forward<V>(val));
          }
        },
        const_cast<variant&>(other));
  }

  void copy_by_visit(variant&& other) {
    visit([this]<typename V>(V&& val) { this->emplace<V>(std::move(val)); }, std::move(other));
  }

  void copy_by_visit(const variant& other) {
    visit([this]<typename V>(V&& val) { this->emplace<std::remove_reference_t<V>>(std::move(val)); },
          const_cast<variant&>(other));
  }

  template <typename Var>
  void change_same_values_by_visit(Var&& other) {
    visit([this]<typename V>(V&& val) { storage.template get<std::remove_cvref_t<V>>() = std::forward<V>(val); },
          std::forward<Var>(other));
  }

public:
  constexpr variant() noexcept(std::is_nothrow_default_constructible_v<tools::get_by_index_t<0, Types...>>)
    requires(std::is_default_constructible_v<tools::get_by_index_t<0, Types...>>)
      : storage() {}

  constexpr variant(const variant& other)
    requires(std::conjunction_v<std::is_copy_constructible<Types>...> &&
             std::conjunction_v<std::is_trivially_copy_constructible<Types>...>)
  = default;

  constexpr variant(const variant& other) noexcept(std::conjunction_v<std::is_nothrow_copy_constructible<Types>...>)
    requires(std::conjunction_v<std::is_copy_constructible<Types>...> &&
             !std::conjunction_v<std::is_trivially_copy_constructible<Types>...>)
      : storage(tools::valueless_tag) {
    if (other.valueless_by_exception()) {
      return;
    }
    copy_by_visit(other);
  }

  constexpr variant(variant&& other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Types>...>)
    requires(std::conjunction_v<std::is_move_constructible<Types>...> &&
             std::conjunction_v<std::is_trivially_move_constructible<Types>...>)
  = default;

  constexpr variant(variant&& other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Types>...>)
    requires(std::conjunction_v<std::is_move_constructible<Types>...> &&
             !std::conjunction_v<std::is_trivially_move_constructible<Types>...>)
      : storage(tools::valueless_tag) {
    if (other.valueless_by_exception()) {
      return;
    }
    copy_by_visit(std::move(other));
  }

  template <typename T, typename T_R = std::remove_cvref_t<T>>
    requires((tools::convertible<T, Types> || ...) && !std::is_same_v<T_R, variant> &&
             !std::convertible_to<T_R, tools::in_place_t>)
  constexpr variant(T&& x) noexcept(std::is_nothrow_constructible_v<tools::get_convertible_t<T, Types...>, T>)
      : variant(in_place_type_t<tools::get_convertible_t<T, Types...>>(), std::forward<T>(x)) {}

  template <class T, class... Args>
  constexpr explicit variant(in_place_type_t<T>, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    requires(tools::without_duplicate<T, Types...> && std::constructible_from<T, Args...>)
      : storage(in_place_index_t<tools::fst_index_by_type_v<T, Types...>>(), std::forward<Args>(args)...) {}

  template <std::size_t I, class... Args>
  constexpr explicit variant(in_place_index_t<I>, Args&&... args) noexcept(
      std::is_nothrow_constructible_v<tools::get_by_index_t<I, Types...>, Args...>)
    requires(I < sizeof...(Types) && std::constructible_from<tools::get_by_index_t<I, Types...>, Args...>)
      : storage(in_place_index_t<I>(), std::forward<Args>(args)...) {}

  constexpr ~variant() = default;

  constexpr variant& operator=(const variant& other)
    requires(std::conjunction_v<std::is_trivially_copy_constructible<Types>...> &&
             std::conjunction_v<std::is_trivially_copy_assignable<Types>...> &&
             std::conjunction_v<std::is_trivially_destructible<Types>...>)
  = default;

  constexpr variant& operator=(const variant& other) noexcept(
      std::conjunction_v<std::is_nothrow_copy_constructible<Types>...> &&
      std::conjunction_v<std::is_nothrow_copy_assignable<Types>...>)
    requires(!(std::conjunction_v<std::is_trivially_copy_constructible<Types>...> &&
               std::conjunction_v<std::is_trivially_copy_assignable<Types>...> &&
               std::conjunction_v<std::is_trivially_destructible<Types>...>) &&
             std::conjunction_v<std::is_copy_constructible<Types>...> &&
             std::conjunction_v<std::is_copy_assignable<Types>...>)
  {
    if (this == std::addressof(other)) {
      return *this;
    }
    if (other.valueless_by_exception()) {
      storage.reset();
      return *this;
    }
    if (index() == other.index()) {
      change_same_values_by_visit(other);
      return *this;
    }
    safe_copy_by_visit(other);
    return *this;
  }

  constexpr variant& operator=(variant&&) noexcept
    requires(std::conjunction_v<std::is_trivially_move_constructible<Types>...> &&
             std::conjunction_v<std::is_trivially_move_assignable<Types>...> &&
             std::conjunction_v<std::is_trivially_destructible<Types>...>)
  = default;

  constexpr variant& operator=(variant&& other) noexcept(
      std::conjunction_v<std::is_nothrow_move_constructible<Types>...> &&
      std::conjunction_v<std::is_nothrow_move_assignable<Types>...>)
    requires(!(std::conjunction_v<std::is_trivially_move_constructible<Types>...> &&
               std::conjunction_v<std::is_trivially_move_assignable<Types>...> &&
               std::conjunction_v<std::is_trivially_destructible<Types>...>) &&
             std::conjunction_v<std::is_move_constructible<Types>...> &&
             std::conjunction_v<std::is_move_assignable<Types>...>)
  {
    if (this == std::addressof(other)) {
      return *this;
    }
    if (other.valueless_by_exception()) {
      storage.reset();
      return *this;
    }
    if (index() == other.index()) {
      change_same_values_by_visit(std::move(other));
      return *this;
    }
    copy_by_visit(std::move(other));
    return *this;
  }

  template <typename T, typename T_R = std::remove_cvref_t<T>>
    requires((tools::convertible<T, Types> || ...) && !std::is_same_v<T_R, variant> &&
             !std::convertible_to<T_R, tools::in_place_t>)
  constexpr variant& operator=(T&& x) noexcept(std::is_nothrow_assignable_v<tools::get_convertible_t<T, Types...>, T>) {
    using T_j = tools::get_convertible_t<T, Types...>;
    if (index() == tools::fst_index_by_type_v<T_j, Types...>) {
      storage.template get<T_j>() = std::forward<T>(x);
      return *this;
    }
    if constexpr (std::is_nothrow_constructible_v<T_j, T> || !std::is_nothrow_move_constructible_v<T_j>) {
      emplace<T_j>(std::forward<T>(x));
    } else {
      emplace<T_j>(T_j(std::forward<T>(x)));
    }
    return *this;
  }

  constexpr std::size_t index() const noexcept {
    return storage.get_index();
  }

  constexpr bool valueless_by_exception() const noexcept {
    return storage.get_index() == variant_npos;
  }

  template <typename T, typename... Args>
  constexpr T& emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    requires(tools::without_duplicate<T, Types...> && std::constructible_from<T, Args...>)
  {
    return emplace<tools::fst_index_by_type_v<T, Types...>, Args...>(std::forward<Args>(args)...);
  }

  template <std::size_t I, typename... Args>
  constexpr variant_alternative_t<I, variant>& emplace(Args&&... args) noexcept(
      std::is_nothrow_constructible_v<tools::get_by_index_t<I, Types...>, Args...>)
    requires(I < sizeof...(Types) && std::constructible_from<tools::get_by_index_t<I, Types...>, Args...>)
  {
    storage.reset();
    storage.template init<I>(std::forward<Args>(args)...);
    return get<I>(*this);
  }

  constexpr void swap(variant& other) {
    if (!valueless_by_exception() && other.valueless_by_exception()) {
      other = std::move(*this);
      storage.reset();
    } else if (valueless_by_exception() && !other.valueless_by_exception()) {
      *this = std::move(other);
      other.storage.reset();
    } else if (!valueless_by_exception()) {
      if (index() == other.index()) {
        return visit(
            []<typename A, typename B>(A&& a, B&& b) {
              if constexpr (std::is_same_v<A, B>) {
                using std::swap;
                swap(a, b);
              }
            },
            *this, other);
      } else {
        std::swap(*this, other);
      }
    }
  }

private:
  tools::variant_storage<Types...> storage;
};

template <typename T, typename... Types>
constexpr bool holds_alternative(const variant<Types...>& v) noexcept {
  return v.index() == tools::fst_index_by_type_v<T, Types...>;
}

template <typename T, typename... Types>
constexpr decltype(auto) get(variant<Types...>& v) {
  return tools::get_impl<T>(v);
}

template <typename T, typename... Types>
constexpr decltype(auto) get(const variant<Types...>& v) {
  return tools::get_impl<T>(v);
}

template <typename T, typename... Types>
constexpr decltype(auto) get(variant<Types...>&& v) {
  return std::move(tools::get_impl<T>(std::move(v)));
}

template <typename T, typename... Types>
constexpr decltype(auto) get(const variant<Types...>&& v) {
  return std::move(tools::get_impl<T>(std::move(v)));
}

template <std::size_t I, typename... Types>
constexpr decltype(auto) get(variant<Types...>& v)
  requires(I < sizeof...(Types))
{
  return tools::get_impl<I>(v);
}

template <std::size_t I, typename... Types>
constexpr decltype(auto) get(const variant<Types...>& v)
  requires(I < sizeof...(Types))
{
  return tools::get_impl<I>(v);
}

template <std::size_t I, typename... Types>
constexpr decltype(auto) get(variant<Types...>&& v)
  requires(I < sizeof...(Types))
{
  return std::move(tools::get_impl<I>(std::move(v)));
}

template <std::size_t I, typename... Types>
constexpr decltype(auto) get(const variant<Types...>&& v)
  requires(I < sizeof...(Types))
{
  return std::move(tools::get_impl<I>(std::move(v)));
}

template <typename T, typename... Types>
constexpr std::add_pointer_t<T> get_if(variant<Types...>* pv) noexcept
  requires(tools::without_duplicate<T, Types...>)
{
  return get_if<tools::fst_index_by_type_v<T, Types...>, Types...>(pv);
}

template <std::size_t I, typename... Types>
constexpr std::add_pointer_t<variant_alternative_t<I, variant<Types...>>> get_if(variant<Types...>* pv) noexcept
  requires(I < sizeof...(Types))
{
  return (pv != nullptr && pv->index() == I ? std::addressof(get<I>(*pv)) : nullptr);
}

template <typename R, typename Visitor, typename... Variants>
constexpr R visit(Visitor&& vis, Variants&&... vars)
  requires(!tools::visit::is_visitor<R, Variants...> && tools::visit::is_visitor<Visitor, Variants...>)
{
  if ((vars.valueless_by_exception() || ...)) {
    throw bad_variant_access();
  }
  return tools::visit::multi_array_get(tools::visit::create_visit_table<R, Visitor, Variants...>(),
                                       vars.index()...)(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

template <typename Visitor, typename... Variants>
constexpr decltype(auto) visit(Visitor&& vis, Variants&&... vars)
  requires(tools::visit::is_visitor<Visitor, Variants...>)
{
  using R = std::invoke_result_t<Visitor, tools::visit::get_fst_t_variant_t<Variants>...>;
  return visit<R>(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

namespace tools {
template <typename Comp, typename Var>
constexpr bool compare_by_visit(Comp&& comp, const Var& v, const Var& w) {
  return ::visit<bool>(
      [&]<typename A, typename B>(A&& a, B&& b) {
        if constexpr (!std::is_same_v<A, B>) {
          return false;
        } else {
          return comp(a, b);
        }
      },
      v, w);
}

// IsEq:     F ->              ; T ->
// RevCond:  F -> v, w         ; T -> w, v
// RevVal:   F -> true, false  ; T -> false, true
template <bool IsEq, bool RevCond, bool RevVal, typename Comp, typename Var>
constexpr bool compare_impl(Comp&& comp, const Var& v, const Var& w) {
  if (IsEq && v.index() != w.index()) {
    return RevVal;
  }
  if (!RevCond && v.valueless_by_exception()) {
    return !RevVal;
  }
  if (w.valueless_by_exception()) {
    return !RevCond ^ !RevVal;
  }
  if (RevCond && v.valueless_by_exception()) {
    return RevVal;
  }
  if constexpr (IsEq) {
    return tools::compare_by_visit(std::forward<Comp>(comp), v, w);
  } else {
    if (v.index() == w.index()) {
      return tools::compare_by_visit(std::forward<Comp>(comp), v, w);
    }
    return comp(v.index(), w.index());
  }
}
} // namespace tools

template <typename... Types>
constexpr bool operator==(const variant<Types...>& v, const variant<Types...>& w) {
  return tools::compare_impl<true, false, false>(std::equal_to(), v, w);
}

template <typename... Types>
constexpr bool operator!=(const variant<Types...>& v, const variant<Types...>& w) {
  return tools::compare_impl<true, false, true>(std::not_equal_to(), v, w);
}

template <typename... Types>
constexpr bool operator<(const variant<Types...>& v, const variant<Types...>& w) {
  return tools::compare_impl<false, true, true>(std::less(), v, w);
}

template <typename... Types>
constexpr bool operator>(const variant<Types...>& v, const variant<Types...>& w) {
  return tools::compare_impl<false, false, true>(std::greater(), v, w);
}

template <typename... Types>
constexpr bool operator<=(const variant<Types...>& v, const variant<Types...>& w) {
  return tools::compare_impl<false, false, false>(std::less_equal(), v, w);
}

template <typename... Types>
constexpr bool operator>=(const variant<Types...>& v, const variant<Types...>& w) {
  return tools::compare_impl<false, true, false>(std::greater_equal(), v, w);
}

template <typename... Types>
constexpr std::common_comparison_category_t<std::compare_three_way_result_t<Types>...> operator<=>(
    const variant<Types...>& v, const variant<Types...>& w) {
  if (w.valueless_by_exception() && v.valueless_by_exception()) {
    return std::strong_ordering::equal;
  }
  if (v.valueless_by_exception()) {
    return std::strong_ordering::less;
  }
  if (w.valueless_by_exception()) {
    return std::strong_ordering::greater;
  }
  if (v.index() != w.index()) {
    return v.index() <=> w.index();
  }
  return visit(
      []<typename A, typename B>(
          A&& a, B&& b) -> std::common_comparison_category_t<std::compare_three_way_result_t<Types>...> {
        if constexpr (!std::is_same_v<A, B>) {
          return std::strong_ordering::equal;
        } else {
          return a <=> b;
        }
      },
      v, w);
}
