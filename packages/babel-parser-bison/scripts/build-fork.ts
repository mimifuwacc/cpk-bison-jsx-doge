import { readFile, mkdir, writeFile } from "node:fs/promises";
import { createRequire } from "node:module";
import { dirname, resolve } from "node:path";

const require = createRequire(import.meta.url);
const upstream = require.resolve("@babel/parser");
const output = resolve("build/babel-parser-bison/parser.mjs");
let source = await readFile(upstream, "utf8");

const original = `  parseExprAtom(refExpressionErrors) {
    if (this.match(138)) {
      return this.jsxParseElement();
    } else if (this.match(43) && this.input.charCodeAt(this.state.pos) !== 33) {
      this.replaceToken(138);
      return this.jsxParseElement();
    } else {
      return super.parseExprAtom(refExpressionErrors);
    }
  }`;

const replacement = `  parseExprAtom(refExpressionErrors) {
    if (this.match(138) || this.match(43) && this.input.charCodeAt(this.state.pos) !== 33) {
      if (this.match(43)) this.replaceToken(138);
      const start = this.state.start;
      const node = parseBisonJsxIntoBabel(this.input, start, this.options);
      const end = node.end;
      for (let cursor = this.state.pos; cursor < end; cursor++) {
        if (this.input.charCodeAt(cursor) === 10) {
          this.state.curLine++;
          this.state.lineStart = cursor + 1;
        }
      }
      this.state.pos = end;
      this.state.end = end;
      this.state.endLoc = this.state.curPosition();
      if (this.state.context[this.state.context.length - 1] === types.j_oTag) {
        this.state.context.pop();
      }
      this.state.canStartJSXElement = false;
      this.next();
      return node;
    }
    return super.parseExprAtom(refExpressionErrors);
  }`;

if (!source.includes(original)) {
  throw new Error("Unsupported @babel/parser build: JSX parseExprAtom hook was not found");
}

source = `// @ts-nocheck\nimport { parseBisonJsxIntoBabel } from "../../packages/babel-parser-bison/src/bison-bridge.ts";\n${source.replace(original, replacement)}`;
await mkdir(dirname(output), { recursive: true });
await writeFile(output, source);
console.log(`Built Babel parser fork from ${upstream}`);
