#pragma once

#include <concepts>
#include <type_traits>

template <typename... Types>
  requires(std::conjunction_v<std::is_nothrow_destructible<Types>...>)
class variant;

namespace tools {
template <std::size_t Index, typename T, typename... Types>
struct get_by_index;

struct in_place_t {};
} // namespace tools

inline constexpr std::size_t variant_npos = -1;

class bad_variant_access : public std::exception {
  const char* what() const noexcept override {
    return "bad_variant_access";
  }
};

template <typename T>
struct in_place_type_t : tools::in_place_t {
  explicit in_place_type_t() = default;
};

template <typename T>
inline constexpr in_place_type_t<T> in_place_type{};

template <std::size_t I>
struct in_place_index_t : tools::in_place_t {
  explicit in_place_index_t() = default;
};

template <std::size_t I>
inline constexpr in_place_index_t<I> in_place_index{};

template <std::size_t I, typename T>
struct variant_alternative;

template <std::size_t I, typename... Types>
struct variant_alternative<I, variant<Types...>> {
  using type = tools::get_by_index<I, Types...>::type;
};

template <std::size_t I, typename T>
using variant_alternative_t = typename variant_alternative<I, T>::type;

template <std::size_t I, typename T>
struct variant_alternative<I, const T> {
  using type = std::add_const_t<variant_alternative_t<I, T>>;
};

template <typename T>
struct variant_size;

template <typename... Types>
struct variant_size<variant<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {};

template <typename T>
struct variant_size<const T> : variant_size<T> {};

template <typename T>
inline constexpr std::size_t variant_size_v = variant_size<T>::value;

namespace tools {

struct valueless_tag_t {};

inline constexpr valueless_tag_t valueless_tag{};

template <std::size_t I, typename T, typename... Types>
struct get_by_index {
  using type = get_by_index<I - 1, Types...>::type;
};

template <typename T, typename... Types>
struct get_by_index<0, T, Types...> {
  using type = T;
};

template <std::size_t I, typename... Types>
using get_by_index_t = get_by_index<I, Types...>::type;

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
struct lst_index_by_type {
  static constexpr std::size_t value = 0;
};

template <typename F, typename... Types>
inline constexpr std::size_t lst_index_by_type_v = lst_index_by_type<F, Types...>::value;

template <typename F, typename T, typename... Types>
struct lst_index_by_type<F, T, Types...> {
  static constexpr std::size_t value = (std::is_same_v<F, T> && lst_index_by_type_v<F, Types...> >= sizeof...(Types)
                                            ? 0
                                            : lst_index_by_type_v<F, Types...> + 1);
};

template <typename F, typename... Types>
inline constexpr bool without_duplicate = fst_index_by_type_v<F, Types...> < sizeof...(Types) &&
                                          fst_index_by_type_v<F, Types...> == lst_index_by_type_v<F, Types...>;

template <typename From, typename To>
concept convertible = requires(From&& a) { new To[1]{std::forward<From>(a)}; };

template <typename... Types>
struct get_convertible {};

template <typename T, typename T_i, typename... Types>
  requires(!(tools::convertible<T, Types> || ...))
struct get_convertible<T, T_i, Types...> {
  static constexpr T_i F(T_i)
    requires(convertible<T, T_i>);
};

template <typename T, typename T_i, typename... Types>
  requires((tools::convertible<T, Types> || ...))
struct get_convertible<T, T_i, Types...> : get_convertible<T, Types...> {
  using get_convertible<T, Types...>::F;
  static constexpr T_i F(T_i)
    requires(convertible<T, T_i>);
};

template <typename T, typename... Types>
using get_convertible_t = decltype(get_convertible<T, Types...>::F(std::declval<T>()));

template <typename T, typename Var>
constexpr decltype(auto) get_impl(Var&& v) {
  if (!holds_alternative<T>(v)) {
    throw bad_variant_access();
  }
  return std::forward<Var>(v).storage.template get<T>();
}

template <std::size_t I, typename Var>
constexpr decltype(auto) get_impl(Var&& v) {
  if (I != v.index()) {
    throw bad_variant_access();
  }
  return std::forward<Var>(v).storage.template get<I>();
}

} // namespace tools
