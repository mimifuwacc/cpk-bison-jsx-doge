# cpk-bison-jsx-doge

超ものづくり合同！vol.1 に寄稿した「かぐやがBisonでJSXパーサを作って犬DOGEを実装する話」の実装です．

## 構成

```text
JavaScript / JSX
  └─ Babel parser fork
       ├─ JavaScript → Babel
       └─ JSX → Node-API → Bison/C++ parser → Babel JSX AST
  └─ React JSX transform → JavaScript
  └─ esbuild → React application
```

- `packages/jsx-parser-native/`: Bison/C++ パーサー、CLI、Node-API addon
- `packages/babel-parser-bison/`: Babel fork と parser plugin
- `examples/react-app/`: React のサンプル (犬 DOGE)
- `build/`: ビルド生成物

## 開発用コマンド

```sh
vp run build:native       # ネイティブパーサーをビルド
vp run test               # C++ のテスト
vp run test:integration   # 統合テスト
vp run build:doge         # DOGE サンプルをビルド
vp run format:cxx         # C++ をフォーマット
vp run check:format:cxx   # フォーマットを確認
```
