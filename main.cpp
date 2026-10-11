#include "btree.hpp"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

bool test_insertions(const char *name, const std::vector<int> &keys) {
  bt::bplustree tree;
  std::vector<int> inserted;

  if (!tree.validate(std::cerr)) {
    std::cerr << name << " FAILED: invalid empty tree\n";
    return false;
  }

  for (int key : keys) {
    tree.insert(key, key + 1);
    inserted.push_back(key);

    if (!tree.validate(std::cerr)) {
      std::cerr << name << " FAILED structure check after inserting " << key
                << " (insertion " << inserted.size() << ")\n";
      return false;
    }

    // Check every stored key after each insertion to catch split errors.
    for (int stored_key : inserted) {
      auto actual = tree.get(stored_key);
      if (!actual || *actual != stored_key + 1) {
        std::cerr << name << " FAILED after inserting " << key
                  << " (insertion " << inserted.size() << "): get("
                  << stored_key << ") returned ";
        if (actual) {
          std::cerr << *actual;
        } else {
          std::cerr << "nullopt";
        }
        std::cerr << ", expected " << stored_key + 1 << '\n';
        return false;
      }
    }
  }

  std::cout << name << " PASSED (" << keys.size()
            << " keys; values and structure checked after every insertion)\n";
  return true;
}

int main() {
  // Missing keys must be distinguishable from valid zero and negative values.
  bt::bplustree optional_tree;
  if (optional_tree.get(7).has_value()) {
    std::cerr << "Empty-tree lookup FAILED: expected nullopt\n";
    return 1;
  }
  optional_tree.insert(7, 0);
  optional_tree.insert(8, -1);
  if (optional_tree.get(7) != 0 || optional_tree.get(8) != -1 ||
      optional_tree.get(6).has_value() || optional_tree.get(9).has_value()) {
    std::cerr << "Optional lookup tests FAILED\n";
    return 1;
  }
  std::cout << "Optional lookup tests PASSED\n";

  std::vector<int> original_order;
  for (int i = 5; i < 50; i += 5) {
    original_order.push_back(i);
  }
  for (int i = 0; i < 50; i++) {
    if (i % 5) {
      original_order.push_back(i);
    }
  }
  if (!test_insertions("Original order", original_order)) {
    return 1;
  }

  std::vector<int> keys(1000);
  std::iota(keys.begin(), keys.end(), 1);
  if (!test_insertions("Ascending", keys)) {
    return 1;
  }

  std::reverse(keys.begin(), keys.end());
  if (!test_insertions("Descending", keys)) {
    return 1;
  }

  // A fixed seed makes any failure reproducible.
  std::mt19937 random(42);
  std::shuffle(keys.begin(), keys.end(), random);
  if (!test_insertions("Shuffled (seed 42)", keys)) {
    return 1;
  }

  std::cout << "All insertion tests passed.\n";
  return 0;
}
