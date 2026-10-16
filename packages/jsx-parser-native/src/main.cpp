#include "jsx/ast.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main(int argc, char** argv) {
  bool pretty = true;
  bool prefix = false;
  std::string file;
  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];
    if (argument == "--compact") {
      pretty = false;
    } else if (argument == "--prefix") {
      prefix = true;
    } else if (argument == "--help" || argument == "-h") {
      std::cout << "Usage: jsx-parser [--compact] [--prefix] [file]\n"
                   "Reads JSX from a file, or from standard input when omitted.\n";
      return 0;
    } else if (file.empty()) {
      file = argument;
    } else {
      std::cerr << "jsx-parser: only one input file is supported\n";
      return 2;
    }
  }

  std::string source;
  if (file.empty() || file == "-") {
    source.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
  } else {
    std::ifstream input(file);
    if (!input) {
      std::cerr << "jsx-parser: cannot open " << file << '\n';
      return 2;
    }
    source.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
  }

  auto result = prefix ? jsx::parsePrefix(std::move(source)) : jsx::parse(std::move(source));
  if (!result) {
    std::cerr << "jsx-parser: " << *result.error << '\n';
    return 1;
  }
  std::cout << jsx::toJson(*result.root, pretty);
  if (!pretty)
    std::cout << '\n';
  return 0;
}
