# Debianパッケージ仕様

仕様項目：

- [パッケージ名](#package-name)
- [配置場所](#package-location)
- [バージョン番号](#version-number)
- [インストール対象ファイル](#install-target-files)
- [依存関係](#dependencies)
- [テスト](#test)

# package-name

## 現在の仕様

* **ランタイム**
  * `libhakoniwa-assets1`
  * `libhakoniwa-conductor1`
  * `libhakoniwa-shakoc1`
  * `python3-hakopy`
* **開発用**
  * `hakoniwa-core-dev`
* **基盤**
  * `hakoniwa-core`
* **メタパッケージ**
  * `hakoniwa-core-full`

## 仕様定義の理由

* **ランタイムと開発用の分離**
  ユーザーが実行時に不要なヘッダや開発用ファイルを省き、インストール容量と依存関係を最小化するため。当初は開発用パッケージもライブラリごとに分割する案がありましたが、依存関係が複雑になるため単一の`hakoniwa-core-dev`に統合されました。
* **数字付きランタイムパッケージ**
  SONAMEのメジャーバージョンを反映させることで、ABI非互換更新時に旧版と共存可能にするため。
* **CLI・Pythonの独立化**
  利用形態や依存関係が異なるため、更新や配布の柔軟性を確保するため。

## 説明

Debianのパッケージ名はaptでの識別子。
パッケージを用途ごとに分けることで、開発者・実行ユーザー・ツール利用者が必要なものだけをインストールできる。
数字付きランタイムは複数バージョンの共存を可能にする。

---

# package-location

## 現在の仕様

* **共有ライブラリ（公開API）**：`/usr/lib/$(DEB_HOST_MULTIARCH)/`
* **実行時参照ファイル**：`/usr/share/hakoniwa/`
* **共有ライブラリ（内部/dlopen専用）**：`/usr/lib/$(DEB_HOST_MULTIARCH)/hakoniwa-core/`
* **開発用ヘッダ**：`/usr/include/hakoniwa/`
* **CMake Config / pkg-config**：（注：現状は未実装）
  * `/usr/lib/cmake/hakoniwa-core/`
  * `/usr/lib/$(DEB_HOST_MULTIARCH)/pkgconfig/`
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

# version-number

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

# install-target-files

## 現在の仕様

* **ランタイムパッケージ** (`libhakoniwa-*1`): `.so`本体（公開APIのみ）
* **開発用パッケージ** (`hakoniwa-core-dev`): ヘッダ、`.so` 開発リンク、pkgconfig/cmake
* **基盤パッケージ** (`hakoniwa-core`): 
  * 実行ファイル（**/usr/bin/hako-cmd** 等）
  * 実行時参照データ（**/usr/share/hakoniwa/**）
  * 設定（**/etc/hakoniwa/**、あれば）
  * 可変データ用ディレクトリ（**/var/lib/hakoniwa/**）
* **Pythonパッケージ** (`python3-hakopy`):  **/usr/lib/python3/dist-packages/hakoniwa/**（C拡張含む）

## 仕様定義の理由

* 利用者に必要な最小限のファイルだけをインストールするため。
* 開発者はヘッダやリンク用ファイルが必須なため、開発用パッケージに分離。

## 説明

Debianでは用途別にファイルをパッケージ化し、不要な依存や容量増加を避ける。
Pythonモジュールは`python3-*`パッケージとして提供し、OSのPython環境に統合する。

---

# dependencies

## 現在の仕様

**ランタイム・開発用依存（パッケージごと）**

* `libhakoniwa-conductor1`
  * Depends: `${shlibs:Depends}, ${misc:Depends}`

* `libhakoniwa-assets1`
  * Depends: `${shlibs:Depends}, ${misc:Depends}, libhakoniwa-conductor1 (= ${binary:Version})`

* `libhakoniwa-shakoc1`
  * Depends: `${shlibs:Depends}, ${misc:Depends}`

* `hakoniwa-core-dev`
  * Depends: `${misc:Depends}, libhakoniwa-conductor1 (= ${binary:Version}), libhakoniwa-assets1 (= ${binary:Version}), libhakoniwa-shakoc1 (= ${binary:Version})`

* `hakoniwa-core`（基盤）
  * Depends: `${shlibs:Depends}, ${misc:Depends}`
  * Recommends: `libhakoniwa-assets1 | libhakoniwa-shakoc1`

* `python3-hakopy`
  * Depends: `${python3:Depends}, ${shlibs:Depends}, ${misc:Depends}, libhakoniwa-assets1 (= ${binary:Version}), libhakoniwa-conductor1 (= ${binary:Version})`

**ビルド依存**

* Build-Depends: `debhelper-compat (= 13), cmake, dh-exec, help2man, dh-sequence-python3, python3-dev`

## 仕様定義の理由

* **`${shlibs:Depends}` に任せる方針**
  glibc系（`libc6`, `libstdc++6`, `libgcc-s1`, `libm` など）は `dh_shlibdeps` が自動付与するため、明示列挙しない。
* **自前ライブラリの“連鎖”は明示**
  * `libassets.so` は `libconductor.so` にリンクしている → `libhakoniwa-assets1` は **`libhakoniwa-conductor1` に明示依存**。
  * `libhako_asset_python.so` は `libassets.so` と `libconductor.so` の両方に依存 → **両方に明示依存**。
  * `hakoniwa-core-dev` は開発に必要な全てのランタイムライブラリに依存する。
* **CLIは最小依存＋推奨**
  `hako-cmd` は標準C/C++のみ（ldd上）。実行時に assets/shakoc を使うプロジェクトが多い想定なので、**Recommends** で案内するに留める。

## 説明

* 依存関係は **lddの結果**を起点に設計し、**標準ライブラリは自動解決**、**自前ライブラリは明示**が原則。
* `python3-hakopy` は **`dh_python3`** を使うことで、Python ABI に適合した `Depends`（例：`python3 (>= 3.12~)` 等）が自動的に付与される。
* 将来的にリンク構成が変わったら（例：`libassets.so` が `libconductor.so` へリンクしなくなる等）、**この依存を削る/追加する**だけでよい。
* 実行時に `dlopen` で読ませる内部 .so は、公開ランタイムに含めず **私有ディレクトリ**（`/usr/lib/$(DEB_HOST_MULTIARCH)/hakoniwa-core/`）へ置くと、**不要な依存を増やさず**に済む。

> 補足（品質チェック）
>
> * ビルド成果物に **RPATH（ビルドツリーの絶対パス）が残らない**ようにする（`readelf -d` / `chrpath -l` で確認）。
> * これにより、`ldd` にローカルパス（`/home/.../cmake-build/...`）が出る事象を防止。

> **注意**: Python ABI 非互換（例: 3.12 ビルド済を 3.13 で実行など）の場合、実行時エラー（SIGSEGV）が発生します。  
> Debianパッケージはビルド時のPython ABIに一致するよう `${python3:Depends}` により制約します。

---

# test

## 現在の仕様

ローカルビルドした `.deb` を用いたインストール／動作確認／アンインストールの手順を定義する。

## 仕様定義の理由

Debianパッケージの品質保証として、インストール後に最低限の動作確認を行うことで、配布後のトラブル（依存不足・ファイル欠落・バージョン不一致）を早期に発見できる。
また、アンインストールテストも行い、不要ファイルの残留やディレクトリ削除の挙動を確認する。

## 説明

テストは以下の流れで行う。

---

### 1. クリーン環境にパッケージをインストール

ローカル `.deb` を直接インストールする場合：

```bash
sudo apt install -y \
  ./libhakoniwa-conductor1_*.deb \
  ./libhakoniwa-assets1_*.deb \
  ./libhakoniwa-shakoc1_*.deb \
  ./python3-hakopy_*.deb \
  ./hakoniwa-core-dev_*.deb \
  ./hakoniwa-core_*.deb \
  ./hakoniwa-core-full_*.deb
```

> `hakoniwa-core-full` はメタパッケージのため、依存一式を同時にインストールする必要がある。

---

### 2. インストール確認

```bash
# インストール済みパッケージ一覧
apt list --installed | grep -E 'hakoniwa|libhakoniwa'

# メタパッケージの依存確認
apt-cache depends hakoniwa-core-full

# 詳細情報
apt show hakoniwa-core-full
```

---

### 3. CLIの動作確認

```bash
hako-cmd --version
```

→ ビルドバージョンが表示されることを確認。

---

### 4. ライブラリリンク確認

```bash
ldd /usr/bin/hako-cmd
```

→ 依存 `.so` がすべて `/usr/lib/$(dpkg-architecture -qDEB_HOST_MULTIARCH)/` 内に解決されていることを確認。

---

### 5. Pythonモジュール動作確認（任意）

```bash
python3 -c "import hakopy; print(hakopy.__version__)"
```

→ Pythonビルドバージョンと一致することを確認。

---

### 6. アンインストールテスト

#### メタパッケージだけ削除（依存は残す）

```bash
sudo apt remove hakoniwa-core-full
```

#### 依存も含めて全削除

```bash
sudo apt purge hakoniwa-core hakoniwa-core-dev \
  libhakoniwa-conductor1 libhakoniwa-assets1 libhakoniwa-shakoc1 \
  python3-hakopy
sudo apt autoremove
```

> `/var/lib/hakoniwa/mmap` など可変データが残る場合は手動で削除する。
> 完全自動化する場合は `postrm` スクリプトで対応可能。

---