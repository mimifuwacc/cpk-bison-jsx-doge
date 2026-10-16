# JSX parser native

JSX grammar を GNU Bison で生成する C++20 パーサーです。C++ API、JSON を出力する CLI、Babel bridge から利用する Node-API addon を同じ parser library 上に提供します。

主要な入口は `src/parser.y`、公開 AST は `include/jsx/ast.hpp` です。リポジトリルートから `vp run build:native` と `vp run test` でビルド・検証できます。

Node-API 経由で解析されるたびに、標準エラーへ `[Bison JSX] parse:start` と解析結果を出力します。変換後の JavaScript を流す標準出力には混ざりません。
