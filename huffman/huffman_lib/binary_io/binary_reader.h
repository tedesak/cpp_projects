//
// Created by Tedes on 03.06.2023.
//

#ifndef HUFFMAN_BINARY_READER_H
#define HUFFMAN_BINARY_READER_H

#include "../utils/constants.h"

#include <iostream>

namespace huffman {
class binary_reader {
public:
  explicit binary_reader(std::istream& in);
  binary_reader(const binary_reader&) = delete;
  binary_reader& operator=(const binary_reader&) = delete;
  ~binary_reader();

  void read(out_element&);
  void read(tail_out&);
  bool eof();
  bool is_empty();

private:
  void update_buf();

private:
  std::basic_streambuf<char>* stream_buf;
  std::string buf;
  size_t pos;
  size_t buf_size;
};
} // namespace huffman
#endif // HUFFMAN_BINARY_READER_H
