#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>

struct nullopt_t {
  constexpr explicit nullopt_t(int) {}
};

inline constexpr nullopt_t nullopt{0};

struct in_place_t {
  explicit in_place_t() = default;
};

inline constexpr in_place_t in_place{};

namespace tools {

template <typename T, bool have_destr>
struct optional_base {

  constexpr optional_base() noexcept {}

  constexpr optional_base(const optional_base&) noexcept = default;

  constexpr optional_base(optional_base&&) noexcept = default;

  constexpr optional_base(T x) : data(std::move(x)), is_present{true} {}

  optional_base& operator=(const optional_base&) noexcept = default;

  optional_base& operator=(optional_base&&) noexcept = default;

  constexpr void reset() noexcept {
    if (is_present) {
      data.~T();
      is_present = false;
    }
  }

  constexpr ~optional_base() {
    reset();
  }

  union {
    T data;
  };

  bool is_present = false;
};

template <typename T>
struct optional_base<T, true> {

  constexpr optional_base() noexcept {}

  constexpr optional_base(const optional_base&) noexcept = default;

  constexpr optional_base(optional_base&&) noexcept = default;

  constexpr optional_base(T x) : data(std::move(x)), is_present{true} {}

  optional_base& operator=(const optional_base&) noexcept = default;

  optional_base& operator=(optional_base&&) noexcept = default;

  constexpr void reset() noexcept {
    if (is_present) {
      data.~T();
      is_present = false;
    }
  }

  constexpr ~optional_base() = default;

  union {
    T data;
  };

  bool is_present = false;
};

template <typename T>
struct optional_impl : optional_base<T, std::is_trivially_destructible_v<T>> {
  using optional_base<T, std::is_trivially_destructible_v<T>>::optional_base;

  constexpr optional_impl() noexcept = default;

  constexpr optional_impl(const optional_impl&) noexcept = default;

  constexpr optional_impl(optional_impl&&) noexcept = default;

  optional_impl& operator=(const optional_impl&) noexcept = default;

  optional_impl& operator=(optional_impl&&) noexcept = default;

  ~optional_impl() = default;

  template <typename... U>
  constexpr void init(bool is_present_, U&&... data_) {
    std::construct_at(&this->data, std::forward<U>(data_)...);
    this->is_present = is_present_;
  }

  constexpr explicit operator bool() const noexcept {
    return this->is_present;
  }

  constexpr T& operator*() & noexcept {
    return *operator->();
  }

  constexpr const T& operator*() const& noexcept {
    return *operator->();
  }

  constexpr T&& operator*() && noexcept {
    return std::move(this->data);
  }

  constexpr const T&& operator*() const&& noexcept {
    return std::move(this->data);
  }

  constexpr T* operator->() noexcept {
    return &this->data;
  }

  constexpr const T* operator->() const noexcept {
    return &this->data;
  }
};

template <typename T, bool triv>
struct copy_base : optional_impl<T> {
  using optional_impl<T>::optional_impl;

  constexpr copy_base(copy_base&&) = default;
  constexpr copy_base& operator=(const copy_base&) = default;
  constexpr copy_base& operator=(copy_base&&) = default;

  constexpr copy_base(const copy_base& other) : optional_impl<T>() {
    this->is_present = other.is_present;
    if (this->is_present) {
      this->init(other.is_present, other.data);
    }
  }
};

template <typename T>
struct copy_base<T, true> : optional_impl<T> {
  using optional_impl<T>::optional_impl;
};

template <typename T>
using copy_base_t = copy_base<T, std::is_trivially_copy_constructible_v<T>>;

template <typename T, bool triv>
struct move_base : copy_base_t<T> {
  using copy_base_t<T>::copy_base_t;

  constexpr move_base(const move_base&) = default;
  constexpr move_base& operator=(const move_base&) = default;
  constexpr move_base& operator=(move_base&&) = default;

  constexpr move_base(move_base&& other) {
    this->is_present = other.is_present;
    if (this->is_present) {
      this->init(other.is_present, std::move(other.data));
    }
  }
};

template <typename T>
struct move_base<T, true> : copy_base_t<T> {
  using copy_base_t<T>::copy_base_t;
};

template <typename T>
using move_base_t = move_base<T, std::is_trivially_move_constructible_v<T>>;

template <typename T, bool triv>
struct copy_assignable_base : move_base_t<T> {
  using move_base_t<T>::move_base_t;

  constexpr copy_assignable_base(const copy_assignable_base&) = default;
  constexpr copy_assignable_base(copy_assignable_base&&) = default;
  constexpr copy_assignable_base& operator=(copy_assignable_base&&) = default;

  constexpr copy_assignable_base& operator=(const copy_assignable_base& other) {
    if (this == &other) {
      return *this;
    }
    if (!other.is_present) {
      this->reset();
    } else if (!this->is_present) {
      this->init(other.is_present, other.data);
    } else {
      this->data = *other;
    }
    return *this;
  }
};

template <typename T>
struct copy_assignable_base<T, true> : move_base_t<T> {
  using move_base_t<T>::move_base_t;
};

template <typename T>
using copy_assignable_base_t =
    copy_assignable_base<T, std::is_trivially_copy_assignable_v<T> && std::is_trivially_copy_constructible_v<T> &&
                                std::is_trivially_destructible_v<T>>;

template <typename T, bool triv>
struct move_assignable_base : copy_assignable_base_t<T> {
  using copy_assignable_base_t<T>::copy_assignable_base_t;

