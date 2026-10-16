import { readFile } from "node:fs/promises";
import { transformAsync } from "@babel/core";
import transformReactJsx from "@babel/plugin-transform-react-jsx";
import bisonJsxParserPlugin from "./parser-plugin.ts";

const inputFile = process.argv[2];
const source = inputFile
  ? await readFile(inputFile, "utf8")
  : await new Promise<string>((resolve, reject) => {
      let input = "";
      process.stdin.setEncoding("utf8");
      process.stdin.on("data", (chunk) => {
        input += chunk;
      });
      process.stdin.on("end", () => resolve(input));
      process.stdin.on("error", reject);
    });

const result = await transformAsync(source, {
  configFile: false,
  babelrc: false,
  sourceType: "unambiguous",
  plugins: [bisonJsxParserPlugin, transformReactJsx],
});

process.stdout.write(`${result.code}\n`);
