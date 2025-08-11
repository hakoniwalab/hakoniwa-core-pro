# Debianパッケージ仕様

仕様項目：

- [パッケージ名](#パッケージ名)
- [配置場所](#配置場所)
- [バージョン番号](#バージョン番号)
- [インストール対象ファイル](#インストール対象ファイル)
- [依存関係](#依存関係)

# パッケージ名

## 現在の仕様

* **ランタイム**
  `libhakoniwa-assets1`, `libhakoniwa-conductor1`, `libhakoniwa-shakoc1`, `python3-hakopy`
* **開発用**
  `libhakoniwa-assets-dev`, `libhakoniwa-conductor-dev`, `libhakoniwa-shakoc-dev`
* **CLIツール**
  `hakoniwa-core`
* **メタパッケージ（任意）**
  `hakoniwa-core-full`

## 仕様定義の理由

* **ランタイムと開発用の分離**
  ユーザーが実行時に不要なヘッダや開発用ファイルを省き、インストール容量と依存関係を最小化するため。
* **数字付きランタイムパッケージ**
  SONAMEのメジャーバージョンを反映させることで、ABI非互換更新時に旧版と共存可能にするため。
* **CLI・Pythonの独立化**
  利用形態や依存関係が異なるため、更新や配布の柔軟性を確保するため。

## 説明

Debianのパッケージ名はaptでの識別子。
パッケージを用途ごとに分けることで、開発者・実行ユーザー・ツール利用者が必要なものだけをインストールできる。
数字付きランタイムは複数バージョンの共存を可能にする。

---

# 配置場所

## 現在の仕様

* **共有ライブラリ（公開API）**：`/usr/lib/$(DEB_HOST_MULTIARCH)/`
* **共有ライブラリ（内部/dlopen専用）**：`/usr/lib/$(DEB_HOST_MULTIARCH)/hakoniwa-core/`
* **開発用ヘッダ**：`/usr/include/hakoniwa/`
* **CMake Config / pkg-config**：
  `/usr/lib/cmake/hakoniwa-core/`
  `/usr/lib/$(DEB_HOST_MULTIARCH)/pkgconfig/`
* **CLI**：`/usr/bin/`
* **設定ファイル**：`/etc/hakoniwa/`
* **可変データ**：`/var/lib/hakoniwa/`（例：`/var/lib/hakoniwa/mmap`）
* **Python拡張**：`/usr/lib/python3/dist-packages/`

## 仕様定義の理由

* FHS（Filesystem Hierarchy Standard）に準拠して競合を防ぐ。
* `$(DEB_HOST_MULTIARCH)`を利用し、マルチアーキテクチャでの共存を実現。
* 設定・データ・バイナリを分離してアップデート耐性を向上。

## 説明

Debianパッケージ品質の要はファイル配置にある。
ライブラリ・設定・可変データを適切に分けることで、OSアップデートや削除時も安全に管理できる。

---

# バージョン番号

## 現在の仕様

* **Upstreamバージョン**：`1.0.0`（プロジェクトのリリース番号）
* **Debianリビジョン**：`-1`（パッケージ世代）
* **PPAリビジョン**（任意）：`~ppa1`（PPA配布の場合）
* **SONAMEバージョン**：`libhakoniwa-assets.so.1` の「1」

## 仕様定義の理由

* UpstreamとDebianのバージョンを分けることで、ソフト本体の更新とパッケージ構成変更を独立管理できる。
* SONAMEバージョンはABI互換性維持のための契約。
* PPA番号は個人ビルドのバージョン管理に使用。

## 説明

Debianのバージョンは `<Upstream>-<Debian>` の形式。
パッケージ構成変更やビルド修正だけならDebian番号のみを上げる。
ABI非互換の変更時はSONAMEとランタイムパッケージ名の両方を更新する。

---

# インストール対象ファイル

## 現在の仕様

* ランタイムパッケージ：`.so`本体、設定ファイル、可変データディレクトリ
* 開発用パッケージ：ヘッダ、`.so`シンボリックリンク、pkg-config/CMakeファイル
* CLIパッケージ：実行ファイル
* Pythonパッケージ：`hakopy`モジュール

## 仕様定義の理由

* 利用者に必要な最小限のファイルだけをインストールするため。
* 開発者はヘッダやリンク用ファイルが必須なため、開発用パッケージに分離。

## 説明

Debianでは用途別にファイルをパッケージ化し、不要な依存や容量増加を避ける。
Pythonモジュールは`python3-*`パッケージとして提供し、OSのPython環境に統合する。

---

# 依存関係

## 現在の仕様

* ランタイムパッケージは`libc6`や必要なシステムライブラリに依存
* 開発用パッケージは対応するランタイムパッケージに依存
* CLIは必要に応じてランタイムやPythonパッケージに依存

## 仕様定義の理由

* 実行時に不足ライブラリがないようにするため。
* 開発用がランタイムに依存することで、インストール後すぐビルド可能にする。

## 説明

依存関係の設計はインストール体験に直結する。
Debianでは`Depends`で必須、`Recommends`で推奨、`Suggests`で任意依存を定義できる。

