#pragma once

#include <algorithm>
#include <any>
#include <cstddef>
#include <memory>
#include <utility>

template <typename T>
class shared_ptr;
template <typename T>
class weak_ptr;

namespace detail {
struct control_block_t {
  template <typename U>
  friend class ::shared_ptr;

  template <typename U>
  friend class ::weak_ptr;

  virtual std::any get_data() = 0;

protected:
  virtual void clear() = 0;
  virtual ~control_block_t() = default;

  void check_lifetime() {
    if (strong_ref_cnt + weak_ref_cnt == 0) {
      delete this;
    }
  }

  void add_strong_ref() {
    ++strong_ref_cnt;
  }

  void rem_strong_ref() {
    if (--strong_ref_cnt == 0) {
      clear();
    }
    check_lifetime();
  }

  void add_weak_ref() {
    ++weak_ref_cnt;
  }

  void rem_weak_ref() {
    --weak_ref_cnt;
    check_lifetime();
  }

  size_t get_count_strong_ref() const {
    return strong_ref_cnt;
  }

  bool is_destroyed() const {
    return strong_ref_cnt == 0;
  }

private:
  size_t strong_ref_cnt{1};
  size_t weak_ref_cnt{};
};

template <typename T, typename D = std::default_delete<T>>
struct control_block_ptr_t : control_block_t {

  control_block_ptr_t(T* _ptr, D&& del) : ptr(_ptr), deleter(std::move(del)) {}

private:
  void clear() override {
    deleter(ptr);
  }

  std::any get_data() override {
    return ptr;
  }

private:
  T* ptr;
  [[no_unique_address]] D deleter;
};

template <typename T>
struct control_block_obj_t : control_block_t {

  template <typename... Args>
  explicit control_block_obj_t(Args&&... args) {
    new (&data) T(std::forward<Args>(args)...);
  }

private:
  void clear() override {
    std::launder(reinterpret_cast<T*>(data))->~T();
  }

  std::any get_data() override {
    return std::launder(reinterpret_cast<T*>(data));
  }

private:
  alignas(T) std::byte data[sizeof(T)];
};

} // namespace detail

template <typename T>
class shared_ptr {
private:
  template <typename U>
  friend class weak_ptr;

  template <typename U>
  friend class shared_ptr;

  void cb_add_strong_ref() {
    if (_cb) {
      _cb->add_strong_ref();
    }
  }

  shared_ptr(detail::control_block_t* cb, T* ptr) : _cb(cb), _ptr(ptr) {
    cb_add_strong_ref();
  }

public:
  shared_ptr() noexcept : _cb(nullptr), _ptr(nullptr) {}

  shared_ptr(std::nullptr_t) noexcept : shared_ptr() {}

  template <typename Y>
  explicit shared_ptr(Y* ptr) : shared_ptr(ptr, std::default_delete<Y>()) {}

  template <typename Y, typename Deleter>
  shared_ptr(Y* ptr, Deleter&& deleter) : _ptr(ptr) {
    try {
      _cb = new detail::control_block_ptr_t(ptr, std::forward<Deleter>(deleter));
    } catch (...) {
      deleter(ptr);
      throw;
    }
  }

  shared_ptr(const shared_ptr& other) noexcept : shared_ptr(other, other.get()) {}

  template <typename Y>
  shared_ptr(const shared_ptr<Y>& other) noexcept : shared_ptr(other, other.get()) {}

  template <typename Y>
  shared_ptr(shared_ptr<Y>&& other) noexcept : shared_ptr(std::move(other), other.get()) {}

  template <typename Y>
  shared_ptr(const shared_ptr<Y>& other, T* ptr) noexcept : _cb(other._cb),
                                                            _ptr(ptr) {
    cb_add_strong_ref();
  }

  template <typename Y>
  shared_ptr(shared_ptr<Y>&& other, T* ptr) noexcept : _cb(std::exchange(other._cb, nullptr)),
                                                       _ptr(ptr) {
    other._ptr = nullptr;
  }

  ~shared_ptr() {
    reset();
  }

  shared_ptr& operator=(const shared_ptr& other) noexcept {
    return operator=<T>(other);
  }

  template <typename Y>
  shared_ptr& operator=(const shared_ptr<Y>& other) noexcept {
    shared_ptr<T>(other).swap(*this);
    return *this;
  }

