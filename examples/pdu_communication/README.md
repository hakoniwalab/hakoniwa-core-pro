# pdu_communication サンプル

このサンプルでは、PDUデータの基本的な通信方法を学びます。箱庭アセットAPIを使用して、箱庭アセット間のデータ交換方法について説明します。

## サンプルプログラムの概要

本サンプルプログラムをビルドすると、`asset_plant` と　`asset_controller`　という２つのバイナリが生成されます。

`asset_plant` は、モーター指示値を読み込み、プラントモデルの観測データを出力します。

`asset_controller` は、プラントモデルの観測データを読み込み、モーター指示値を出力します。

本サンプルでは、簡単のため、制御プログラムおよびプラントの数値計算プログラムは含めず、PDUデータのやり取りがわかるようにしています。

## ファイル内訳

本サンプルでは、以下のファイルを用意しています。

* src/asset_plant.c
* src/asset_controller.c
* src/pdu_info.h
* custom.json

## ファイルの役割

* `asset_plant.c` は、箱庭アセットであり、プラントモデル側の役割を想定したものです。
* `asset_controller.c` は、箱庭アセットであり、制御側の役割を想定したものです。
* `pdu_info.h` は、箱庭アセット間の通信データであるPDUデータを定義したものです。
  * PDUデータそのものは、`geometry_msgs/pdu_ctype_Twist.h`のデータ型を利用しています。
  * また、そのデータのインスタンスは２つあります。
    * PDU_MOTOR_CHANNLE_ID：プラントモデルへの指示値
    * PDU_POS_CHANNEL_ID：プラントモデルの観測データ
* `custom.json`は、上記のPDUデータを箱庭アセットAPIに理解してもらうためのコンフィグファイルです。


## 動作手順

本サンプルプログラムを実行するには、３つの端末を利用します。

* 端末A：asset_plant実行用
* 端末B：asset_controller実行用
* 端末C：箱庭コマンド実行用

**端末Aでの手順：**

hakoniwa-core-cpp-clientでビルドおよびインストール完了後、以下のコマンドを実行します。

```sh
./cmake-build/examples/pdu_communication/asset_plant  examples/pdu_communication/custom.json
```

成功すると、以下のログが出力され、待機状態になります。

```sh
INFO: shmget() key=255 size=1129352 
INFO: hako_conductor thread start
Robot: Robot, PduWriter: Robot_pos
channel_id: 1 pdu_size: 48
INFO: Robot create_lchannel: logical_id=1 real_id=0 size=48
INFO: asset(Writer) is registered.
WAIT START
```

**端末Bでの手順：**

以下のコマンドを実行します。

```sh
./cmake-build/examples/pdu_communication/asset_controller examples/pdu_communication/custom.json 
```

成功すると、以下のログが出力され、待機状態になります。

```sh
Robot: Robot, PduWriter: Robot_motor
channel_id: 0 pdu_size: 48
INFO: Robot create_lchannel: logical_id=0 real_id=1 size=48
INFO: asset(Reader) is registered.
WAIT START
```

**端末Cでの手順:**

箱庭アセットのシミュレーションを開始するためには、別の端末Bで、以下のコマンドを実行します。

```sh
hako-cmd start
```

成功すると、端末AとBとで以下のようにログ出力されます。出力されているログの内容は、各種コールバックで実装されているものです。


端末A：プラントモデル側

```sh
1000000: motor data(1001.000000, 1002.000000, 1003.000000)
3000000: motor data(1002.000000, 1003.000000, 1004.000000)
5000000: motor data(1003.000000, 1004.000000, 1005.000000)
7000000: motor data(1004.000000, 1005.000000, 1006.000000)
9000000: motor data(1005.000000, 1006.000000, 1007.000000)
11000000: motor data(1006.000000, 1007.000000, 1008.000000)
13000000: motor data(1007.000000, 1008.000000, 1009.000000)
15000000: motor data(1008.000000, 1009.000000, 1010.000000)
17000000: motor data(1009.000000, 1010.000000, 1011.000000)
19000000: motor data(1010.000000, 1011.000000, 1012.000000)
21000000: motor data(1011.000000, 1012.000000, 1013.000000)
```

端末B：制御側

```sh
1000000: pos data(1.000000, 2.000000, 3.000000)
3000000: pos data(2.000000, 3.000000, 4.000000)
5000000: pos data(3.000000, 4.000000, 5.000000)
7000000: pos data(4.000000, 5.000000, 6.000000)
9000000: pos data(5.000000, 6.000000, 7.000000)
11000000: pos data(6.000000, 7.000000, 8.000000)
13000000: pos data(7.000000, 8.000000, 9.000000)
15000000: pos data(8.000000, 9.000000, 10.000000)
17000000: pos data(9.000000, 10.000000, 11.000000)
19000000: pos data(10.000000, 11.000000, 12.000000)
21000000: pos data(11.000000, 12.000000, 13.000000)
```

次に、以下のコマンドで、シミュレーションを停止およびリセットします。


```sh
hako-cmd stop
hako-cmd reset
```

そうすると、端末AとBとでそれぞれ、以下のログが出力され、サンプルプログラムが終了します。

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


