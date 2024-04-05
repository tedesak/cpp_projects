#pragma once

#include <functional>

namespace signals {

template <typename T>
struct signal;

template <typename... Args>
struct signal<void(Args...)> {
  struct connection {
    void disconnect() noexcept;
  };

  signal();

  signal(const signal&) = delete;
  signal& operator=(const signal&) = delete;

  ~signal();

  connection connect(std::function<void(Args...)> slot) noexcept;

  void operator()(Args...) const;
};

} // namespace signals
