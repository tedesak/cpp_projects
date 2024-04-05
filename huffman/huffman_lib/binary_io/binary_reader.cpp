//
// Created by Tedes on 03.06.2023.
//

#include "binary_reader.h"

#include <algorithm>

namespace huffman {
binary_reader::binary_reader(std::istream& in) : stream_buf(in.rdbuf()), buf(BUF_SIZE, '0'), pos(0), buf_size(0) {
  if (!in.good()) {
    throw std::runtime_error("Broken file");
  }
  update_buf();
}

binary_reader::~binary_reader() {
  buf.clear();
}

bool binary_reader::is_empty() {
  return buf_size == 1;
}

void binary_reader::read(tail_out& el) {
  if (pos >= buf_size) {
    throw std::runtime_error("Find EOF");
  }
  std::reverse_copy(buf.data() + pos, buf.data() + pos + OUT_CHAR_SIZE / CHAR_SIZE, reinterpret_cast<char*>(&el.first));
  pos += OUT_CHAR_SIZE / CHAR_SIZE;
  if (pos == BUF_SIZE) {
    update_buf();
  }
  el.second = (pos + 1 == buf_size ? buf[pos++] : OUT_CHAR_SIZE);
}

void binary_reader::read(out_element& out_el) {
  if (pos >= buf_size) {
    throw std::runtime_error("Find EOF");
  }
  for (size_t i = 0; i != NUMBER_OUT_CHAR_IN_OUT_ELEMENT; ++i) {
    tail_out el;
    read(el);
    out_el[i] = el.first;
    out_el[LEN_INDEX_OUT_EL] += el.second;
    if (el.second != OUT_CHAR_SIZE || eof()) {
      return;
    }
  }
}

bool binary_reader::eof() {
  return buf_size == pos;
}

void binary_reader::update_buf() {
  buf_size = stream_buf->sgetn(buf.data(), BUF_SIZE);
  pos = 0;
}
} // namespace huffman