  constexpr move_assignable_base(const move_assignable_base&) = default;
  constexpr move_assignable_base(move_assignable_base&&) = default;
  constexpr move_assignable_base& operator=(const move_assignable_base& other) = default;

  constexpr move_assignable_base& operator=(move_assignable_base&& other) {
    if (this == &other) {
      return *this;
    }
    if (!other.is_present) {
      this->reset();
    } else if (!this->is_present) {
      this->init(other.is_present, std::move(other.data));
    } else {
      this->data = std::move(*other);
    }
    return *this;
  }
};

template <typename T>
struct move_assignable_base<T, true> : copy_assignable_base_t<T> {
  using copy_assignable_base_t<T>::copy_assignable_base_t;
};

template <typename T>
using move_assignable_base_t =
    move_assignable_base<T, std::is_trivially_move_assignable_v<T> && std::is_trivially_move_constructible_v<T> &&
                                std::is_trivially_destructible_v<T>>;

template <bool enable>
struct copy_enable {
  copy_enable() = default;
  copy_enable(const copy_enable&) = delete;
  constexpr copy_enable(copy_enable&&) noexcept = default;
  constexpr copy_enable& operator=(const copy_enable& other) = delete;
  constexpr copy_enable& operator=(copy_enable&&) = default;
};

template <>
struct copy_enable<true> {};

template <bool enable>
struct move_enable {
  move_enable() = default;
  constexpr move_enable(const move_enable&) noexcept = default;
  constexpr move_enable(move_enable&&) noexcept = delete;
  constexpr move_enable& operator=(const move_enable& other) = default;
  constexpr move_enable& operator=(move_enable&&) = delete;
};

template <>
struct move_enable<true> {};

template <bool enable>
struct copy_assignable_enable {
  copy_assignable_enable() = default;
  copy_assignable_enable(const copy_assignable_enable&) = default;
  constexpr copy_assignable_enable(copy_assignable_enable&&) noexcept = default;
  constexpr copy_assignable_enable& operator=(const copy_assignable_enable& other) = delete;
  constexpr copy_assignable_enable& operator=(copy_assignable_enable&&) = default;
};

template <>
struct copy_assignable_enable<true> {};

template <bool enable>
struct move_assignable_enable {
  move_assignable_enable() = default;
  move_assignable_enable(const move_assignable_enable&) = default;
  constexpr move_assignable_enable(move_assignable_enable&&) noexcept = default;
  constexpr move_assignable_enable& operator=(const move_assignable_enable& other) = default;
  constexpr move_assignable_enable& operator=(move_assignable_enable&& other) = delete;
};

template <>
struct move_assignable_enable<true> {};

} // namespace tools

template <typename T>
class optional : tools::copy_enable<std::is_copy_constructible_v<T>>,
                 tools::move_enable<std::is_move_constructible_v<T>>,
                 tools::copy_assignable_enable<std::is_copy_assignable_v<T>>,
                 tools::move_assignable_enable<std::is_move_assignable_v<T>>,
                 public tools::move_assignable_base_t<T> {
private:
  using base = tools::move_assignable_base_t<T>;
  using base::data;
  using base::init;
  using base::is_present;

public:
  using base::base;

  constexpr optional() noexcept {}

  constexpr optional(const optional& other) = default;

  constexpr optional(optional&& other) = default;

  constexpr optional(nullopt_t) noexcept : optional() {}

  template <typename... Args>
  explicit constexpr optional(in_place_t, Args&&... args) {
    this->init(true, std::forward<Args>(args)...);
  }

  constexpr optional& operator=(const optional& other) = default;

  constexpr optional& operator=(optional&& other) = default;

  constexpr optional& operator=(nullopt_t) noexcept {
    this->reset();
    return *this;
  }

  constexpr void swap(optional& other) {
    if (!this->is_present && !other.is_present) {
      return;
    }
    if (this->is_present && other.is_present) {
      using std::swap;
      swap(**this, *other);
      return;
    }
    if (this->is_present) {
      other.swap(*this);
      return;
    }
    *this = std::move(other);
    other.reset();
  }

  friend constexpr void swap(optional& lhs, optional& rhs) {
    lhs.swap(rhs);
  }

  template <typename... Args>
  constexpr void emplace(Args&&... args) {
    this->reset();
    this->init(true, std::forward<Args>(args)...);
  }
};

template <typename T>
constexpr bool operator==(const optional<T>& a, const optional<T>& b) {
  return (!a && !b) || (a && b && *a == *b);
}

template <typename T>
constexpr bool operator!=(const optional<T>& a, const optional<T>& b) {
  return (a || b) && (!a || !b || *a != *b);
}

template <typename T>
constexpr bool operator<(const optional<T>& a, const optional<T>& b) {
  return (!a && b) || (b && *a < *b);
}

template <typename T>
constexpr bool operator>(const optional<T>& a, const optional<T>& b) {
  return (a && !b) || (a && *a > *b);
}

template <typename T>
constexpr bool operator<=(const optional<T>& a, const optional<T>& b) {
  return !a || (b && *a <= *b);
}

template <typename T>
constexpr bool operator>=(const optional<T>& a, const optional<T>& b) {
  return !b || (a && *a >= *b);
}
