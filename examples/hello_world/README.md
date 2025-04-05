# Hello world サンプル

このサンプルでは、箱庭アセットの基本的な作成方法を学びます。箱庭アセットAPIを使用して、アセットの起動と各種コールバック関数の実装方法について説明します。

## コード説明

ここでは、主なコードの部分について説明します。サンプルコードでは、以下の主要な機能が実装されています：

* 箱庭アセットの登録と初期化
* コールバック関数の実装（初期化、シミュレーションステップ、リセット処理など）
* シミュレーションの開始と終了
* コード内の各セクションには、その機能や役割についてのコメントが含まれています。

## 動作手順

**端末Aでの手順：**

hakoniwa-core-cpp-clientでビルドおよびインストール完了後、以下のコマンドを実行します。

```sh
./cmake-build/examples/hello_world/hello_world HelloWorld ./examples/hello_world/custom.json 100
```

成功すると、以下のログが出力され、待機状態になります。

```sh
INFO: shmget() key=255 size=1129352 
INFO: hako_master thread start
Robot: ROBOT, PduWriter: ROBOT_motor
channel_id: 0 pdu_size: 48
INFO: ROBOT create_lchannel: logical_id=0 real_id=0 size=48
INFO: asset(HelloWorld) is registered.
WAIT START
```

**端末Bでの手順:**

箱庭アセットのシミュレーションを開始するためには、別の端末Bで、以下のコマンドを実行します。

```sh
hako-cmd start
```

成功すると、端末Aで以下のようにログ出力されます。出力されているログの内容は、各種コールバックで実装されているものです。


```sh
WAIT RUNNING
START CREATE PDU DATA: total_size= PDU CREATED
48
INFO: shmget() key=256 size=48 
PDU DATA CREATED
CREATED ADDR=0x10428400c
INFO: my_on_initialize enter
INFO: sleep 1sec
INFO: my_on_initialize exit
INFO: start simulation
SYNC MODE: true
INFO: hako_asset_impl_pdus_write_done() Robot: ROBOT, PduWriter: ROBOT_motor
channel_id: 0 pdu_size: 48
INFO: on_simulation_step enter: 100000
INFO: sleep 1sec
INFO: on_simulation_step exit
INFO: on_simulation_step enter: 200000
INFO: sleep 1sec
INFO: on_simulation_step exit
INFO: on_simulation_step enter: 300000
INFO: sleep 1sec
INFO: on_simulation_step exit
INFO: on_simulation_step enter: 400000
INFO: sleep 1sec
INFO: on_simulation_step exit
INFO: on_simulation_step enter: 500000
INFO: sleep 1sec
INFO: on_simulation_step exit
```

次に、以下のコマンドで、シミュレーションを停止およびリセットします。


```sh
hako-cmd stop
hako-cmd reset
```

そうすると、以下のログが出力され、サンプルプログラムが終了します。

```sh
NOT RUNNING: curr = 3
WAIT STOP
INFO: my_on_reset enter
INFO: sleep 1sec
INFO: my_on_reset exit
INFO: stopped simulation
INFO: hako_asset_start() returns 4
EVENT: reset
```
