#pragma once

#include "utils.h"

#include <array>
#include <concepts>
#include <type_traits>
#include <utility>

namespace tools::visit {

template <typename... Variants>
struct get_fst_t_variant;

template <typename Type, typename... Types>
struct get_fst_t_variant<variant<Type, Types...>> {
  using type = Type;
};

template <typename Type, typename... Types>
struct get_fst_t_variant<const variant<Type, Types...>> {
  using type = const Type;
};

template <typename Type, typename... Types>
struct get_fst_t_variant<variant<Type, Types...>&> {
  using type = Type&;
};

template <typename Type, typename... Types>
struct get_fst_t_variant<const variant<Type, Types...>&> {
  using type = const Type&;
};

template <typename Type, typename... Types>
struct get_fst_t_variant<variant<Type, Types...>&&> {
  using type = Type&&;
};

template <typename Type, typename... Types>
struct get_fst_t_variant<const variant<Type, Types...>&&> {
  using type = const Type&&;
};

template <typename Variant>
using get_fst_t_variant_t = get_fst_t_variant<Variant>::type;

template <typename Visitor, typename... Variants>
concept is_visitor = std::is_invocable_v<Visitor, get_fst_t_variant_t<Variants>...>;

template <typename T, typename... Vs>
constexpr std::array<T, sizeof...(Vs) + 1> create_multi_array(T fst, Vs... vs) {
  return {fst, vs...};
}

template <typename Multi_arr>
constexpr decltype(auto) multi_array_get(Multi_arr&& multi_arr) {
  return std::forward<Multi_arr>(multi_arr);
}

template <typename Multi_arr, typename... Tail>
constexpr decltype(auto) multi_array_get(Multi_arr&& multi_arr, std::size_t act_index, Tail&&... tail) {
  return multi_array_get(std::forward<Multi_arr>(multi_arr)[act_index], std::forward<Tail>(tail)...);
}

template <typename R, typename Vis, typename... Vars, std::size_t... Act_seq>
constexpr decltype(auto) visit_table(std::index_sequence<Act_seq...>) {
  struct visitor_wrapper {
    constexpr decltype(auto) operator()(Vis && v, Vars&&... vars) {
      return call_visitor(std::forward<Vis>(v), std::forward<Vars>(vars)...);
    }

    static constexpr R call_visitor(Vis&& v, Vars&&... vars) {
      return std::forward<Vis>(v)(get<Act_seq>(std::forward<Vars>(vars))...);
    }
  };

  if constexpr (std::convertible_to<std::invoke_result_t<visitor_wrapper, Vis, Vars...>, R>) {
    return &visitor_wrapper::call_visitor;
  } else {
    return nullptr;
  }
}

template <typename R, typename Vis, typename... Vars, typename... Tail, std::size_t... Act_seq, std::size_t... Add_seq>
constexpr auto visit_table(std::index_sequence<Act_seq...>, std::index_sequence<Add_seq...>, Tail&&... tail) {
  return create_multi_array(
      visit_table<R, Vis, Vars...>(std::index_sequence<Act_seq..., Add_seq>(), std::forward<Tail>(tail)...)...);
}

template <typename R, typename Vis, typename... Vars>
constexpr decltype(auto) create_visit_table() {
  return visit_table<R, Vis, Vars...>(std::index_sequence<>(),
                                      std::make_index_sequence<variant_size_v<std::remove_cvref_t<Vars>>>()...);
}

} // namespace tools::visit
