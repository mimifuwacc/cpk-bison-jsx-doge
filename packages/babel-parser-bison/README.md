# Babel parser Bison integration

インストール済み `@babel/parser` から fork を生成し、JSX の解析だけをネイティブ Bison parser に委譲します。`src/parser-plugin.ts` は Babel の `parserOverride` として利用できます。

生成した fork は `build/babel-parser-bison/parser.mjs` に配置されます。`vp run test:integration` で JavaScript、JSX、Babel transform を通した統合テストを実行できます。
