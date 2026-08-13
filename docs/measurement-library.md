# Hakoniwa Measurement Library

`hakoniwa_measurement`は、既存の箱庭アセットから関数として呼び出すPython測定部品です。
新しい箱庭アセットや監視プロセスを起動しません。

## 責務

- `HakoniwaTimeObserver`
  - `world time -> minimum Asset time -> world time`の順に取得します。
  - 前後のworld timeが一致するsampleだけを採用します。
  - 観測値はcore-to-slowest-participating-Asset lagであり、Drone process間の厳密な時刻差ではありません。
- `MachineResourceMonitor`
  - wall-clock周期でhost CPUとhost memoryを取得します。
  - OS固有処理は`platform/linux`、`platform/macos`、`platform/windows`へ分離します。
- `SimulationExecutionMeter`
  - virtual-time eventで指定された測定区間のworld timeとmonotonic wall clockを記録します。
  - step数はworld timeの進み幅をworld step幅で割って算出します。
- `MeasurementResultSet`
  - 1 runのsummaryとvalidation結果を保持します。
  - `success`、`failed`、`invalid`を区別します。

## 出力契約

- `result.json`: 1 runのsummaryとvalidation
- `machine-samples.jsonl`: Machine resourceのraw時系列
- `temporal-samples.jsonl`: Temporal observerのraw時系列

`JsonLinesWriter`は既存ファイルを開かず、前回trialへの追記を拒否します。
`write_json_atomic`も既存resultをデフォルトで置換しません。Trial directoryの作成、削除、
`.incomplete`から完了directoryへのcommitは呼び出し側のworkspace managerが担当します。

## 実行モード

Official performance runでは、`MachineResourceMonitor`と
`SimulationExecutionMeter`を使用し、`HakoniwaTimeObserver`を生成しません。

Dedicated Temporal Validationでは`HakoniwaTimeObserver`を有効化します。このrunで取得した
wall-clock値はofficial performance resultへ混在させません。

## インストール

`hakoniwa_measurement`は`hakopy`と同じPython install directoryへCMakeでインストールされ、
Core Component Receiptのartifact directoryとして宣言されます。
