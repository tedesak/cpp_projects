//
// Created by Tedes on 03.06.2023.
//

#ifndef HUFFMAN_CONSTANTS_H
#define HUFFMAN_CONSTANTS_H

#include <array>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace huffman {
constexpr std::size_t NUMBER_OUT_CHAR_IN_OUT_ELEMENT = 4;
constexpr std::size_t LEN_INDEX_OUT_EL = NUMBER_OUT_CHAR_IN_OUT_ELEMENT;

using atom_char_t = unsigned char;
using out_char_t = uint64_t;
using int_freq_t = uint64_t;
using out_element = std::array<out_char_t, NUMBER_OUT_CHAR_IN_OUT_ELEMENT + 1>;
using tail_out = std::pair<out_char_t, std::size_t>;

constexpr std::size_t MAX_ATOM_CHAR = std::numeric_limits<atom_char_t>::max();
constexpr std::size_t MAX_OUT_CHAR = std::numeric_limits<out_char_t>::max();
constexpr std::size_t NUMBER_ATOM_CHARS = MAX_ATOM_CHAR + 1;
constexpr std::size_t OUT_CHAR_SIZE = std::numeric_limits<out_char_t>::digits;
constexpr std::size_t ATOM_CHAR_SIZE = std::numeric_limits<atom_char_t>::digits;
constexpr std::size_t BUF_SIZE = 32768;
constexpr std::size_t CHAR_SIZE = std::numeric_limits<char>::digits + 1;
constexpr std::size_t ATOM_CHAR_TO_OUT_FACTOR = (MAX_OUT_CHAR >> ATOM_CHAR_SIZE) + 1;

static void shift_out_element(std::size_t shift, out_element& el) {
  el[4] -= shift;
  for (std::size_t i = 0; i != 4; ++i) {
    if (i + shift / OUT_CHAR_SIZE < 4) {
      el[i] = el[i + shift / OUT_CHAR_SIZE];
      if (i > 0 && (shift % OUT_CHAR_SIZE) != 0) {
        el[i - 1] += el[i] >> (OUT_CHAR_SIZE - (shift % OUT_CHAR_SIZE));
      }
      el[i] <<= shift % OUT_CHAR_SIZE;
    } else {
      el[i] = 0;
    }
  }
}
} // namespace huffman
#endif // HUFFMAN_CONSTANTS_H
