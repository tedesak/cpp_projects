//
// Created by Tedes on 03.06.2023.
//

#ifndef HUFFMAN_BINARY_WRITER_H
#define HUFFMAN_BINARY_WRITER_H

#include "../utils/constants.h"

#include <iostream>

namespace huffman {
class binary_writer {
public:
  explicit binary_writer(std::ostream& in);
  binary_writer(const binary_writer&) = delete;
  binary_writer& operator=(const binary_writer&) = delete;
  ~binary_writer();

  void write_out(out_element el);
  void write(out_char_t el);
  void write(atom_char_t el);
  void write(tail_out el);

private:
  void write_to_stream();
  void push(out_char_t el);

private:
  std::ostream& out;
  std::string buf;
  tail_out tail;
  size_t buf_size;
};
} // namespace huffman
#endif // HUFFMAN_BINARY_WRITER_H
