//
// Created by Tedes on 03.06.2023.
//

#include "binary_writer.h"

#include <algorithm>

namespace huffman {
binary_writer::binary_writer(std::ostream& out) : out(out), buf(BUF_SIZE, '0'), tail({0, 0}), buf_size(0) {
  if (out.fail()) {
    throw std::runtime_error("Writing error");
  }
}

binary_writer::~binary_writer() {
  if (tail.second != 0) {
    push(tail.first);
  } else {
    tail.second = OUT_CHAR_SIZE;
  }
  push(static_cast<out_char_t>(tail.second) * ATOM_CHAR_TO_OUT_FACTOR);
  buf_size -= OUT_CHAR_SIZE / CHAR_SIZE - 1;
  write_to_stream();
}

void binary_writer::write_out(out_element el) {
  for (size_t i = 0; i != (el[LEN_INDEX_OUT_EL] - 1) / OUT_CHAR_SIZE; ++i) {
    write({el[i], OUT_CHAR_SIZE});
  }
  if (el[LEN_INDEX_OUT_EL] % OUT_CHAR_SIZE != 0) {
    write({el[el[LEN_INDEX_OUT_EL] / OUT_CHAR_SIZE], el[LEN_INDEX_OUT_EL] % OUT_CHAR_SIZE});
  }
}

void binary_writer::write(out_char_t el) {
  write({el, OUT_CHAR_SIZE});
}

void binary_writer::write(atom_char_t el) {
  write({static_cast<out_char_t>(el) * ATOM_CHAR_TO_OUT_FACTOR, ATOM_CHAR_SIZE});
}

void binary_writer::write(tail_out add_tail) {
  if (tail.second + add_tail.second > OUT_CHAR_SIZE) {
    out_char_t part_add_tail = add_tail.first >> tail.second;
    push(tail.first + part_add_tail);
    tail = {add_tail.first << (OUT_CHAR_SIZE - tail.second), tail.second + add_tail.second - OUT_CHAR_SIZE};
  } else {
    add_tail.first >>= tail.second;
    tail.first += add_tail.first;
    tail.second += add_tail.second;
    if (tail.second == OUT_CHAR_SIZE) {
      push(tail.first);
      tail = {0, 0};
    }
  }
  if (buf_size == BUF_SIZE) {
    write_to_stream();
  }
}

void binary_writer::push(out_char_t el) {
  std::reverse_copy(reinterpret_cast<char*>(&el), reinterpret_cast<char*>(&el) + OUT_CHAR_SIZE / ATOM_CHAR_SIZE,
                    buf.data() + buf_size);
  buf_size += OUT_CHAR_SIZE / ATOM_CHAR_SIZE;
}

void binary_writer::write_to_stream() {
  out.write(buf.data(), buf_size);
  buf_size = 0;
}
} // namespace huffman
