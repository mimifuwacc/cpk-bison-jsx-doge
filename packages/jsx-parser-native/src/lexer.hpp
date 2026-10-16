#pragma once

#include "driver.hpp"
#include "parser.hpp"

#include <cstddef>

namespace jsx {

class Lexer {
public:
  explicit Lexer(Driver& driver);
  parser::token_type next(parser::semantic_type* value, parser::location_type* location);

private:
  enum class Mode { Data, Tag };

  Driver& driver_;
  std::size_t position_{0};
  Mode mode_{Mode::Data};
  std::size_t elementDepth_{0};
  bool closingTag_{false};
  bool finishedRoot_{false};

  bool startsWith(const char* text) const;
  void skipTagWhitespace();
  TokenValue scanQuotedString();
  TokenValue scanExpression();
  TokenValue scanName();
  void setLocation(parser::location_type* location, Span span) const;
  parser::token_type makeToken(parser::semantic_type* value, parser::location_type* location,
                               parser::token_type type, TokenValue token);
  parser::token_type fixedToken(parser::semantic_type* value, parser::location_type* location,
                                parser::token_type type, std::size_t length);
  parser::token_type nextDataToken(parser::semantic_type* value, parser::location_type* location);
  parser::token_type nextTagToken(parser::semantic_type* value, parser::location_type* location);
};

} // namespace jsx
