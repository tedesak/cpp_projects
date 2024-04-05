#pragma once

#include <any>
#include <cstddef>
#include <exception>
#include <type_traits>
#include <utility>

template <typename F>
class function;

class bad_function_call : public std::exception {
public:
  explicit bad_function_call(const char* message) : message(message) {}

  const char* what() const noexcept override {
    return message;
  }

private:
  const char* message;
};

namespace tools {
using storage_t = std::aligned_storage_t<sizeof(std::max_align_t), alignof(std::max_align_t)>;

template <typename R, typename... Args>
class descriptor_t {
public:
  void (*destruct)(storage_t*);
  void (*copy_init)(storage_t*, const storage_t*);
  void (*move_init)(storage_t*, storage_t*);
  R (*call)(const storage_t*, Args...);
//  template <typename F>
//  static void (*init)(storage_t*, F*);
//  template <typename F>
//  static F* (*get_data)(storage_t*);

  template <typename F>
  static void init(storage_t*, F*) {

  }

  template <typename F>
  static F* get_data(storage_t*) {

  }
};

//template <typename R, typename... Args>
//class descriptor_t {
//public:
//  virtual void destruct(storage_t*) const noexcept = 0;
//  virtual void init(storage_t*, std::any) const = 0;
//  virtual void copy_init(storage_t*, const storage_t*) const = 0;
//  virtual void move_init(storage_t*, storage_t*) const noexcept = 0;
//  virtual R call(const storage_t*, Args...) const = 0;
//  virtual std::any get_data(storage_t*) const noexcept = 0;
//};

template <typename R, typename... Args>
class empty_desc_t {
public:
  static void destruct(storage_t*) noexcept {}

  static void init(storage_t*, storage_t *) {}

  static void copy_init(storage_t* dst, const storage_t* src) {
    *dst = *src;
  }

  static void move_init(storage_t* dst, storage_t* src) noexcept {
    *dst = *src;
  }

  static R call(const storage_t*, Args...) {
    throw bad_function_call("call empty function");
  }

  static storage_t* get_data(storage_t*) noexcept {
    return {};
  }

  constexpr static descriptor_t<R, Args...> get_desc() {
    return {&destruct, &copy_init, &move_init, &call};
  }
};

template <typename F, typename R, typename... Args>
class small_desc_t {
private:
  static F* get_func(storage_t* st) noexcept {
    return reinterpret_cast<F*>(st);
  }

  static const F* get_func(const storage_t* st) noexcept {
    return reinterpret_cast<const F*>(st);
  }

public:
  static void destruct(storage_t* st) noexcept {
    get_func(st)->~F();
  }

  static void init(storage_t* st, F* f) {
    new (st) F(std::move(*f));
  }

  static void copy_init(storage_t* dst, const storage_t* src) {
    new (dst) F(*get_func(src));
  }

  static void move_init(storage_t* dst, storage_t* src) noexcept {
    new (dst) F(std::move(*get_func(src)));
    get_func(src)->~F();
  }

  static R call(const storage_t* f, Args... args) {
    return (*const_cast<F*>(get_func(f)))(std::forward<Args>(args)...);
  }

  static F* get_data(storage_t* st) noexcept {
    return reinterpret_cast<F*>(st);
  }

  constexpr static descriptor_t<R, Args...> get_desc() {
//    descriptor_t<R, Args...>::template init<F> = &init;
//    descriptor_t<R, Args...>::template get_data<F> = &get_data;
    return {&destruct, &copy_init, &move_init, &call};
  }
};

template <typename F, typename R, typename... Args>
class big_desc_t {
private:
  static F*& get_func(storage_t* st) {
    return *reinterpret_cast<F**>(st);
  }

  static const F* get_func(const storage_t* st) {
    return *reinterpret_cast<F**>(const_cast<storage_t*>(st));
  }

public:
  static void destruct(storage_t* st) noexcept {
    delete get_func(st);
  }

  static void init(storage_t* st, F* f) {
    *reinterpret_cast<F**>(st) = new F(std::move(*f));
  }

  static void copy_init(storage_t* dst, const storage_t* src) {

    get_func(dst) = new F(*get_func(src));
  }

  static void move_init(storage_t* dst, storage_t* src) noexcept {
    *dst = *src;
  }

  static R call(const storage_t* f, Args... args) {
    return (*const_cast<F*>(get_func(f)))(std::forward<Args>(args)...);
  }

  static F* get_data(storage_t* st) noexcept {
    return *reinterpret_cast<F**>(st);
  }

  constexpr static descriptor_t<R, Args...> get_desc() {
    return {&destruct, &copy_init, &move_init, &call};
  }
};

} // namespace tools

template <typename R, typename... Args>
class function<R(Args...)> {
private:
  using descriptor_t = const tools::descriptor_t<R, Args...>;

//  static constexpr tools::empty_desc_t empty_desc = tools::empty_desc_t<R, Args...>();
//
//  template <typename F>
//  static constexpr tools::small_desc_t small_desc = tools::small_desc_t<F, R, Args...>();
//
//  template <typename F>
//  static constexpr tools::big_desc_t big_desc = tools::big_desc_t<F, R, Args...>();

  static constexpr descriptor_t empty_desc = tools::empty_desc_t<R, Args...>::get_desc();

  template <typename F>
  static constexpr descriptor_t small_desc = tools::small_desc_t<F, R, Args...>::get_desc();

  template <typename F>
  static constexpr descriptor_t big_desc = tools::big_desc_t<F, R, Args...>::get_desc();

  template <typename F>
  static constexpr bool is_small() noexcept {
    return sizeof(F) <= sizeof(std::max_align_t) && std::is_nothrow_move_constructible_v<F>;
  }

  template <typename F>
  static constexpr descriptor_t* desc_select() noexcept {
    return is_small<F>() ?
        static_cast<descriptor_t*>(&small_desc<F>) :
                         static_cast<descriptor_t*>(&big_desc<F>);
  }

public:
  function() noexcept : storage(), desc(&empty_desc) {}

  template <typename F>
  function(F func) : storage(),
                     desc(desc_select<F>()) {
    desc->template init<F>(&storage, &func);
  }

  function(const function& other) : storage(), desc(other.desc) {
    desc->copy_init(&storage, &other.storage);
  }

  function(function&& other) noexcept : storage(), desc(other.desc) {
    desc->move_init(&storage, &other.storage);
    other.desc = &empty_desc;
  }

  function& operator=(const function& other) {
    if (this != &other) {
      *this = function(other);
    }
    return *this;
  }

  function& operator=(function&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    desc->destruct(&storage);
    desc = std::exchange(other.desc, &empty_desc);
    desc->move_init(&storage, &other.storage);
    return *this;
  }

  ~function() {
    desc->destruct(&storage);
  }

  explicit operator bool() const noexcept {
    return desc != &empty_desc;
  }

  R operator()(Args... args) const {
    return desc->call(&storage, std::forward<Args>(args)...);
  }

  template <typename T>
  T* target() noexcept {
    T* data = desc->template get_data<T>(&storage);
    return data;
//    std::any data = desc->get_data(&storage);
//    return data.type() == typeid(T*) ? std::any_cast<T*>(data) : nullptr;
  }

  template <typename T>
  const T* target() const noexcept {
    T* data = desc->template get_data<T>(&storage);
    return data;
//    return data.type() == typeid(T*) ? std::any_cast<T*>(data) : nullptr;
  }

private:
  mutable tools::storage_t storage;
  descriptor_t* desc;
};
