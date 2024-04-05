#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <utility>

template <typename T>
class buffer {
public:
  std::size_t _capacity;
  std::size_t _ref_count;
  T _data[0];

  buffer(size_t capacity) : _capacity(capacity), _ref_count(1) {}

  void disconnect() {
    _ref_count--;
  }

  void connect() {
    _ref_count++;
  }

  bool is_shared() const {
    return _ref_count > 1;
  }
};

template <typename T, size_t SMALL_SIZE>
class socow_vector {
public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = pointer;
  using const_iterator = const_pointer;

private:
  std::size_t _size;
  bool is_small_obj;

  union {
    buffer<T>* _outer_data;
    T _inner_data[SMALL_SIZE];
  };

private:
  void validation() {
    if (!is_small_obj && _outer_data->is_shared()) {
      auto tmp = _outer_data;
      try {
        try {
          _outer_data = create_new_buffer(capacity());
          copy_from_begin(tmp->_data, _outer_data->_data, std::min(_size, tmp->_capacity));
        } catch (...) {
          operator delete(_outer_data);
          throw;
        }
      } catch (...) {
        _outer_data = tmp;
        throw;
      }
      tmp->disconnect();
    }
  }

  void disconnect() {
    if (!is_small_obj) {
      _outer_data->disconnect();
      if (_outer_data->_ref_count == 0) {
        remove(begin(), end());
        operator delete(_outer_data);
      }
    }
  }

  void copy_in_range(const T* src, T* dst, size_t start, size_t end) {
    size_t i = start;
    try {
      while (i < end) {
        new (dst + i) T(src[i]);
        ++i;
      }
    } catch (...) {
      remove(dst + start, dst + i);
      throw;
    }
  }

  void copy_from_begin(const T* src, T* dst, size_t count) {
    copy_in_range(src, dst, 0, count);
  }

  void remove(T* start, T* end) {
    if (start != nullptr) {
      ptrdiff_t count = end - start;
      for (ptrdiff_t i = count - 1; i >= 0; --i) {
        (start + i)->~T();
      }
    }
  }

  buffer<T>* create_new_buffer(size_t new_capacity) {
    return new (static_cast<buffer<T>*>(operator new(sizeof(buffer<T>) + new_capacity * sizeof(T))))
        buffer<T>(new_capacity);
  }

  socow_vector(const socow_vector& other, size_t capacity, size_t size) : _size(std::min(size, capacity)) {
    if (capacity <= SMALL_SIZE) {
      copy_from_begin(other.data(), _inner_data, std::min(size, capacity));
      is_small_obj = true;
    } else {
      try {
        _outer_data = create_new_buffer(capacity);
        copy_from_begin(other.data(), _outer_data->_data, std::min(size, capacity));
      } catch (...) {
        operator delete(_outer_data);
        throw;
      }
      is_small_obj = false;
    }
  }

  socow_vector(const socow_vector& other, size_t capacity) : socow_vector(other, capacity, other.size()) {}

public:
  socow_vector() noexcept : _size(0), is_small_obj(true) {} // O(1) nothrow

  socow_vector(const socow_vector& other) : _size(other._size), is_small_obj(other.is_small_obj) {
    if (other.is_small_obj) {
      copy_from_begin(other._inner_data, _inner_data, other._size);
    } else {
      _outer_data = other._outer_data;
      _outer_data->connect();
    }
  } // O(N) strong

  socow_vector& operator=(const socow_vector& other) { // O(N) strong
    if (&other == this) {
      return *this;
    }
    if (other.is_small_obj) {
      if (is_small_obj) {
        copy_in_range(other.data(), _inner_data, _size, other._size);
        size_t min_inner_size = std::min(other._size, _size);
        try {
          socow_vector tmp(other, min_inner_size);
          for (size_t i = 0; i < min_inner_size; ++i) {
            std::swap(_inner_data[i], tmp._inner_data[i]);
          }
        } catch (...) {
          for (size_t j = other._size; _size < j;) {
            _inner_data[--j].~T();
          }
          throw;
        }
        remove(_inner_data + other._size, _inner_data + _size);
      } else {
        auto tmp = _outer_data;
        try {
          copy_from_begin(other.data(), _inner_data, other._size);
        } catch (...) {
          _outer_data = tmp;
          throw;
        }
        tmp->disconnect();
        if (tmp->_ref_count == 0) {
          remove(tmp->_data, tmp->_data + _size);
          operator delete(tmp);
        }
      }
      is_small_obj = true;
    } else {
      if (is_small_obj) {
        remove(begin(), end());
        _size = 0;
      }
      disconnect();
      _outer_data = other._outer_data;
      _outer_data->connect();
      is_small_obj = false;
    }
    _size = other._size;
    return *this;
  }

