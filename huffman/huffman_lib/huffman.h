//
// Created by Tedes on 03.06.2023.
//

#ifndef HUFFMAN_HUFFMAN_H
#define HUFFMAN_HUFFMAN_H

#include "utils/constants.h"

#include <istream>

namespace huffman {
void encode(std::istream& in, std::ostream& out);
void decode(std::istream& in, std::ostream& out);
} // namespace huffman
#endif // HUFFMAN_HUFFMAN_H
