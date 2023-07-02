//
// Created by Tedes on 03.06.2023.
//

#include "huffman.h"

#include "binary_io/binary_reader.h"
#include "binary_io/binary_writer.h"
#include "huffman_convert_tree/convert_tree.h"

#include <array>

namespace huffman {
std::vector<int_freq_t> count_freq(std::istream& in) {
  std::vector<int_freq_t> freq(NUMBER_ATOM_CHARS, 0);
  std::array<unsigned char, BUF_SIZE> buf{};
  size_t count_dif = 0;
  std::streamsize buf_size;
  do {
    buf_size = in.readsome(reinterpret_cast<char*>(buf.data()), BUF_SIZE);
    // if (in.fail()) {
    //   throw std::runtime_error("Reading failed");
    // }
    for (size_t i = 0; i < buf_size; i++) {
      freq[buf[i]]++;
      if (freq[buf[i]] == 1) {
        count_dif++;
      }
    }
  } while (buf_size != 0);
  for (size_t i = 0; count_dif <= 1; ++i) {
    if (freq[i] == 0) {
      freq[i]++;
      count_dif++;
    }
  }
  return freq;
}

void write_code_len(std::map<atom_char_t, out_element>& code_table, binary_writer& writer) {
  for (size_t el = 0; el != NUMBER_ATOM_CHARS; ++el) {
    if (code_table.count(el) != 0) {
      writer.write(static_cast<atom_char_t>(code_table[el][LEN_INDEX_OUT_EL]));
    } else {
      writer.write(static_cast<atom_char_t>(0));
    }
  }
}

void encode(std::istream& in, std::ostream& out) {
  std::map<atom_char_t, out_element> code_table = convert_tree::get_encode_code_table(count_freq(in));
  in.seekg(0);
  binary_writer writer(out);
  write_code_len(code_table, writer);
  auto stream_buf = in.rdbuf();
  std::array<unsigned char, BUF_SIZE> buf{};
  for (std::streamsize buf_size = 1; buf_size != 0;) {
    buf_size = stream_buf->sgetn(reinterpret_cast<char*>(buf.data()), BUF_SIZE);
    for (size_t i = 0; i < buf_size; i++) {
      writer.write_out(code_table[buf[i]]);
    }
  }
}

std::vector<atom_char_t> read_code_len(std::istream& in) {
  auto stream_buf = in.rdbuf();
  std::vector<atom_char_t> code_len(NUMBER_ATOM_CHARS, 0);
  stream_buf->sgetn(reinterpret_cast<char*>(code_len.data()), NUMBER_ATOM_CHARS);
  return code_len;
}

void merge_tail(tail_out& tail1, tail_out& tail2) {
  if (tail2.second == 0) {
    return;
  }
  if (tail1.second + tail2.second > OUT_CHAR_SIZE) {
    tail1.first += tail2.first >> tail1.second;
    tail2.first <<= OUT_CHAR_SIZE - tail1.second;
    tail2.second = tail1.second + tail2.second - OUT_CHAR_SIZE;
    tail1.second = OUT_CHAR_SIZE;
  } else {
    tail1.first += tail2.first >> tail1.second;
    tail1.second += tail2.second;
    tail2 = {0, 0};
  }
}

void merge_out_el(out_element& out_el1, out_element& out_el2) {
  size_t i = 0;
  tail_out tail1, tail2;
  std::size_t old_size_out_el2 = out_el2[LEN_INDEX_OUT_EL];
  while (out_el2[LEN_INDEX_OUT_EL] != 0 && out_el1[LEN_INDEX_OUT_EL] != NUMBER_ATOM_CHARS) {
    tail1 = {out_el1[out_el1[LEN_INDEX_OUT_EL] / OUT_CHAR_SIZE], out_el1[LEN_INDEX_OUT_EL] % OUT_CHAR_SIZE};
    tail2 = {out_el2[i], (out_el2[LEN_INDEX_OUT_EL] > OUT_CHAR_SIZE ? OUT_CHAR_SIZE : out_el2[LEN_INDEX_OUT_EL])};
    std::size_t old_size_tail1 = tail1.second;
    merge_tail(tail1, tail2);
    out_el1[out_el1[LEN_INDEX_OUT_EL] / OUT_CHAR_SIZE] = tail1.first;
    out_el2[i] = tail2.first;
    out_el1[LEN_INDEX_OUT_EL] += tail1.second - old_size_tail1;
    out_el2[LEN_INDEX_OUT_EL] += old_size_tail1 - tail1.second;
    i++;
  }
  if (i == 0) {
    return;
  }
  std::size_t shift = old_size_out_el2 - out_el2[LEN_INDEX_OUT_EL];
  out_el2[i - 1] >>= shift % OUT_CHAR_SIZE;
  out_el2[LEN_INDEX_OUT_EL] = old_size_out_el2;
  shift_out_element(shift, out_el2);
}

void decode(std::istream& in, std::ostream& out) {
  convert_tree convert_tree(read_code_len(in));
  binary_reader reader(in);
  if (reader.is_empty()) {
    return;
  }
  std::string buf;
  buf.reserve(BUF_SIZE);
  std::size_t buf_ind = 0;
  out_element out_el1 = {}, out_el2 = {};
  while (!reader.eof() || out_el1[LEN_INDEX_OUT_EL] != 0) {
    if (out_el1[LEN_INDEX_OUT_EL] != NUMBER_ATOM_CHARS && !reader.eof()) {
      reader.read(out_el2);
      merge_out_el(out_el1, out_el2);
    }
    buf[buf_ind++] = convert_tree.decode(out_el1);
    if (buf_ind == BUF_SIZE) {
      out.write(buf.data(), BUF_SIZE);
      buf_ind = 0;
    }
    merge_out_el(out_el1, out_el2);
  }
  out.write(buf.data(), buf_ind);
}
} // namespace huffman
