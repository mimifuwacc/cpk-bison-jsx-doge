#include "lexer.hpp"

#include <cctype>
#include <cstring>
#include <stdexcept>

namespace jsx {

Lexer::Lexer(Driver& driver) : driver_(driver) {}

bool Lexer::startsWith(const char* text) const {
  const auto length = std::strlen(text);
  return driver_.source.compare(position_, length, text) == 0;
}

void Lexer::setLocation(parser::location_type* location, Span span) const {
  location->begin.column = static_cast<int>(span.start);
  location->end.column = static_cast<int>(span.end);
}

void Lexer::skipTagWhitespace() {
  while (position_ < driver_.source.size() &&
         std::isspace(static_cast<unsigned char>(driver_.source[position_]))) {
    ++position_;
  }
}

TokenValue Lexer::scanName() {
  const auto start = position_;
  while (position_ < driver_.source.size()) {
    const char c = driver_.source[position_];
    if (std::isspace(static_cast<unsigned char>(c)) || c == '=' || c == '/' || c == '>') {
      break;
    }
    ++position_;
  }
  return {driver_.source.substr(start, position_ - start), {start, position_}};
}

TokenValue Lexer::scanQuotedString() {
  const auto start = position_;
  const char quote = driver_.source[position_++];
  std::string value;
  while (position_ < driver_.source.size()) {
    const char c = driver_.source[position_++];
    if (c == quote) {
      return {std::move(value), {start, position_}};
    }
    if (c == '\\' && position_ < driver_.source.size()) {
      value.push_back(c);
      value.push_back(driver_.source[position_++]);
    } else {
      value.push_back(c);
    }
  }
  throw std::runtime_error("unterminated quoted attribute value");
}

TokenValue Lexer::scanExpression() {
  const auto start = position_++;
  const auto contentStart = position_;
  int braceDepth = 1;
  char quote = 0;
  bool escaped = false;
  bool lineComment = false;
  bool blockComment = false;

  while (position_ < driver_.source.size()) {
    const char c = driver_.source[position_];
    const char next = position_ + 1 < driver_.source.size() ? driver_.source[position_ + 1] : '\0';

    if (lineComment) {
      ++position_;
      if (c == '\n')
        lineComment = false;
      continue;
    }
    if (blockComment) {
      position_ += (c == '*' && next == '/') ? 2 : 1;
      if (c == '*' && next == '/')
        blockComment = false;
      continue;
    }
    if (quote != 0) {
      ++position_;
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == quote) {
        quote = 0;
      }
      continue;
    }
    if (c == '/' && next == '/') {
      lineComment = true;
      position_ += 2;
      continue;
    }
    if (c == '/' && next == '*') {
      blockComment = true;
      position_ += 2;
      continue;
    }
    if (c == '\'' || c == '"' || c == '`') {
      quote = c;
      ++position_;
      continue;
    }
    if (c == '{') {
      ++braceDepth;
      ++position_;
      continue;
    }
    if (c == '}') {
      --braceDepth;
      if (braceDepth == 0) {
        const auto contentEnd = position_++;
        return {driver_.source.substr(contentStart, contentEnd - contentStart), {start, position_}};
      }
    }
    ++position_;
  }
  throw std::runtime_error("unterminated JSX expression container");
}

parser::token_type Lexer::makeToken(parser::semantic_type* value, parser::location_type* location,
                                    parser::token_type type, TokenValue token) {
  setLocation(location, token.span);
  value->emplace<TokenValue>(std::move(token));
  return type;
}

parser::token_type Lexer::fixedToken(parser::semantic_type* value, parser::location_type* location,
                                     parser::token_type type, std::size_t length) {
  const auto start = position_;
  position_ += length;
  return makeToken(value, location, type,
                   {driver_.source.substr(start, length), {start, position_}});
}

parser::token_type Lexer::nextDataToken(parser::semantic_type* value,
                                        parser::location_type* location) {
  if (startsWith("</>")) {
    auto token = fixedToken(value, location, parser::token::FRAGMENT_CLOSE, 3);
    if (elementDepth_ > 0)
      --elementDepth_;
    finishedRoot_ = elementDepth_ == 0;
    return token;
  }
  if (startsWith("</")) {
    mode_ = Mode::Tag;
    closingTag_ = true;
    return fixedToken(value, location, parser::token::LT_SLASH, 2);
  }
  if (startsWith("<>")) {
    ++elementDepth_;
    return fixedToken(value, location, parser::token::FRAGMENT_OPEN, 2);
  }
  if (driver_.source[position_] == '<') {
    mode_ = Mode::Tag;
    closingTag_ = false;
    ++elementDepth_;
    return fixedToken(value, location, parser::token::LT, 1);
  }
  if (driver_.source[position_] == '{')
    return makeToken(value, location, parser::token::EXPRESSION, scanExpression());

  const auto start = position_;
  while (position_ < driver_.source.size() && driver_.source[position_] != '<' &&
         driver_.source[position_] != '{') {
    ++position_;
  }
  return makeToken(value, location, parser::token::TEXT,
                   {driver_.source.substr(start, position_ - start), {start, position_}});
}

parser::token_type Lexer::nextTagToken(parser::semantic_type* value,
                                       parser::location_type* location) {
  if (startsWith("/>")) {
    mode_ = Mode::Data;
    auto token = fixedToken(value, location, parser::token::SLASH_GT, 2);
    if (elementDepth_ > 0)
      --elementDepth_;
    finishedRoot_ = elementDepth_ == 0;
    return token;
  }
  if (driver_.source[position_] == '>') {
    mode_ = Mode::Data;
    auto token = fixedToken(value, location, parser::token::GT, 1);
    if (closingTag_) {
      if (elementDepth_ > 0)
        --elementDepth_;
      finishedRoot_ = elementDepth_ == 0;
    }
    closingTag_ = false;
    return token;
  }
  if (driver_.source[position_] == '=')
    return fixedToken(value, location, parser::token::EQUAL, 1);
  if (driver_.source[position_] == '{')
    return makeToken(value, location, parser::token::EXPRESSION, scanExpression());
  if (driver_.source[position_] == '\'' || driver_.source[position_] == '"')
    return makeToken(value, location, parser::token::STRING, scanQuotedString());

  auto name = scanName();
  if (!name.text.empty())
    return makeToken(value, location, parser::token::NAME, std::move(name));
  throw std::runtime_error("unexpected character in JSX tag");
}

parser::token_type Lexer::next(parser::semantic_type* value, parser::location_type* location) {
  try {
    if (driver_.prefixMode && finishedRoot_) {
      setLocation(location, {position_, position_});
      return parser::token::END;
    }
    if (mode_ == Mode::Tag)
      skipTagWhitespace();
    if (position_ >= driver_.source.size()) {
      setLocation(location, {position_, position_});
      return parser::token::END;
    }
    if (mode_ == Mode::Data)
      return nextDataToken(value, location);
    return nextTagToken(value, location);
  } catch (const std::runtime_error& error) {
    driver_.reportError(position_, error.what());
    setLocation(location, {position_, position_});
    return parser::token::YYerror;
  }
}

} // namespace jsx
