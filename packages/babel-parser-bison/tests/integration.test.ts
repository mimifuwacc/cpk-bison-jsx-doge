import assert from "node:assert/strict";
import test from "node:test";
import { transformAsync } from "@babel/core";
import transformReactJsx from "@babel/plugin-transform-react-jsx";
import bisonJsxParserPlugin from "../src/parser-plugin.ts";

async function compile(source) {
  const result = await transformAsync(source, {
    configFile: false,
    babelrc: false,
    sourceType: "unambiguous",
    plugins: [bisonJsxParserPlugin, transformReactJsx],
  });
  return result.code;
}

test("Babel parses JavaScript and transforms the Bison-produced JSX AST", async () => {
  const output = await compile(`
    import React from "react";
    const button = <Button disabled={loading} {...props}>Save</Button>;
    export function App() {
      return <><h1>{title}</h1><Panel><Icon /></Panel></>;
    }
  `);

  assert.match(output, /const button/);
  assert.match(output, /disabled: loading/);
  assert.match(output, /\.\.\.props/);
  assert.match(output, /children: "Save"/);
  assert.match(output, /Fragment/);
  assert.match(output, /children: title/);
  assert.match(output, /jsx\(Icon/);
});

test("a Bison closing-tag error is surfaced through Babel", async () => {
  await assert.rejects(
    () => compile("const broken = <Panel></Pane>;"),
    /Bison JSX parser:.*does not match/,
  );
});
