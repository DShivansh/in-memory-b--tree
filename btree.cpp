#include "btree.hpp"
#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>

bt::bplustree::bplustree() { root = std::make_unique<bt::bplustree::Leaf>(); }

auto bt::bplustree::get_leaf(int key) -> bt::bplustree::Leaf * {
  auto temp = root.get();
  while (!temp->is_leaf) {
    auto internal = dynamic_cast<bt::bplustree::Internal *>(temp);
    auto point = std::upper_bound((internal->separators).begin(),
                                  (internal->separators).end(), key) -
                 (internal->separators).begin();

    temp = (internal->children)[point].get();
  }
  return dynamic_cast<bt::bplustree::Leaf *>(temp);
}

auto bt::bplustree::split(Leaf *l) -> void {
  if (l->entries.size() > children_allowed) {
    auto &entries = l->entries;
    int mid = static_cast<int>(entries.size()) / 2;
    int to_insert_parent = entries[mid].first;
    std::vector<std::pair<int, int>> second(entries.begin() + mid,
                                            entries.end());
    auto second_pt = std::make_unique<bt::bplustree::Leaf>();
    second_pt->entries = std::move(second);
    second_pt->parent = l->parent;
    second_pt->next = l->next;
    l->next = second_pt.get();
    entries.resize(mid);
    if (l->parent) {
      auto parent = dynamic_cast<bt::bplustree::Internal *>(l->parent);
      auto it = std::upper_bound(parent->separators.begin(),
                                 parent->separators.end(), to_insert_parent);
      auto index = it - parent->separators.begin();
      parent->separators.insert(it, to_insert_parent);
      parent->children.insert(parent->children.begin() + index + 1,
                              std::move(second_pt));
      while (parent->separators.size() > (children_allowed - 1)) {
        int mid = static_cast<int>(parent->separators.size()) / 2;
        int elem_to_move_above = (parent->separators)[mid];

        std::vector<int> second_separators(parent->separators.begin() + mid + 1,
                                           parent->separators.end());
        std::vector<std::unique_ptr<Node>> second_children(
            std::make_move_iterator(parent->children.begin() + mid + 1),
            std::make_move_iterator(parent->children.end()));

        auto second_internal = std::make_unique<bt::bplustree::Internal>();

        second_internal->separators = std::move(second_separators);
        second_internal->children = std::move(second_children);

        for (auto &i : (second_internal->children)) {
          i->parent = second_internal.get();
        }

        second_internal->parent = parent->parent;

        parent->separators.resize(mid);
        parent->children.resize(mid + 1);
        if (parent->parent) {
          parent = dynamic_cast<bt::bplustree::Internal *>(parent->parent);
          auto it =
              std::upper_bound(parent->separators.begin(),
                               parent->separators.end(), elem_to_move_above);
          auto index = it - parent->separators.begin();
          parent->separators.insert(it, elem_to_move_above);
          parent->children.insert(parent->children.begin() + index + 1,
                                  std::move(second_internal));
        } else {
          auto temp_parent =
              std::move(std::make_unique<bt::bplustree::Internal>());
          temp_parent->separators.push_back(elem_to_move_above);
          root->parent = temp_parent.get();
          second_internal->parent = temp_parent.get();
          temp_parent->children.push_back(std::move(root));
          temp_parent->children.push_back(std::move(second_internal));
          root = std::move(temp_parent);
          break;
        }
      }
    } else {
      auto temp_parent = std::move(std::make_unique<bt::bplustree::Internal>());
      temp_parent->separators.push_back(to_insert_parent);
      root->parent = temp_parent.get();
      second_pt->parent = temp_parent.get();
      temp_parent->children.push_back(std::move(root));
      temp_parent->children.push_back(std::move(second_pt));
      root = std::move(temp_parent);
    }
  } else {
    return;
  }
}

auto bt::bplustree::insert(int key, int value) -> void {
  if (!get(key)) {
    auto leaf = get_leaf(key);
    auto &l_entries = leaf->entries;
    auto it = std::upper_bound(l_entries.begin(), l_entries.end(),
                               std::make_pair(key, value));
    l_entries.insert(it, std::make_pair(key, value));
    split(leaf);
  }
}

