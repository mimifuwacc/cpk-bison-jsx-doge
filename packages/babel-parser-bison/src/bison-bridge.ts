import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import { dirname, resolve } from "node:path";
import { parseExpression } from "@babel/parser";

const projectRoot = resolve(dirname(fileURLToPath(import.meta.url)), "../../..");
const require = createRequire(import.meta.url);
const nativeParser = require(resolve(projectRoot, "build/node-addon/jsx_parser_node.node"));

function convertUtf8SpansToUtf16(root, source) {
  const byteToCodeUnit = [0];
  let byteOffset = 0;
  let codeUnitOffset = 0;
  for (const character of source) {
    const byteLength = Buffer.byteLength(character, "utf8");
    const codeUnitLength = character.length;
    for (let index = 0; index < byteLength; index += 1) {
      byteToCodeUnit[byteOffset + index] = codeUnitOffset;
    }
    byteOffset += byteLength;
    codeUnitOffset += codeUnitLength;
    byteToCodeUnit[byteOffset] = codeUnitOffset;
  }

  const visit = (node) => {
    if (node.span) {
      node.span.start = byteToCodeUnit[node.span.start];
      node.span.end = byteToCodeUnit[node.span.end];
    }
    node.attributes?.forEach(visit);
    node.children?.forEach(visit);
  };
  visit(root);
  return root;
}

function positionAt(source, index) {
  let line = 1;
  let lineStart = 0;
  for (let cursor = 0; cursor < index; cursor += 1) {
    if (source.charCodeAt(cursor) === 10) {
      line += 1;
      lineStart = cursor + 1;
    }
  }
  return { line, column: index - lineStart, index };
}

function withLocation(node, source, start, end) {
  node.start = start;
  node.end = end;
  node.loc = {
    start: positionAt(source, start),
    end: positionAt(source, end),
  };
  return node;
}

function jsxName(name, source, start) {
  if (name.includes(":")) {
    const separator = name.indexOf(":");
    return withLocation(
      {
        type: "JSXNamespacedName",
        namespace: jsxName(name.slice(0, separator), source, start),
        name: jsxName(name.slice(separator + 1), source, start + separator + 1),
      },
      source,
      start,
      start + name.length,
    );
  }
  if (name.includes(".")) {
    const parts = name.split(".");
    let object = jsxName(parts[0], source, start);
    let cursor = start + parts[0].length + 1;
    for (const part of parts.slice(1)) {
      const property = jsxName(part, source, cursor);
      object = withLocation(
        { type: "JSXMemberExpression", object, property },
        source,
        object.start,
        property.end,
      );
      cursor += part.length + 1;
    }
    return object;
  }
  return withLocation({ type: "JSXIdentifier", name }, source, start, start + name.length);
}

function expressionNode(raw, source, start, parserOptions) {
  const leading = raw.length - raw.trimStart().length;
  const expressionSource = raw.trim();
  const expressionStart = start + leading;
  if (expressionSource === "" || /^\/\*[\s\S]*\*\/$/.test(expressionSource)) {
    return withLocation(
      { type: "JSXEmptyExpression", innerComments: [] },
      source,
      expressionStart,
      expressionStart + expressionSource.length,
    );
  }
  const plugins = parserOptions.plugins || [];
  return parseExpression(expressionSource, {
    plugins,
    sourceType: parserOptions.sourceType === "module" ? "module" : "script",
    allowAwaitOutsideFunction: true,
    allowYieldOutsideFunction: true,
    startIndex: expressionStart,
    startLine: positionAt(source, expressionStart).line,
    startColumn: positionAt(source, expressionStart).column,
  });
}