  ~socow_vector() noexcept { // O(N) nothrow
    disconnect();
    if (is_small_obj) {
      remove(begin(), end());
    }
  }

  reference operator[](size_t index) { // O(1) nothrow
    return data()[index];
  }

  const_reference operator[](size_t index) const { // O(1) nothrow
    return data()[index];
  }

  pointer data() { // O(1) nothrow
    if (is_small_obj) {
      return _inner_data;
    } else {
      validation();
      return _outer_data->_data;
    }
  }

  const_pointer data() const noexcept { // O(1) nothrow
    return is_small_obj ? _inner_data : _outer_data->_data;
  }

  size_t size() const noexcept { // O(1) nothrow
    return _size;
  }

  bool empty() const noexcept { // O(1) nothrow
    return size() == 0;
  }

  size_t capacity() const noexcept { // O(1) nothrow
    return is_small_obj ? SMALL_SIZE : _outer_data->_capacity;
  }

  reference front() { // O(1) nothrow
    return data()[0];
  }

  const_reference front() const { // O(1) nothrow
    return data()[0];
  }

  reference back() { // O(1) nothrow
    return data()[_size - 1];
  }

  const_reference back() const { // O(1) nothrow
    return data()[_size - 1];
  }

  iterator begin() { // O(1) nothrow
    return data();
  }

  iterator end() { // O(1) nothrow
    return data() + _size;
  }

  const_iterator begin() const noexcept { // O(1) nothrow
    return data();
  }

  const_iterator end() const noexcept { // O(1) nothrow
    return data() + _size;
  }

  void reserve(size_t new_capacity) { // O(N) strong
    if (_size <= new_capacity && (new_capacity > capacity() || (!is_small_obj && _outer_data->is_shared()))) {
      if (new_capacity <= SMALL_SIZE) {
        auto tmp = _outer_data;
        try {
          copy_from_begin(std::as_const(*this).data(), _inner_data, _size);
        } catch (...) {
          _outer_data = tmp;
          throw;
        }
        tmp->disconnect();
        if (tmp->_ref_count == 0) {
          remove(tmp->_data, tmp->_data + _size);
          operator delete(tmp);
        }
        is_small_obj = true;
        return;
      }
      socow_vector tmp(*this, new_capacity);
      *this = tmp;
    }
  }

  void shrink_to_fit() { // O(N) strong
    if (is_small_obj || (_size == capacity() && !_outer_data->is_shared())) {
      return;
    }
    if (_size > SMALL_SIZE) {
      socow_vector tmp(*this, _size);
      *this = tmp;
    } else {
      buffer<T>* tmp = _outer_data;
      _outer_data = nullptr;
      try {
        copy_from_begin(tmp->_data, _inner_data, _size);
      } catch (...) {
        _outer_data = tmp;
        throw;
      }
      tmp->disconnect();
      if (tmp->_ref_count == 0) {
        remove(tmp->_data, tmp->_data + _size);
        operator delete(tmp);
      }
      is_small_obj = true;
    }
  }

  void clear() noexcept { // O(N) nothrow
    if (!is_small_obj && _outer_data->is_shared()) {
      disconnect();
      _size = 0;
      is_small_obj = true;
      return;
    }
    erase(begin(), end());
  }

