# hakoniwa-core-pro

hakoniwa-core-pro は、箱庭シミュレーションフレームワーク **hakoniwa-core-cpp** を拡張し、
イベント駆動型のシミュレーションを実現するためのライブラリ群です。C/C++/Python で
記述されたアセットを組み合わせることで、ロボット制御やセンサシミュレーションなどの
開発・検証を行うことができます。

## 構成

- `sources/`    コアライブラリおよびアセット用モジュール
- `examples/`   サンプルプログラム
- `tests/`      Python 版バインディングのテストコード

## ビルド方法

```sh
./build.bash
```

デフォルトでは `cmake-build` 配下にビルド成果物が生成されます。初回のみ
`git submodule update --init --recursive` を実行してサブモジュールを取得してください。

## 実行方法

ビルド後、サンプルを以下のように実行できます。

```sh
./run-example.bash ./cmake-build/examples/hello_world/hello_world
```

詳細な手順は `examples/hello_world/README.md` など各サンプルの README を参照してください。

## ライセンス

このプロジェクトは MIT License の下で公開されています。詳しくは `LICENSE` ファイルを参照してください。
