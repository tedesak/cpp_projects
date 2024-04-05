#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

template <typename... Types>
class tuple;

// deduction guide to aid CTAD
template <typename... Types>
tuple(Types...) -> tuple<Types...>;

template <typename T>
struct tuple_size;

template <std::size_t N, typename T>
struct tuple_element;

template <std::size_t N, typename T>
using tuple_element_t = tuple_element<N, T>::type;

template <std::size_t N, typename... Types>
constexpr tuple_element_t<N, tuple<Types...>>& get(tuple<Types...>& t) noexcept;

template <std::size_t N, typename... Types>
constexpr tuple_element_t<N, tuple<Types...>>&& get(tuple<Types...>&& t) noexcept;

template <std::size_t N, typename... Types>
constexpr const tuple_element_t<N, tuple<Types...>>& get(const tuple<Types...>& t) noexcept;

template <std::size_t N, typename... Types>
constexpr const tuple_element_t<N, tuple<Types...>>&& get(const tuple<Types...>&& t) noexcept;

namespace tools {

template <typename F, typename... Types>
struct fst_index_by_type {
  static constexpr std::size_t value = 0;
};

template <typename F, typename... Types>
inline constexpr std::size_t fst_index_by_type_v = fst_index_by_type<F, Types...>::value;

template <typename F, typename T, typename... Types>
struct fst_index_by_type<F, T, Types...> {
  static constexpr std::size_t value = (std::is_same_v<F, T> ? 0 : fst_index_by_type_v<F, Types...> + 1);
};

template <typename F, typename... Types>
struct num_of_occurrences {
  static constexpr std::size_t value = 0;
};

template <typename F, typename... Types>
inline constexpr std::size_t num_of_occurrences_v = num_of_occurrences<F, Types...>::value;

template <typename F, typename T, typename... Types>
struct num_of_occurrences<F, T, Types...> {
  static constexpr std::size_t value = num_of_occurrences_v<F, Types...> + (std::is_same_v<F, T> ? 1 : 0);
};

template <typename... Types>
class tuple_storage {};

template <typename T>
class tuple_storage<T> {
private:
  template <typename... Args>
  friend class tuple_storage;

  template <std::size_t N, typename... _Types>
  friend constexpr ::tuple_element_t<N, ::tuple<_Types...>>& ::get(::tuple<_Types...>& t) noexcept;
  template <std::size_t N, typename... _Types>
  friend constexpr ::tuple_element_t<N, ::tuple<_Types...>>&& ::get(::tuple<_Types...>&& t) noexcept;
  template <std::size_t N, typename... _Types>
  friend constexpr const ::tuple_element_t<N, ::tuple<_Types...>>& ::get(const ::tuple<_Types...>& t) noexcept;
  template <std::size_t N, typename... _Types>
  friend constexpr const ::tuple_element_t<N, ::tuple<_Types...>>&& ::get(const ::tuple<_Types...>&& t) noexcept;

public:
  tuple_storage() noexcept
    requires(std::is_default_constructible_v<T>)
  = default;

  template <typename UT>
  constexpr tuple_storage(UT&& u_el)
    requires(std::is_constructible_v<T, UT>)
      : actual(std::forward<UT>(u_el)) {}

  template <typename UT>
  constexpr tuple_storage(tuple_storage<UT>&& other)
    requires(std::is_constructible_v<T, UT>)
      : actual(std::move(other.actual)) {}

  template <typename UT>
  constexpr tuple_storage(const tuple_storage<UT>& other)
    requires(std::is_constructible_v<T, UT>)
      : actual(other.actual) {}

private:
  template <std::size_t N>
  constexpr tuple_element_t<N, tuple<T>>& get() noexcept
    requires(N == 0)
  {
    return (actual);
  }

  template <std::size_t N>
  constexpr const tuple_element_t<N, tuple<T>>& get() const noexcept
    requires(N == 0)
  {
    return const_cast<tuple_storage<T>*>(this)->template get<N>();
  }

  template <std::size_t N>
  constexpr tuple_element_t<N, tuple<T>>&& get_rv() && noexcept
    requires(N == 0)
  {
    return std::move(actual);
  }

  template <std::size_t N>
  constexpr const tuple_element_t<N, tuple<T>>&& get_rv() const&& noexcept
    requires(N == 0)
  {
    return std::move(const_cast<tuple_storage<T>*>(this)->template get<N>());
  }

private:
  T actual{};
};

template <typename T, typename... Tail>
class tuple_storage<T, Tail...> {
private:
  template <typename... Args>
  friend class tuple_storage;