  void swap(socow_vector& other) {
    if (&other == this) {
      return;
    }
    if (_size > other._size || (!is_small_obj && other.is_small_obj)) {
      other.swap(*this);
      return;
    }
    if (is_small_obj && other.is_small_obj) {
      copy_in_range(other._inner_data, _inner_data, _size, other._size);
      try {
        for (size_t i = 0; i < _size; ++i) {
          std::swap(_inner_data[i], other._inner_data[i]);
        }
      } catch (...) {
        remove(_inner_data + _size, _inner_data + other._size);
        throw;
      }
      remove(other.begin() + _size, other.end());
    } else if (!is_small_obj && !other.is_small_obj) {
      std::swap(_outer_data, other._outer_data);
    } else {
      buffer<T>* tmp = other._outer_data;
      other._outer_data = nullptr;
      try {
        copy_from_begin(_inner_data, other._inner_data, _size);
      } catch (...) {
        other._outer_data = tmp;
        throw;
      }
      remove(begin(), end());
      _outer_data = tmp;
    }
    std::swap(_size, other._size);
    std::swap(is_small_obj, other.is_small_obj);
  }

  void push_back(const T& value) { // O(1)* strong
    if (capacity() == _size || (!is_small_obj && _outer_data->is_shared())) {
      socow_vector tmp(*this, capacity() == 0 ? 1 : capacity() * 2);
      tmp.push_back(value);
      *this = tmp;
    } else {
      new (data() + size()) T(value);
      _size++;
    }
  }

  void pop_back() { // O(1) nothrow
    if (!is_small_obj && _outer_data->is_shared()) {
      socow_vector tmp(*this, capacity(), _size - 1);
      swap(tmp);
    } else {
      data()[--_size].~T();
    }
  }

  iterator insert(const_iterator pos, const T& value) { // O(N) strong
    size_t shift = pos - std::as_const(*this).begin();
    if (capacity() == _size || (!is_small_obj && _outer_data->is_shared())) {
      auto actual_data = is_small_obj ? _inner_data : _outer_data->_data;
      auto _new_outer_data = create_new_buffer(capacity() == 0 ? 1 : capacity() * 2);
      try {
        copy_from_begin(actual_data, _new_outer_data->_data, shift);
        try {
          new (_new_outer_data->_data + shift) T(value);
          try {
            copy_in_range(actual_data, _new_outer_data->_data + 1, shift, _size);
          } catch (...) {
            (_new_outer_data->_data + shift)->~T();
            throw;
          }
        } catch (...) {
          remove(_new_outer_data->_data, _new_outer_data->_data + shift);
          throw;
        }
      } catch (...) {
        operator delete(_new_outer_data);
        throw;
      }
      disconnect();
      if (is_small_obj) {
        remove(begin(), end());
      }
      _outer_data = _new_outer_data;
      _size++;
      is_small_obj = false;
      return begin() + shift;
    }
    push_back(value);
    for (iterator it = end() - 1; it != begin() + shift; it--) {
      std::iter_swap(it, it - 1);
    }
    return begin() + shift;
  }

  iterator erase(const_iterator pos) { // O(N) nothrow(swap)
    return erase(pos, pos + 1);
  }

  iterator erase(const_iterator first, const_iterator last) { // O(N) nothrow(swap)
    ptrdiff_t shift_first = first - std::as_const(*this).begin(), shift = last - first;
    if (first == last) {
      return const_cast<iterator>(first);
    }
    if (!is_small_obj && _outer_data->is_shared()) {
      auto _new_outer_data = create_new_buffer(capacity() == 0 ? 1 : capacity() * 2);
      try {
        copy_from_begin(_outer_data->_data, _new_outer_data->_data, shift_first);
        try {
          copy_in_range(_outer_data->_data + shift, _new_outer_data->_data, shift_first, _size - shift);
        } catch (...) {
          remove(_new_outer_data->_data, _new_outer_data->_data + shift_first);
          throw;
        }
      } catch (...) {
        operator delete(_new_outer_data);
        throw;
      }
      disconnect();
      _outer_data = _new_outer_data;
      _size -= shift;
      return begin() + shift_first;
    }
    for (iterator it = begin() + shift_first + shift; it != end(); it++) {
      std::swap(*it, *(it - shift));
    }
    for (size_t i = 0; i != shift; ++i) {
      pop_back();
    }
    return begin() + shift_first;
  }
};