auto bt::bplustree::get(int key) -> std::optional<int> {
  auto leaf = get_leaf(key);
  for (auto &[k, v] : leaf->entries) {
    if (k == key) {
      return v;
    }
  }
  return std::nullopt;
}

auto bt::bplustree::validate(std::ostream &errors) const -> bool {
  std::vector<const Leaf *> leaves;
  std::optional<int> leaf_depth;
  const auto max_children = static_cast<std::size_t>(children_allowed);

  // Bounds match upper_bound routing: lower is inclusive, upper exclusive.
  auto check = [&](auto &&self, const Node *node, const Node *expected_parent,
                   int depth, std::optional<int> lower,
                   std::optional<int> upper) -> bool {
    auto fail = [&](const char *message) {
      errors << "Structure error at depth " << depth << ": " << message << '\n';
      return false;
    };

    if (!node) {
      return fail("null node");
    }
    if (node->parent != expected_parent) {
      return fail("incorrect parent pointer");
    }

    if (node->is_leaf) {
      const auto *leaf = dynamic_cast<const Leaf *>(node);
      if (!leaf) {
        return fail("leaf flag does not match node type");
      }
      if (leaf->entries.size() > max_children ||
          (expected_parent && leaf->entries.size() < (max_children + 1) / 2)) {
        return fail("invalid number of leaf entries");
      }
      if (leaf_depth && *leaf_depth != depth) {
        return fail("leaves have different depths");
      }
      leaf_depth = depth;
      for (std::size_t i = 0; i < leaf->entries.size(); ++i) {
        int key = leaf->entries[i].first;
        if (i && leaf->entries[i - 1].first >= key) {
          return fail("leaf keys are not strictly increasing");
        }
        if ((lower && key < *lower) || (upper && key >= *upper)) {
          return fail("leaf key is outside its separator range");
        }
      }
      leaves.push_back(leaf);
      return true;
    }

    const auto *internal = dynamic_cast<const Internal *>(node);
    if (!internal) {
      return fail("internal flag does not match node type");
    }
    if (internal->children.size() != internal->separators.size() + 1) {
      return fail("child count must equal separator count plus one");
    }
    const auto min_children = expected_parent ? (max_children + 1) / 2 : 2;
    if (internal->children.size() < min_children ||
        internal->children.size() > max_children) {
      return fail("invalid number of internal children");
    }
    for (std::size_t i = 1; i < internal->separators.size(); ++i) {
      if (internal->separators[i - 1] >= internal->separators[i]) {
        return fail("separators are not strictly increasing");
      }
    }
    for (std::size_t i = 0; i < internal->children.size(); ++i) {
      auto child_lower = i ? std::optional<int>(internal->separators[i - 1]) : lower;
      auto child_upper = i < internal->separators.size()
                             ? std::optional<int>(internal->separators[i])
                             : upper;
      const auto first_leaf = leaves.size();
      if (!self(self, internal->children[i].get(), node, depth + 1,
                child_lower, child_upper)) {
        return false;
      }
      // In this insertion-only tree, a separator equals the right subtree minimum.
      if (i && leaves[first_leaf]->entries.front().first !=
                   internal->separators[i - 1]) {
        return fail("separator does not equal right subtree's minimum key");
      }
    }
    return true;
  };

  if (!check(check, root.get(), nullptr, 0, std::nullopt, std::nullopt)) {
    return false;
  }
  // Compare links with the independently collected tree order, without following
  // possibly invalid next pointers. This also detects cycles and skipped leaves.
  for (std::size_t i = 0; i < leaves.size(); ++i) {
    const Node *expected_next = i + 1 < leaves.size() ? leaves[i + 1] : nullptr;
    if (leaves[i]->next != expected_next) {
      errors << "Structure error: incorrect next link at leaf " << i << '\n';
      return false;
    }
    if (i && leaves[i - 1]->entries.back().first >= leaves[i]->entries.front().first) {
      errors << "Structure error: leaf ranges overlap or are out of order\n";
      return false;
    }
  }
  return true;
}
