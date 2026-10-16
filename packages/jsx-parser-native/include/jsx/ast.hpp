#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace jsx {

struct Span {
  std::size_t start{};
  std::size_t end{};
};

struct Expression {
  std::string source;
  Span span;
};

struct Attribute {
  enum class ValueKind { Boolean, String, Expression };

  std::string name;
  ValueKind valueKind{ValueKind::Boolean};
  std::string value;
  bool spread{false};
  Span span;
};

struct Node;
using NodePtr = std::shared_ptr<Node>;

struct Node {
  enum class Kind { Element, Fragment, Text, Expression };

  Kind kind{Kind::Text};
  Span span;
  std::string name;
  std::string value;
  bool selfClosing{false};
  std::vector<Attribute> attributes;
  std::vector<NodePtr> children;
};

struct ParseResult {
  NodePtr root;
  std::optional<std::string> error;
  std::size_t consumed{};

  explicit operator bool() const {
    return root != nullptr && !error;
  }
};

ParseResult parse(std::string source);
ParseResult parsePrefix(std::string source);
std::string toJson(const Node& node, bool pretty = true);

} // namespace jsx
