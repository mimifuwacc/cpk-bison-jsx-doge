#include "jsx/ast.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

jsx::NodePtr parseOk(const std::string& source) {
  auto result = jsx::parse(source);
  require(static_cast<bool>(result),
          "expected parse success for " + source + (result.error ? ": " + *result.error : ""));
  return result.root;
}

void testElementWithAttributesAndChildren() {
  const auto root = parseOk(
      R"(<Comp enabled title="hello" count={items.length} {...props}>Hello {user.name}<Icon /></Comp>)");
  require(root->kind == jsx::Node::Kind::Element, "root is an element");
  require(root->name == "Comp", "component name is preserved");
  require(root->attributes.size() == 4, "all attributes are parsed");
  require(root->attributes[0].valueKind == jsx::Attribute::ValueKind::Boolean,
          "boolean attribute is represented");
  require(root->attributes[1].value == "hello", "string attribute is represented");
  require(root->attributes[2].value == "items.length", "expression attribute is represented");
  require(root->attributes[3].spread && root->attributes[3].value == "props",
          "spread attribute is represented");
  require(root->children.size() == 3, "text, expression, and nested element are children");
  require(root->children[1]->kind == jsx::Node::Kind::Expression,
          "expression child has the correct type");
  require(root->children[2]->selfClosing, "nested element is self closing");
}

void testFragmentAndBalancedExpression() {
  const std::string source = R"(<><div>{call({ value: "}" })}</div>{`text }`}</>)";
  const auto root = parseOk(source);
  require(root->kind == jsx::Node::Kind::Fragment, "fragment root is parsed");
  require(root->children.size() == 2, "fragment children are parsed");
  require(root->children[0]->children[0]->value == R"(call({ value: "}" }))",
          "nested braces and strings stay inside the expression");
  require(root->span.end == source.size(), "root span covers the source");
}

void testExpressionStringsAndComments() {
  const auto root = parseOk(R"(<Panel>{call({ value: "}" })}{/* } */value}{`text }`}</Panel>)");
  require(root->children.size() == 3, "all expression containers are preserved");
  require(root->children[0]->value == R"(call({ value: "}" }))",
          "braces inside strings do not end an expression");
  require(root->children[1]->value == "/* } */value",
          "braces inside comments do not end an expression");
  require(root->children[2]->value == "`text }`",
          "braces inside template strings do not end an expression");
}

void testNestedFragmentsAndQuotedAttributes() {
  const auto root = parseOk(R"(<Panel title='hello' data-value="world"><><Icon /></></Panel>)");
  require(root->attributes.size() == 2, "quoted attributes are parsed");
  require(root->attributes[0].value == "hello", "single-quoted attribute is preserved");
  require(root->attributes[1].value == "world", "double-quoted attribute is preserved");
  require(root->children.size() == 1, "nested fragment is parsed as one child");
  require(root->children[0]->kind == jsx::Node::Kind::Fragment,
          "nested fragment has the fragment kind");
  require(root->children[0]->children.size() == 1, "fragment child is preserved");
}

void testMemberAndNamespacedNames() {
  const auto root = parseOk(R"(<UI.Button xml:lang='ja' data-id="1" />)");
  require(root->name == "UI.Button", "member name is accepted");
  require(root->attributes[0].name == "xml:lang", "namespaced attribute is accepted");
  require(root->attributes[1].name == "data-id", "hyphenated attribute is accepted");
}

void testErrors() {
  auto mismatch = jsx::parse("<div></span>");
  require(!mismatch, "mismatched closing tag is rejected");
  require(mismatch.error->find("does not match") != std::string::npos,
          "mismatched tag error is useful");

  auto unterminated = jsx::parse("<div>{value</div>");
  require(!unterminated, "unterminated expression is rejected");

  auto invalidAttribute = jsx::parse("<div {value}></div>");
  require(!invalidAttribute, "non-spread expression in attributes is rejected");
}

void testPrefixParsing() {
  auto result = jsx::parsePrefix("<Card><Icon /></Card> + trailingJavaScript");
  require(static_cast<bool>(result), "a JSX prefix can be parsed from JavaScript");
  require(result.root->name == "Card", "prefix parser returns the JSX root");
  require(result.consumed == std::string("<Card><Icon /></Card>").size(),
          "prefix parser reports where Babel should resume");
}

} // namespace

int main() {
  testElementWithAttributesAndChildren();
  testFragmentAndBalancedExpression();
  testExpressionStringsAndComments();
  testNestedFragmentsAndQuotedAttributes();
  testMemberAndNamespacedNames();
  testErrors();
  testPrefixParsing();
  std::cout << "All parser tests passed\n";
}
