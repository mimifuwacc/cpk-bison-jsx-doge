import { parse } from "../../../build/babel-parser-bison/parser.mjs";

export default function bisonJsxParserPlugin() {
  return {
    name: "bison-jsx-parser",
    parserOverride(code, options) {
      const plugins = options.plugins || [];
      if (!plugins.some((plugin) => (Array.isArray(plugin) ? plugin[0] : plugin) === "jsx")) {
        plugins.push("jsx");
      }
      return parse(code, { ...options, plugins });
    },
  };
}
