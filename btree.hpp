#include <iosfwd>
#include <memory>
#include <optional>
#include <vector>

namespace bt {
class bplustree {
private:
  struct Node {
    bool is_leaf;
    explicit Node(bool entry) : is_leaf(entry) {}
    Node *parent = nullptr;
    virtual ~Node() = default;
  };

  struct Leaf : Node {
    std::vector<std::pair<int, int>> entries;
    Node *next = nullptr;
    Leaf() : Node(true) {}
  };

  struct Internal : Node {
    std::vector<int> separators;
    std::vector<std::unique_ptr<Node>> children;

    Internal() : Node(false) {}
  };

  std::unique_ptr<Node> root;
  int children_allowed = 3;
  auto get_leaf(int key) -> Leaf*;
  auto split(Leaf*) -> void;

public:
  bplustree();
  auto insert(int key, int value) -> void;
  auto get(int key) -> std::optional<int>;
  auto validate(std::ostream &errors) const -> bool;
  auto del(int key) -> void;
};
} // namespace bt
