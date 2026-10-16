#include "jsx/ast.hpp"

#include <sstream>

namespace jsx {
namespace {

std::string escapeJson(const std::string& input) {
  std::ostringstream out;
  for (const unsigned char c : input) {
    switch (c) {
    case '"':
      out << "\\\"";
      break;
    case '\\':
      out << "\\\\";
      break;
    case '\b':
      out << "\\b";
      break;
    case '\f':
      out << "\\f";
      break;
    case '\n':
      out << "\\n";
      break;
    case '\r':
      out << "\\r";
      break;
    case '\t':
      out << "\\t";
      break;
    default:
      if (c < 0x20) {
        constexpr char hex[] = "0123456789abcdef";
        out << "\\u00" << hex[c >> 4] << hex[c & 0xf];
      } else {
        out << c;
      }
    }
  }
  return out.str();
}

class JsonWriter {
public:
  explicit JsonWriter(bool pretty) : pretty_(pretty) {}

  std::string write(const Node& node) {
    writeNode(node, 0);
    if (pretty_)
      out_ << '\n';
    return out_.str();
  }

private:
  std::ostringstream out_;
  bool pretty_;

  void newline(int depth) {
    if (!pretty_)
      return;
    out_ << '\n' << std::string(static_cast<std::size_t>(depth * 2), ' ');
  }
  void separator(int depth) {
    out_ << ',';
    newline(depth);
  }
  void key(const char* name) {
    out_ << '"' << name << "\":" << (pretty_ ? " " : "");
  }
  void string(const std::string& value) {
    out_ << '"' << escapeJson(value) << '"';
  }
  void span(Span value) {
    out_ << '{';
    key("start");
    out_ << value.start;
    out_ << ',';
    if (pretty_)
      out_ << ' ';
    key("end");
    out_ << value.end << '}';
  }

  void writeAttribute(const Attribute& attribute, int depth) {
    out_ << '{';
    newline(depth + 1);
    key("type");
    string(attribute.spread ? "JSXSpreadAttribute" : "JSXAttribute");
    separator(depth + 1);
    key("name");
    string(attribute.name);
    separator(depth + 1);
    key("valueType");
    switch (attribute.valueKind) {
    case Attribute::ValueKind::Boolean:
      string("boolean");
      break;
    case Attribute::ValueKind::String:
      string("string");
      break;
    case Attribute::ValueKind::Expression:
      string("expression");
      break;
    }
    separator(depth + 1);
    key("value");
    if (attribute.valueKind == Attribute::ValueKind::Boolean)
      out_ << "true";
    else
      string(attribute.value);
    separator(depth + 1);
    key("span");
    span(attribute.span);
    newline(depth);
    out_ << '}';
  }

  void writeNode(const Node& node, int depth) {
    out_ << '{';
    newline(depth + 1);
    key("type");
    switch (node.kind) {
    case Node::Kind::Element:
      string("JSXElement");
      break;
    case Node::Kind::Fragment:
      string("JSXFragment");
      break;
    case Node::Kind::Text:
      string("JSXText");
      break;
    case Node::Kind::Expression:
      string("JSXExpressionContainer");
      break;
    }
    separator(depth + 1);
    key("span");
    span(node.span);

    if (node.kind == Node::Kind::Element) {
      separator(depth + 1);
      key("name");
      string(node.name);
      separator(depth + 1);
      key("selfClosing");
      out_ << (node.selfClosing ? "true" : "false");
      separator(depth + 1);
      key("attributes");
      out_ << '[';
      for (std::size_t i = 0; i < node.attributes.size(); ++i) {
        if (i)
          out_ << ',';
        newline(depth + 2);
        writeAttribute(node.attributes[i], depth + 2);
      }
      if (!node.attributes.empty())
        newline(depth + 1);
      out_ << ']';
    } else if (node.kind == Node::Kind::Text || node.kind == Node::Kind::Expression) {
      separator(depth + 1);
      key(node.kind == Node::Kind::Text ? "value" : "source");
      string(node.value);
    }

    if (node.kind == Node::Kind::Element || node.kind == Node::Kind::Fragment) {
      separator(depth + 1);
      key("children");
      out_ << '[';
      for (std::size_t i = 0; i < node.children.size(); ++i) {
        if (i)
          out_ << ',';
        newline(depth + 2);
        writeNode(*node.children[i], depth + 2);
      }
      if (!node.children.empty())
        newline(depth + 1);
      out_ << ']';
    }
    newline(depth);
    out_ << '}';
  }
};

} // namespace

std::string toJson(const Node& node, bool pretty) {
  return JsonWriter(pretty).write(node);
}

} // namespace jsx