  template <std::size_t N, typename... _Types>
  friend constexpr ::tuple_element_t<N, ::tuple<_Types...>>& ::get(::tuple<_Types...>& t) noexcept;
  template <std::size_t N, typename... _Types>
  friend constexpr ::tuple_element_t<N, ::tuple<_Types...>>&& ::get(::tuple<_Types...>&& t) noexcept;
  template <std::size_t N, typename... _Types>
  friend constexpr const ::tuple_element_t<N, ::tuple<_Types...>>& ::get(const ::tuple<_Types...>& t) noexcept;
  template <std::size_t N, typename... _Types>
  friend constexpr const ::tuple_element_t<N, ::tuple<_Types...>>&& ::get(const ::tuple<_Types...>&& t) noexcept;

public:
  tuple_storage() noexcept
    requires(std::is_default_constructible_v<T> && std::is_default_constructible_v<tuple_storage<Tail...>>)
  = default;

  template <typename UT, typename... UTypes>
  constexpr tuple_storage(UT&& u_el, UTypes&&... u_tail)
    requires(sizeof...(Tail) == sizeof...(UTypes) && std::is_constructible_v<T, UT> &&
             std::conjunction_v<std::is_constructible<Tail, UTypes>...>)
      : actual(std::forward<UT>(u_el)),
        tail(std::forward<UTypes>(u_tail)...) {}

  template <typename UT, typename... UTypes>
  constexpr tuple_storage(tuple_storage<UT, UTypes...>&& other)
    requires(sizeof...(Tail) == sizeof...(UTypes) && std::is_constructible_v<T, UT> &&
             std::conjunction_v<std::is_constructible<Tail, UTypes>...>)
      : actual(std::move(other.actual)),
        tail(std::move(other.tail)) {}

  template <typename UT, typename... UTypes>
  constexpr tuple_storage(const tuple_storage<UT, UTypes...>& other)
    requires(sizeof...(Tail) == sizeof...(UTypes) && std::is_constructible_v<T, UT> &&
             std::conjunction_v<std::is_constructible<Tail, UTypes>...>)
      : actual(other.actual),
        tail(other.tail) {}

private:
  template <std::size_t N>
  constexpr tuple_element_t<N, tuple<T, Tail...>>& get() noexcept {
    if constexpr (N != 0) {
      return (tail.template get<N - 1>());
    } else {
      return (actual);
    }
  }

  template <std::size_t N>
  constexpr const tuple_element_t<N, tuple<T, Tail...>>& get() const noexcept {
    return const_cast<tuple_storage<T, Tail...>*>(this)->template get<N>();
  }

  template <std::size_t N>
  constexpr tuple_element_t<N, tuple<T, Tail...>>&& get_rv() && noexcept {
    if constexpr (N != 0) {
      return std::move(tail).template get_rv<N - 1>();
    } else {
      return std::move(actual);
    }
  }

  template <std::size_t N>
  constexpr const tuple_element_t<N, tuple<T, Tail...>>&& get_rv() const&& noexcept {
    return std::move(const_cast<tuple_storage<T, Tail...>*>(this)->template get<N>());
  }

private:
  T actual{};
  tuple_storage<Tail...> tail;
};

template <size_t I, typename... TTypes, typename... UTypes>
constexpr bool compare_eq(const tuple<TTypes...>& lhs, const tuple<UTypes...>& rhs) {
  if constexpr (I == sizeof...(TTypes)) {
    return true;
  } else {
    return get<I>(lhs) == get<I>(rhs) && compare_eq<I + 1>(lhs, rhs);
  }
}

template <size_t I, typename... TTypes, typename... UTypes>
constexpr std::common_comparison_category_t<std::compare_three_way_result_t<TTypes, UTypes>...> compare_tree_way_tuple(
    const tuple<TTypes...>& lhs, const tuple<UTypes...>& rhs)
  requires(sizeof...(TTypes) == sizeof...(UTypes))
{
  if constexpr (I == sizeof...(TTypes)) {
    return std::strong_ordering::equal;
  } else {
    auto act = get<I>(lhs) <=> get<I>(rhs);
    return act == std::strong_ordering::equal ? compare_tree_way_tuple<I + 1>(lhs, rhs) : act;
  }
}

} // namespace tools

template <typename... Types>
struct tuple_size<tuple<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {};

