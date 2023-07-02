//
// Created by Tedes on 03.06.2023.
//

#ifndef HUFFMAN_CONVERT_TREE_H
#define HUFFMAN_CONVERT_TREE_H

#include "../utils/constants.h"

#include <queue>

#include <map>
#include <set>
#include <vector>

namespace huffman {
class convert_tree {
private:
  class base_node {
  public:
    base_node(base_node* left, base_node* right);
    virtual ~base_node();

    bool is_leaf() const;
    void get_len_code(std::size_t actual_len, std::vector<std::set<atom_char_t>>& len_code);

  private:
    base_node* left;
    base_node* right;
  };

  class node : public base_node {
  public:
    explicit node(atom_char_t value);
    ~node();

  public:
    atom_char_t value;
  };

public:
  explicit convert_tree(std::vector<atom_char_t> code_len);

  atom_char_t decode(out_element& code_frag);
  static std::map<atom_char_t, out_element> get_encode_code_table(const std::vector<int_freq_t>& freq);

private:
  class freq_node_comparator;
  std::map<out_element, atom_char_t, std::greater<>> decode_table;
};
} // namespace huffman
#endif // HUFFMAN_CONVERT_TREE_H
