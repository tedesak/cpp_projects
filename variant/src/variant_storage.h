#pragma once

#include "utils.h"

#include <concepts>
#include <memory>
#include <type_traits>

namespace tools {

template <bool Triv_destr, typename... Types>
struct variadic_union;

template <bool Triv_destr, typename T, typename... Types>
struct variadic_union<Triv_destr, T, Types...> {

  constexpr variadic_union()
    requires(std::is_default_constructible_v<T>)
      : actual() {}

  constexpr variadic_union(valueless_tag_t) : vu(valueless_tag) {}

  template <size_t I, typename... Args>
    requires(I == 0)
  constexpr explicit variadic_union(in_place_index_t<I>, Args&&... args) : actual(std::forward<Args>(args)...) {}

  template <size_t I, typename... Args>
    requires(I != 0)
  constexpr explicit variadic_union(in_place_index_t<I>, Args&&... args)
      : vu(in_place_index_t<I - 1>(), std::forward<Args>(args)...) {}

  ~variadic_union()
    requires(Triv_destr)
  = default;

  ~variadic_union() {}

  template <size_t I, typename... Args>
  constexpr void construct(Args&&... args) {
    if constexpr (I == 0) {
      std::construct_at<T>(std::addressof(actual), std::forward<Args>(args)...);
    } else {
      vu.template construct<I - 1>(std::forward<Args>(args)...);
    }
  }

  constexpr void destruct(size_t index) {
    if (index == 0) {
      actual.~T();
    } else {
      vu.destruct(index - 1);
    }
  }

  template <typename V>
  constexpr decltype(auto) get()
    requires(fst_index_by_type_v<V, T, Types...> <= sizeof...(Types))
  {
    return get<fst_index_by_type_v<V, T, Types...>>();
  }

  template <size_t I>
  constexpr decltype(auto) get()
    requires(I != 0 && I <= sizeof...(Types))
  {
    return (vu.template get<I - 1>());
  }

  template <size_t I>
  constexpr decltype(auto) get()
    requires(I == 0)
  {
    return (actual);
  }

  template <typename V>
  constexpr decltype(auto) get() const
    requires(fst_index_by_type_v<V, T, Types...> <= sizeof...(Types))
  {
    return get<fst_index_by_type_v<V, T, Types...>>();
  }

  template <size_t I>
  constexpr decltype(auto) get() const
    requires(I != 0 && I <= sizeof...(Types))
  {
    return (vu.template get<I - 1>());
  }

  template <size_t I>
  constexpr decltype(auto) get() const
    requires(I == 0)
  {
    return (actual);
  }

  union {
    T actual;
    variadic_union<Triv_destr, Types...> vu;
  };
};

template <bool Triv_destr>
struct variadic_union<Triv_destr> {

  constexpr variadic_union(valueless_tag_t) : valueless_value() {}

  constexpr void destruct(size_t) {}

  union {
    char valueless_value;
  };
};

template <typename... Types>
struct variant_storage : variadic_union<std::conjunction_v<std::is_trivially_destructible<Types>...>, Types...> {
  using v_union = variadic_union<std::conjunction_v<std::is_trivially_destructible<Types>...>, Types...>;
  using v_union::get;

  template <typename... Types_>
    requires(std::conjunction_v<std::is_nothrow_destructible<Types_>...>)
  friend class ::variant;

private:
  constexpr variant_storage(valueless_tag_t) : v_union(valueless_tag) {}

public:
  constexpr variant_storage()
    requires(std::is_default_constructible_v<v_union>)
      : v_union(),
        index(0) {}

  template <size_t I, typename... Args>
  constexpr explicit variant_storage(in_place_index_t<I> inPlaceIndex, Args&&... args)
    requires(std::is_constructible_v<get_by_index_t<I, Types...>, Args...>)
      : v_union(inPlaceIndex, std::forward<Args>(args)...),
        index(I) {}

  constexpr ~variant_storage() = default;

  constexpr ~variant_storage()
    requires(!std::conjunction_v<std::is_trivially_destructible<Types>...>)
  {
    reset();
  }

  template <size_t I, typename... Args>
  constexpr void init(Args&&... args)
    requires(std::is_constructible_v<get_by_index_t<I, Types...>, Args...>)
  {
    try {
      this->template construct<I>(std::forward<Args>(args)...);
    } catch (...) {
      index = variant_npos;
      throw;
    }
    index = I;
  }

  constexpr void reset() {
    if (index != variant_npos) {
      this->destruct(index);
    }
    index = variant_npos;
  }

  constexpr size_t get_index() const {
    return index;
  }

private:
  size_t index{variant_npos};
};
} // namespace tools