template <typename T>
inline constexpr std::size_t tuple_size_v = tuple_size<T>::value;

template <std::size_t N, typename T, typename... Types>
struct tuple_element<N, tuple<T, Types...>> {
  using type = tuple_element<N - 1, tuple<Types...>>::type;
};

template <typename T, typename... Types>
struct tuple_element<0, tuple<T, Types...>> {
  using type = std::remove_reference<T>::type;
};

template <typename... Types>
class tuple : public tools::tuple_storage<Types...> {
private:
  using storage = tools::tuple_storage<Types...>;

public:
  constexpr tuple() noexcept
    requires(std::conjunction_v<std::is_default_constructible<Types>...>)
  = default;

  constexpr tuple(const Types&... args)
    requires(std::conjunction_v<std::is_copy_constructible<Types>...>)
      : storage(args...) {}

  template <typename... UTypes>
  constexpr tuple(UTypes&&... args)
    requires(sizeof...(Types) == sizeof...(UTypes) && std::conjunction_v<std::is_constructible<Types, UTypes>...>)
      : storage(std::forward<UTypes>(args)...) {}

  template <typename... UTypes>
  constexpr tuple(const tuple<UTypes...>& other)
    requires(sizeof...(Types) == sizeof...(UTypes) && std::conjunction_v<std::is_constructible<Types, UTypes>...>)
      : storage(other) {}

  template <typename... UTypes>
  constexpr tuple(tuple<UTypes...>&& other)
    requires(sizeof...(Types) == sizeof...(UTypes) && std::conjunction_v<std::is_constructible<Types, UTypes>...>)
      : storage(std::move(other)) {}
};

template <std::size_t N, typename... Types>
constexpr tuple_element_t<N, tuple<Types...>>& get(tuple<Types...>& t) noexcept {
  return t.template get<N>();
}

template <std::size_t N, typename... Types>
constexpr tuple_element_t<N, tuple<Types...>>&& get(tuple<Types...>&& t) noexcept {
  return std::move(t).template get_rv<N>();
}

template <std::size_t N, typename... Types>
constexpr const tuple_element_t<N, tuple<Types...>>& get(const tuple<Types...>& t) noexcept {
  return t.template get<N>();
}

template <std::size_t N, typename... Types>
constexpr const tuple_element_t<N, tuple<Types...>>&& get(const tuple<Types...>&& t) noexcept {
  return std::move(t).template get_rv<N>();
}

template <typename T, typename... Types>
constexpr T& get(tuple<Types...>& t) noexcept
  requires(tools::num_of_occurrences_v<T, Types...> == 1)
{
  return get<tools::fst_index_by_type_v<T, Types...>, Types...>(t);
}

template <typename T, typename... Types>
constexpr T&& get(tuple<Types...>&& t) noexcept
  requires(tools::num_of_occurrences_v<T, Types...> == 1)
{
  return get<tools::fst_index_by_type_v<T, Types...>, Types...>(std::move(t));
}

template <typename T, typename... Types>
constexpr const T& get(const tuple<Types...>& t) noexcept
  requires(tools::num_of_occurrences_v<T, Types...> == 1)
{
  return get<tools::fst_index_by_type_v<T, Types...>, Types...>(t);
}

template <typename T, typename... Types>
constexpr const T&& get(const tuple<Types...>&& t) noexcept
  requires(tools::num_of_occurrences_v<T, Types...> == 1)
{
  return get<tools::fst_index_by_type_v<T, Types...>, Types...>(std::move(t));
}

template <class... Types>
constexpr tuple<std::unwrap_ref_decay_t<Types>...> make_tuple(Types&&... args) {
  return tuple<std::unwrap_ref_decay_t<Types>...>(std::forward<Types>(args)...);
}

template <typename... TTypes, typename... UTypes>
constexpr bool operator==(const tuple<TTypes...>& lhs, const tuple<UTypes...>& rhs)
  requires(sizeof...(TTypes) == sizeof...(UTypes))
{
  return tools::compare_eq<0>(lhs, rhs);
}

template <typename... TTypes, typename... UTypes>
constexpr std::common_comparison_category_t<std::compare_three_way_result_t<TTypes, UTypes>...> operator<=>(
    const tuple<TTypes...>& lhs, const tuple<UTypes...>& rhs)
  requires(sizeof...(TTypes) == sizeof...(UTypes))
{
  return tools::compare_tree_way_tuple<0>(lhs, rhs);
}
