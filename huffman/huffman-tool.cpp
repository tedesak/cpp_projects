//
// Created by Tedes on 03.06.2023.
//

#include "huffman.h"
#include "utils/constants.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

static const std::map<std::string, size_t> flags_arg = {
    {  "compress", 0},
    {"decompress", 0},
    {     "input", 1},
    {    "output", 1},
    {      "help", 0}
};

static int handle_error(std::string message) {
  std::cerr << message << std::endl;
  return 1;
}

int main(int argc, char** argv) {
  std::map<std::string, std::vector<std::string>> flags;
  for (size_t i = 1; i < argc; ++i) {
    std::string flag = argv[i];
    if (flag.length() < 2 || flag[0] != '-' || flag[1] != '-') {
      return handle_error("Unknown flag: " + flag);
    }
    flag = flag.substr(2);
    if (flags_arg.count(flag) == 0) {
      return handle_error("Unknown flag: " + flag);
    }
    flags[flag] = {};
    for (size_t j = 0; j != flags_arg.at(flag); ++j) {
      i++;
      flags[flag].emplace_back(argv[i]);
    }
  }
  if (flags.count("help") == 1) {
    std::cout << "Usage: huffman-tool\n"
              << "This util can compress and decompress files by using Huffman algorithm\n"
              << "FLAGS:\n"
              << "--help                more information\n"
              << "--compress            encode file\n"
              << "--decompress          decode file\n"
              << "--input FILE_IN       input file\n"
              << "--output FILE_OUT     output file\n";
    return 0;
  }
  if (flags.count("input") == 0) {
    return handle_error("Specify input file");
  }
  if (flags.count("output") == 0) {
    return handle_error("Specify output file");
  }
  // if (flags["input"] == flags["output"]) {
  //   return handle_error("Same input and output file");
  // }
  if (flags.count("compress") == flags.count("decompress")) {
    return handle_error("Specify working mode");
  }
  std::ifstream fin(flags["input"].front(), std::ios::binary);
  std::ofstream fout(flags["output"].front(), std::ios::binary);
  if (fin.fail()) {
    return handle_error("Input file open error: " + std::string(strerror(errno)));
  }
  if (fout.fail()) {
    return handle_error("Output file open error: " + std::string(strerror(errno)));
  }
  try {
    if (flags.count("compress") == 1) {
      huffman::encode(fin, fout);
    } else {
      huffman::decode(fin, fout);
    }
  } catch (std::runtime_error& error) {
    std::string mode = flags.count("compress") == 1 ? "Encoding" : "Decoding";
    return handle_error(mode + " failed: " + std::string(error.what()));
  }
}
