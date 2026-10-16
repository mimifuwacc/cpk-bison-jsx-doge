#pragma once

#include "jsx/ast.hpp"

#include <memory>
#include <string>

namespace jsx {

class Lexer;

struct TokenValue {
  std::string text;
  Span span;
};

class Driver {
public:
  explicit Driver(std::string input, bool prefixMode = false);
  ~Driver();

  Driver(const Driver&) = delete;
  Driver& operator=(const Driver&) = delete;

  std::string source;
  std::unique_ptr<Lexer> lexer;
  NodePtr root;
  std::optional<std::string> errorMessage;
  bool prefixMode{false};

  void reportError(std::size_t offset, const std::string& message);
};

} // namespace jsx
