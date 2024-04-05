//
// Created by Tedes on 03.06.2023.
//

#include "convert_tree.h"

#include <set>
#include <stdexcept>

namespace huffman {
convert_tree::base_node::base_node(base_node* left, base_node* right) : left(left), right(right) {}

convert_tree::base_node::~base_node() {
  if (!is_leaf()) {
    delete left;
    delete right;
  }
}

bool convert_tree::base_node::is_leaf() const {
  return left == nullptr && right == nullptr;
}

void convert_tree::base_node::get_len_code(std::size_t actual_len, std::vector<std::set<atom_char_t>>& len_code) {
  if (is_leaf()) {
    len_code[actual_len].insert(static_cast<node*>(this)->value);
  } else {
    actual_len++;
    left->get_len_code(actual_len, len_code);
    right->get_len_code(actual_len, len_code);
  }
}

convert_tree::node::node(atom_char_t value) : value(value), base_node(nullptr, nullptr) {}

convert_tree::node::~node() = default;

class convert_tree::freq_node_comparator {
public:
  bool operator()(std::pair<int_freq_t, base_node*>& a, std::pair<int_freq_t, base_node*>& b) {
    return a.first > b.first;
  }
};

void code_len_to_code_table(out_element actual_code, std::map<atom_char_t, out_element>* code_table,
                            std::map<out_element, atom_char_t, std::greater<>>* decode_table,
                            std::vector<std::set<atom_char_t>>& len_code) {
  size_t& len = actual_code[LEN_INDEX_OUT_EL];
  if (len >= OUT_CHAR_SIZE) {
    throw std::runtime_error("Invalid decoding file type");
  }
  if (len_code[len].size() != 0) {
    if (code_table != nullptr) {
      (*code_table)[*len_code[len].begin()] = actual_code;
    }
    if (decode_table != nullptr) {
      (*decode_table)[actual_code] = *len_code[len].begin();
    }
    len_code[len].erase(len_code[len].begin());
    return;
  }
  len++;
  code_len_to_code_table(actual_code, code_table, decode_table, len_code);
  actual_code[(len - 1) / OUT_CHAR_SIZE] += static_cast<unsigned long long>(1)
                                         << (OUT_CHAR_SIZE - (len % OUT_CHAR_SIZE));
  code_len_to_code_table(actual_code, code_table, decode_table, len_code);
}

convert_tree::convert_tree(std::vector<atom_char_t> code_len) {
  std::map<atom_char_t, out_element> code_table;
  std::vector<std::set<atom_char_t>> len_code(NUMBER_ATOM_CHARS);
  for (size_t i = 0; i != NUMBER_ATOM_CHARS; ++i) {
    len_code[code_len[i]].insert(i);
  }
  len_code[0].clear();
  code_len_to_code_table({}, nullptr, &decode_table, len_code);
  for (size_t i = 0; i != NUMBER_ATOM_CHARS; ++i) {
    if (!len_code[code_len[i]].empty()) {
      throw std::runtime_error("Invalid len_code");
    }
  }
}

atom_char_t convert_tree::decode(out_element& code_frag) {
  auto el = decode_table.lower_bound(code_frag);
  shift_out_element(el->first[LEN_INDEX_OUT_EL], code_frag);
  return el->second;
}

std::map<atom_char_t, out_element> convert_tree::get_encode_code_table(const std::vector<int_freq_t>& freq) {
  std::priority_queue<std::pair<int_freq_t, base_node*>, std::vector<std::pair<int_freq_t, base_node*>>,
                      convert_tree::freq_node_comparator>
      freq_node;
  try {
    for (size_t el = 0; el != freq.size(); ++el) {
      if (freq[el] != 0) {
        freq_node.emplace(freq[el], new node(el));
      }
    }
    while (freq_node.size() > 1) {
      auto p1 = freq_node.top();
      freq_node.pop();
      auto p2 = freq_node.top();
      freq_node.pop();
      freq_node.emplace(p1.first + p2.first, new base_node(p1.second, p2.second));
    }
  } catch (...) {
    while (!freq_node.empty()) {
      delete freq_node.top().second;
      freq_node.pop();
    }
    throw;
  }
  if (freq_node.empty()) {
    throw std::runtime_error("Invalid frequency");
  }
  auto root = freq_node.top().second;
  std::vector<std::set<atom_char_t>> len_code(NUMBER_ATOM_CHARS);
  root->get_len_code(0, len_code);
  delete root;
  std::map<atom_char_t, out_element> code_table;
  code_len_to_code_table({}, &code_table, nullptr, len_code);
  return code_table;
}
} // namespace huffman