function findOpeningEnd(source, start, end) {
  let quote = null;
  let braces = 0;
  for (let cursor = start; cursor < end; cursor += 1) {
    const char = source[cursor];
    if (quote) {
      if (char === "\\") cursor += 1;
      else if (char === quote) quote = null;
    } else if (char === '"' || char === "'") {
      quote = char;
    } else if (char === "{") {
      braces += 1;
    } else if (char === "}") {
      braces -= 1;
    } else if (char === ">" && braces === 0) {
      return cursor + 1;
    }
  }
  return end;
}

function convertAttribute(attribute, source, offset, parserOptions) {
  const start = offset + attribute.span.start;
  const end = offset + attribute.span.end;
  if (attribute.type === "JSXSpreadAttribute") {
    const expressionOffset = source.indexOf("...", start) + 3;
    return withLocation(
      {
        type: "JSXSpreadAttribute",
        argument: expressionNode(attribute.value, source, expressionOffset, parserOptions),
      },
      source,
      start,
      end,
    );
  }
  const name = jsxName(attribute.name, source, start);
  let value = null;
  if (attribute.valueType === "string") {
    const quoteStart = source.indexOf("=", name.end) + 1;
    value = withLocation(
      {
        type: "StringLiteral",
        value: attribute.value,
        extra: {
          raw: source.slice(quoteStart, end),
          rawValue: attribute.value,
        },
      },
      source,
      quoteStart,
      end,
    );
  } else if (attribute.valueType === "expression") {
    const braceStart = source.indexOf("{", name.end);
    value = withLocation(
      {
        type: "JSXExpressionContainer",
        expression: expressionNode(attribute.value, source, braceStart + 1, parserOptions),
      },
      source,
      braceStart,
      end,
    );
  }
  return withLocation({ type: "JSXAttribute", name, value }, source, start, end);
}

function convertNode(input, source, offset, parserOptions) {
  const start = offset + input.span.start;
  const end = offset + input.span.end;
  if (input.type === "JSXText") {
    return withLocation({ type: "JSXText", value: input.value }, source, start, end);
  }
  if (input.type === "JSXExpressionContainer") {
    return withLocation(
      {
        type: "JSXExpressionContainer",
        expression: expressionNode(input.source, source, start + 1, parserOptions),
      },
      source,
      start,
      end,
    );
  }

  const children = input.children.map((child) => convertNode(child, source, offset, parserOptions));
  if (input.type === "JSXFragment") {
    const openingFragment = withLocation({ type: "JSXOpeningFragment" }, source, start, start + 2);
    const closingFragment = withLocation({ type: "JSXClosingFragment" }, source, end - 3, end);
    return withLocation(
      { type: "JSXFragment", openingFragment, closingFragment, children },
      source,
      start,
      end,
    );
  }

  const openingEnd = findOpeningEnd(source, start, end);
  const nameStart = start + 1;
  const openingName = jsxName(input.name, source, nameStart);
  const attributes = input.attributes.map((attribute) =>
    convertAttribute(attribute, source, offset, parserOptions),
  );
  const openingElement = withLocation(
    {
      type: "JSXOpeningElement",
      name: openingName,
      attributes,
      selfClosing: input.selfClosing,
    },
    source,
    start,
    openingEnd,
  );

  let closingElement = null;
  if (!input.selfClosing) {
    const closingStart = source.lastIndexOf("</", end);
    closingElement = withLocation(
      {
        type: "JSXClosingElement",
        name: jsxName(input.name, source, closingStart + 2),
      },
      source,
      closingStart,
      end,
    );
  }
  return withLocation(
    { type: "JSXElement", openingElement, closingElement, children },
    source,
    start,
    end,
  );
}

export function parseBisonJsxIntoBabel(source, offset, parserOptions = {}) {
  let ast;
  try {
    const jsxSource = source.slice(offset);
    ast = convertUtf8SpansToUtf16(JSON.parse(nativeParser.parsePrefix(jsxSource)), jsxSource);
  } catch (error) {
    throw new SyntaxError(`Bison JSX parser: ${error.message}`, {
      cause: error,
    });
  }
  return convertNode(ast, source, offset, parserOptions);
}
