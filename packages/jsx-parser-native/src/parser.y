%require "3.8"
%language "c++"
%define api.namespace {jsx}
%define api.parser.class {parser}
%define api.value.type variant
%define parse.error detailed
%locations

%code requires {
  #include "jsx/ast.hpp"
  #include "driver.hpp"
  #include <vector>
}

%parse-param { jsx::Driver& driver }
%lex-param { jsx::Driver& driver }

%code {
  #include "lexer.hpp"
  #include <algorithm>
  #include <cctype>

  static jsx::parser::token_type yylex(
      jsx::parser::semantic_type* value,
      jsx::parser::location_type* location,
      jsx::Driver& driver) {
    return driver.lexer->next(value, location);
  }

  static jsx::Span joined(jsx::Span first, jsx::Span last) {
    return {first.start, last.end};
  }

  static bool whitespaceOnly(const std::string& text) {
    return std::all_of(text.begin(), text.end(), [](unsigned char c) {
      return std::isspace(c);
    });
  }
}

%token END 0 "end of input"
%token <jsx::TokenValue> LT "<"
%token <jsx::TokenValue> LT_SLASH "</"
%token <jsx::TokenValue> GT ">"
%token <jsx::TokenValue> SLASH_GT "/>"
%token <jsx::TokenValue> FRAGMENT_OPEN "<>"
%token <jsx::TokenValue> FRAGMENT_CLOSE "</>"
%token <jsx::TokenValue> EQUAL "="
%token <jsx::TokenValue> NAME "name"
%token <jsx::TokenValue> STRING "string"
%token <jsx::TokenValue> TEXT "text"
%token <jsx::TokenValue> EXPRESSION "expression"

%type <jsx::NodePtr> document node element fragment child
%type <std::vector<jsx::NodePtr>> children
%type <jsx::Attribute> attribute
%type <std::vector<jsx::Attribute>> attributes

%start document

%%

document:
  outer_trivia node outer_trivia END { driver.root = $2; $$ = $2; }
;

outer_trivia:
  %empty
| outer_trivia TEXT {
    if (!whitespaceOnly($2.text)) {
      driver.reportError($2.span.start, "unexpected text outside the root JSX node");
      YYERROR;
    }
  }
;

node:
  element  { $$ = $1; }
| fragment { $$ = $1; }
;

element:
  LT NAME attributes SLASH_GT {
    auto node = std::make_shared<jsx::Node>();
    node->kind = jsx::Node::Kind::Element;
    node->name = $2.text;
    node->attributes = std::move($3);
    node->selfClosing = true;
    node->span = joined($1.span, $4.span);
    $$ = std::move(node);
  }
| LT NAME attributes GT children LT_SLASH NAME GT {
    if ($2.text != $7.text) {
      driver.reportError($7.span.start,
        "closing tag </" + $7.text + "> does not match <" + $2.text + ">");
      YYERROR;
    }
    auto node = std::make_shared<jsx::Node>();
    node->kind = jsx::Node::Kind::Element;
    node->name = $2.text;
    node->attributes = std::move($3);
    node->children = std::move($5);
    node->span = joined($1.span, $8.span);
    $$ = std::move(node);
  }
;

fragment:
  FRAGMENT_OPEN children FRAGMENT_CLOSE {
    auto node = std::make_shared<jsx::Node>();
    node->kind = jsx::Node::Kind::Fragment;
    node->children = std::move($2);
    node->span = joined($1.span, $3.span);
    $$ = std::move(node);
  }
;

attributes:
  %empty               { $$ = {}; }
| attributes attribute { $1.push_back(std::move($2)); $$ = std::move($1); }
;

attribute:
  NAME {
    $$.name = $1.text;
    $$.span = $1.span;
  }
| NAME EQUAL STRING {
    $$.name = $1.text;
    $$.valueKind = jsx::Attribute::ValueKind::String;
    $$.value = $3.text;
    $$.span = joined($1.span, $3.span);
  }
| NAME EQUAL EXPRESSION {
    $$.name = $1.text;
    $$.valueKind = jsx::Attribute::ValueKind::Expression;
    $$.value = $3.text;
    $$.span = joined($1.span, $3.span);
  }
| EXPRESSION {
    if (!$1.text.starts_with("...")) {
      driver.reportError($1.span.start, "only spread expressions are valid in an attribute list");
      YYERROR;
    }
    $$.spread = true;
    $$.name = "";
    $$.valueKind = jsx::Attribute::ValueKind::Expression;
    $$.value = $1.text.substr(3);
    $$.span = $1.span;
  }
;

children:
  %empty         { $$ = {}; }
| children child { $1.push_back(std::move($2)); $$ = std::move($1); }
;

child:
  element  { $$ = $1; }
| fragment { $$ = $1; }
| TEXT {
    auto node = std::make_shared<jsx::Node>();
    node->kind = jsx::Node::Kind::Text;
    node->value = $1.text;
    node->span = $1.span;
    $$ = std::move(node);
  }
| EXPRESSION {
    auto node = std::make_shared<jsx::Node>();
    node->kind = jsx::Node::Kind::Expression;
    node->value = $1.text;
    node->span = $1.span;
    $$ = std::move(node);
  }
;

%%

void jsx::parser::error(const location_type& location, const std::string& message) {
  driver.reportError(location.begin.column, message);
}
