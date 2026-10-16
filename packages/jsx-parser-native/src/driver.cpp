#include "driver.hpp"

#include "lexer.hpp"
#include "parser.hpp"

#include <sstream>

namespace jsx {

Driver::Driver(std::string input, bool usePrefixMode)
    : source(std::move(input)), prefixMode(usePrefixMode) {
  lexer = std::make_unique<Lexer>(*this);
}

Driver::~Driver() = default;

void Driver::reportError(std::size_t offset, const std::string& message) {
  if (errorMessage)
    return;
  std::size_t line = 1;
  std::size_t column = 1;
  for (std::size_t i = 0; i < offset && i < source.size(); ++i) {
    if (source[i] == '\n') {
      ++line;
      column = 1;
    } else {
      ++column;
    }
  }
  std::ostringstream out;
  out << line << ':' << column << ": " << message;
  errorMessage = out.str();
}

ParseResult parse(std::string source) {
  Driver driver(std::move(source));
  parser generatedParser(driver);
  const int status = generatedParser.parse();
  if (status != 0 || driver.errorMessage) {
    return {nullptr, driver.errorMessage.value_or("JSX parsing failed"), 0};
  }
  const auto consumed = driver.root ? driver.root->span.end : 0;
  return {std::move(driver.root), std::nullopt, consumed};
}

ParseResult parsePrefix(std::string source) {
  Driver driver(std::move(source), true);
  parser generatedParser(driver);
  const int status = generatedParser.parse();
  if (status != 0 || driver.errorMessage) {
    return {nullptr, driver.errorMessage.value_or("JSX parsing failed"), 0};
  }
  const auto consumed = driver.root ? driver.root->span.end : 0;
  return {std::move(driver.root), std::nullopt, consumed};
}

} // namespace jsx
