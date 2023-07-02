//
// Created by Tedes on 03.06.2023.
//

#include "../huffman_lib/binary_io/binary_reader.h"
#include "../huffman_lib/binary_io/binary_writer.h"
#include "../huffman_lib/huffman.h"
#include "../huffman_lib/huffman_convert_tree/convert_tree.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <set>

const std::string path = ROOT_DIRECTORY;
const std::string source_dir = "dataset";

static const std::map<std::string, std::set<std::string>> source_file_names = {
    {                      "empty",                      {"empty"}},
    {                     "simple",                     {"simple"}},
    {"many_dif_letters_small_text",                  {"m_d_l_s_t"}},
    {   "few_dif_letters_big_text",                  {"f_d_l_b_t"}},
    {  "many_dif_letters_big_text", {"winter_night_pasternak.txt"}},
    {                  "huge_test",  {"HOM.jpg", "mountains.jpeg"}},
};

void ASSERT_FILE_EQ(const std::string& file1, const std::string& file2) {
  std::ifstream fin1(file1, std::ios::binary), fin2(file2, std::ios::binary);
  char tmp1 = 0, tmp2 = 0;
  while (!fin1.eof() && !fin2.eof()) {
    fin1 >> tmp1;
    fin2 >> tmp2;
    ASSERT_EQ(tmp1, tmp2);
  }
  fin1.close();
  fin2.close();
}

void encode_file(const std::string& in, const std::string& out) {
  std::fstream encode_fin(in, std::fstream::in | std::ios::binary);
  std::fstream encode_fout(out, std::fstream::out | std::ios::binary);
  huffman::encode(encode_fin, encode_fout);
  encode_fin.close();
  encode_fout.close();
}

void decode_file(const std::string& in, const std::string& out) {
  std::fstream decode_fin(in, std::fstream::in | std::ios::binary);
  std::fstream decode_fout(out, std::fstream::out | std::ios::binary);
  huffman::decode(decode_fin, decode_fout);
  decode_fin.close();
  decode_fout.close();
}

void encode_decode_test_source(const std::string& file_name) {
  std::string in = path + "/" + source_dir + "/" + file_name;
  std::string huf_zip = path + "/" + file_name + ".huf_zip";
  std::string enc_dec = path + "/" + file_name + ".huf_unzip";
  {
    std::fstream fin(in, std::fstream::in | std::ios::binary);
    if (fin.fail()) {
      return;
    }
    fin.close();
  }
  encode_file(in, huf_zip);
  decode_file(huf_zip, enc_dec);
  ASSERT_FILE_EQ(in, enc_dec);
  std::filesystem::remove(huf_zip);
  std::filesystem::remove(enc_dec);
}

void testing_with_source(const std::string& test_name) {
  for (const auto& file_name : source_file_names.at(test_name)) {
    encode_decode_test_source(file_name);
  }
}

TEST(correctness, empty) {
  testing_with_source("empty");
}

TEST(correctness, simple) {
  testing_with_source("simple");
}

TEST(correctness, many_dif_letters_small_text) {
  testing_with_source("many_dif_letters_small_text");
}

TEST(correctness, few_dif_letters_big_text) {
  testing_with_source("few_dif_letters_big_text");
}

TEST(correctness, many_dif_letters_big_text) {
  testing_with_source("many_dif_letters_big_text");
}

TEST(correctness, huge_test) {
  testing_with_source("huge_test");
}

// TEST(io_test, empty_name_encode) {
//   ASSERT_THROW(encode_file("", "empty"), std::runtime_error);
//   ASSERT_THROW(encode_file("empty", ""), std::runtime_error);
//   ASSERT_THROW(encode_file("", ""), std::runtime_error);
// }

TEST(io_test, empty_name_decode) {
  ASSERT_THROW(decode_file("", "empty"), std::runtime_error);
  ASSERT_THROW(decode_file("empty", ""), std::runtime_error);
  ASSERT_THROW(decode_file("", ""), std::runtime_error);
}

TEST(io_test, invalid_input_decode) {
  ASSERT_THROW(decode_file("empty", "empty.huf_unzip"), std::runtime_error);
}

TEST(reader_test, bad_stream_create) {
  std::fstream fin(path + "/" + source_dir + "/empty", std::fstream::in | std::ios::binary);
  while (fin.good()) {
    fin.get();
  }
  ASSERT_THROW(huffman::binary_reader reader(fin), std::runtime_error);
}

TEST(reader_test, bad_stream_read_tail_out) {
  std::fstream fin(path + "/" + source_dir + "/simple", std::fstream::in | std::ios::binary);
  if (fin.good()) {
    huffman::binary_reader reader(fin);
    huffman::tail_out tail;
    while (!reader.eof()) {
      reader.read(tail);
    }
    ASSERT_THROW(reader.read(tail), std::runtime_error);
  }
}

TEST(reader_test, bad_stream_read_out_element) {
  std::fstream fin(path + "/" + source_dir + "/simple", std::fstream::in | std::ios::binary);
  if (fin.good()) {
    huffman::binary_reader reader(fin);
    huffman::out_element out_el;
    while (!reader.eof()) {
      reader.read(out_el);
    }
    ASSERT_THROW(reader.read(out_el), std::runtime_error);
  }
}

TEST(writer_test, bad_stream_create) {
  std::fstream fout("tmp", std::ios::binary);
  while (fout.good()) {
    fout.get();
  }
  ASSERT_THROW(huffman::binary_reader reader(fout), std::runtime_error);
  std::filesystem::remove("tmp");
}

TEST(encode_converting, zero_value_freq) {
  std::vector<huffman::int_freq_t> freq(huffman::NUMBER_ATOM_CHARS, 0);
  ASSERT_THROW(huffman::convert_tree::get_encode_code_table(freq), std::runtime_error);
}

TEST(encode_converting, zero_size_freq) {
  std::vector<huffman::int_freq_t> freq(0);
  ASSERT_THROW(huffman::convert_tree::get_encode_code_table(freq), std::runtime_error);
}

TEST(decode_converting, zero_value_code_len) {
  std::vector<huffman::atom_char_t> code_len(huffman::NUMBER_ATOM_CHARS, 0);
  ASSERT_THROW(huffman::convert_tree conv_tree(code_len), std::runtime_error);
}

TEST(decode_converting, same_value_code_len) {
  for (std::size_t val = 1; val != huffman::NUMBER_ATOM_CHARS; ++val) {
    if (val == huffman::CHAR_SIZE) {
      continue;
    }
    std::vector<huffman::atom_char_t> code_len(huffman::NUMBER_ATOM_CHARS, val);
    ASSERT_THROW(huffman::convert_tree conv_tree(code_len), std::runtime_error);
  }
}
