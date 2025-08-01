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

## データ受信イベント

hakoniwa-core-pro では、PDU チャンネルのデータ受信を検知する
"データ受信イベント" 機能を提供しています。アセットは
`hako_asset_register_data_recv_event()` を用いることで特定の論理
チャンネルに対する受信イベントを登録でき、データが書き込まれた
タイミングでコールバックが呼び出されます。コールバックを指定し
ない場合はフラグ方式となり、`hako_asset_check_data_recv_event()` で
受信の有無を確認できます。詳細な使い方は
`examples/pdu_communication` 以下のサンプルを参考にしてください。

## RPCサービス

hakoniwa-core-pro では、PDU を介したリクエスト／レスポンス通信を実現する
"RPC サービス" 機能を提供しています。サービス定義を
`hako_asset_service_initialize()` で読み込み、サーバは
`hako_asset_service_server_create()` によりサービスを登録します。
クライアントは `hako_asset_service_client_create()` を呼び出して接続し、
`hako_asset_service_client_get_request_buffer()` でリクエストを準備して
`hako_asset_service_client_call_request()` で送信します。サーバ側では
`hako_asset_service_server_poll()` でリクエスト到着を検知し、
`hako_asset_service_server_get_request()` でデータを取得したのち、
`hako_asset_service_server_get_response_buffer()` と
`hako_asset_service_server_put_response()` を用いて応答を返します。
クライアントは `hako_asset_service_client_poll()` で応答を確認し、
`hako_asset_service_client_get_response()` から結果を取得できます。
詳細な使い方は `examples/service` 以下のサンプルを参照してください。

## ライセンス

このプロジェクトは MIT License の下で公開されています。詳しくは `LICENSE` ファイルを参照してください。
