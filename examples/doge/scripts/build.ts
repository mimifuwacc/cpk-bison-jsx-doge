import { mkdir, readFile, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { transformAsync } from "@babel/core";
import transformReactJsx from "@babel/plugin-transform-react-jsx";
import { build } from "esbuild";
import bisonJsxParserPlugin from "../../../packages/babel-parser-bison/src/parser-plugin.ts";

const entry = resolve("examples/doge/src/App.jsx");
const outputDirectory = resolve("examples/doge/dist");

const source = await readFile(entry, "utf8");

const transformed = await transformAsync(source, {
  filename: entry,
  configFile: false,
  babelrc: false,
  sourceType: "module",
  sourceMaps: "inline",
  plugins: [bisonJsxParserPlugin, [transformReactJsx, { runtime: "automatic" }]],
});

await mkdir(outputDirectory, { recursive: true });
await writeFile(
  resolve(outputDirectory, "doge.bmp"),
  await readFile(resolve("examples/doge/assets/doge.bmp")),
);
await build({
  stdin: {
    contents: transformed.code,
    loader: "js",
    resolveDir: dirname(entry),
    sourcefile: entry,
  },
  outfile: resolve(outputDirectory, "app.js"),
  bundle: true,
  format: "esm",
  platform: "browser",
  target: ["es2022"],
  sourcemap: true,
});

await writeFile(
  resolve(outputDirectory, "index.html"),
  await readFile(resolve("examples/doge/index.html")),
);
await writeFile(
  resolve(outputDirectory, "styles.css"),
  await readFile(resolve("examples/doge/styles.css")),
);
console.log("Built React demo through the Babel/Bison JSX pipeline");
