# 🧭 Hakoniwa Debian Release 手順（簡易版・忘備録）

## 0. 前提

### build
* repo: `hakoniwa-core-pro`
* distro: `noble`
* version例: `1.1.2-1`

### release
* repo: `apt`
* branch: `gh-pages`（`main` ではない）

```
cd ../apt
git checkout gh-pages
```

---

## 0 古いファイルの削除

`hakoniwa-core-pro` の親ディレクトリにある古い `.deb` ファイルを削除しておく。


## 1️⃣ ビルド前準備（core repo）

```bash
cd hakoniwa-core-pro
bash build.bash
```

### 環境変数（毎回忘れるやつ）

```bash
export DEBFULLNAME="Takashi Mori"
export DEBEMAIL="tmori@hakoniwa-lab.net"
```

---

## 2️⃣ changelog 更新

### 新バージョン作成

```bash
dch -v 1.1.2-1 -D noble "new release"
```

changelogは後から編集できるので、コメントは適当でOK。
debian/changelog が変更されるので、それを後から編集する。

### 内容確認・微修正（必要なら）

```bash
dch --edit
```

### リリース確定（※CTRL+Cしない）

```bash
dch -r
```

#### もし失敗したら

```bash
rm -f debian/changelog.dch
rm -f debian/.changelog.dch.swp
dch -r
```

---

## 3️⃣ Debian パッケージビルド

```bash
debuild -b -us -uc
```

生成物は **親ディレクトリ** に出る。

---

## 4️⃣ ゴミ混入チェック（macOS対策）

```bash
find . -name ".DS_Store" -print
```

---

## 5️⃣ APT リポジトリ更新（apt repo）

```bash
cd ../apt
rm -f pool/main/*
rm -f dists/stable/main/binary-amd64/Packages*
cp ../*.deb pool/main/
```

```bash
dpkg-scanpackages --arch amd64 pool/main > dists/stable/main/binary-amd64/Packages
gzip -kf dists/stable/main/binary-amd64/Packages
touch .nojekyll
```

```bash
git add -A
git commit -m "release: hakoniwa-core 1.1.2-1"
git push
```

---

## 6️⃣ APT 経由アップデート確認（最重要）

```bash
sudo apt update
apt-cache policy hakoniwa-core-full
```

### アップデート実行

```bash
sudo apt upgrade
```

### 確認

```bash
dpkg -l | grep hakoniwa
```

---

## 7️⃣ 合格判定 ✅（これだけ見ればOK）

* `1.1.2-1` が全部入っている
* `apt upgrade` で remove が出ない
* 「インストールできませんでした」が出ない

👉 **lintian は今回は見ない**

---

## 📝 メモ（運用ルール）

* PR者と合意済みなら lintian E/W は次回対応でOK
* Python `.so` の命名は PEP 3149 前提
* 判断基準は **ユーザーが install/upgrade できるか**