  template <typename Y>
  shared_ptr& operator=(shared_ptr<Y>&& other) noexcept {
    shared_ptr<T>(std::move(other)).swap(*this);
    return *this;
  }

  T* get() const noexcept {
    return _ptr;
  }

  operator bool() const noexcept {
    return get() != nullptr;
  }

  T& operator*() const noexcept {
    return *get();
  }

  T* operator->() const noexcept {
    return get();
  }

  std::size_t use_count() const noexcept {
    return _cb ? _cb->get_count_strong_ref() : 0;
  }

  void reset() noexcept {
    if (_cb) {
      _cb->rem_strong_ref();
      _cb = nullptr;
      _ptr = nullptr;
    }
  }

  template <typename Y>
  void reset(Y* new_ptr) {
    shared_ptr<T>(new_ptr).swap(*this);
  }

  template <typename Y, typename Deleter>
  void reset(Y* new_ptr, Deleter&& deleter) {
    shared_ptr<T>(new_ptr, std::move(deleter)).swap(*this);
  }

  template <typename U>
  friend bool operator==(const shared_ptr& lhs, const shared_ptr<U>& rhs) noexcept {
    return lhs.get() == rhs.get();
  }

  template <typename U>
  friend bool operator!=(const shared_ptr& lhs, const shared_ptr<U>& rhs) noexcept {
    return !(lhs == rhs);
  }

  void swap(shared_ptr<T>& other) {
    std::swap(_cb, other._cb);
    std::swap(_ptr, other._ptr);
  }

  template <typename U, typename... Args>
  friend shared_ptr<U> make_shared(Args&&... args);

private:
  detail::control_block_t* _cb;
  T* _ptr;
};

template <typename T>
class weak_ptr {
private:
  template <typename U>
  friend class weak_ptr;

  void cb_add_weak_ref() {
    if (_cb) {
      _cb->add_weak_ref();
    }
  }

public:
  weak_ptr() noexcept : _cb(nullptr), _ptr(nullptr) {}

  template <typename Y>
  weak_ptr(const shared_ptr<Y>& other) noexcept : _cb(other._cb),
                                                  _ptr(other._ptr) {
    cb_add_weak_ref();
  }

  weak_ptr(const weak_ptr& other) noexcept : _cb(other._cb), _ptr(other._ptr) {
    cb_add_weak_ref();
  }

  template <typename Y>
  weak_ptr(const weak_ptr<Y>& other) noexcept : _cb(other._cb),
                                                _ptr(other._ptr) {
    cb_add_weak_ref();
  }

  template <typename Y>
  weak_ptr(weak_ptr<Y>&& other) noexcept
      : _cb(std::exchange(other._cb, nullptr)),
        _ptr(std::exchange(other._ptr, nullptr)) {}

  ~weak_ptr() {
    reset();
  }

  template <typename Y>
  weak_ptr& operator=(const shared_ptr<Y>& other) noexcept {
    weak_ptr<T>(other).swap(*this);
    return *this;
  }

  weak_ptr& operator=(const weak_ptr& other) noexcept {
    return operator=<T>(other);
  }

  template <typename Y>
  weak_ptr& operator=(const weak_ptr<Y>& other) noexcept {
    weak_ptr<T>(other).swap(*this);
    return *this;
  }

  template <typename Y>
  weak_ptr& operator=(weak_ptr<Y>&& other) noexcept {
    weak_ptr<T>(std::move(other)).swap(*this);
    return *this;
  }

  shared_ptr<T> lock() const noexcept {
    if (!_cb || _cb->is_destroyed()) {
      return shared_ptr<T>();
    } else {
      return shared_ptr<T>(_cb, _ptr);
    }
  }

  void reset() noexcept {
    if (_cb) {
      _cb->rem_weak_ref();
      _cb = nullptr;
      _ptr = nullptr;
    }
  }

  void swap(weak_ptr<T>& other) {
    std::swap(_cb, other._cb);
    std::swap(_ptr, other._ptr);
  }

private:
  detail::control_block_t* _cb;
  T* _ptr;
};

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  auto* _cb = static_cast<detail::control_block_t*>(new detail::control_block_obj_t<T>(std::forward<Args>(args)...));
  shared_ptr<T> ans;
  ans._cb = _cb;
  ans._ptr = std::any_cast<T*>(_cb->get_data());
  return ans;
}
