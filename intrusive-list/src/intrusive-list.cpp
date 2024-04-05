#include "intrusive-list.h"

using namespace intrusive::tools;

list_element_base::list_element_base() noexcept : prev(this), next(this) {}

list_element_base::list_element_base(const list_element_base&) noexcept : list_element_base() {}

list_element_base& list_element_base::operator=(const list_element_base& other) noexcept {
  if (this == &other) {
    return *this;
  }
  unlink();
  return *this;
}

list_element_base::list_element_base(list_element_base&& other) noexcept {
  change_pointers(other);
}

list_element_base& list_element_base::operator=(list_element_base&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  unlink();
  change_pointers(other);
  return *this;
}

void list_element_base::change_pointers(list_element_base& other) {
  if (other.prev == &other) {
    prev = this;
    next = this;
    return;
  }
  prev = std::exchange(other.prev, &other);
  next = std::exchange(other.next, &other);
  prev->next = this;
  next->prev = this;
}

void list_element_base::unlink() noexcept {
  prev->next = next;
  next->prev = prev;
  prev = next = this;
}

void list_element_base::clear_pointers() noexcept {
  prev = next = this;
}

list_element_base::~list_element_base() noexcept {
  unlink();
}

list_element_base* list_element_base::get_prev() const {
  return prev;
}

list_element_base* list_element_base::get_next() const {
  return next;
}
