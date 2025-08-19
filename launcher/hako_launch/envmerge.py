# launcher/hako_launch/envmerge.py
from __future__ import annotations

import os
import platform
from datetime import datetime
from typing import Dict, Iterable, Mapping, Optional

# ---- lib_path の実キー解決 ----
def _lib_env_key() -> str:
    sysname = platform.system().lower()
    if sysname == "darwin":
        return "DYLD_LIBRARY_PATH"
    if sysname == "linux":
        return "LD_LIBRARY_PATH"
    # windows などは PATH で DLL/EXE 検索
    return "PATH"

# ---- テンプレート展開: ${asset}, ${timestamp}, ${ENV:FOO} ----
def _expand(s: str, *, asset: Optional[str], os_env: Mapping[str, str], timestamp: Optional[str]) -> str:
    if s is None:
        return s
    out = s
    if asset is not None:
        out = out.replace("${asset}", asset)
    if timestamp is not None:
        out = out.replace("${timestamp}", timestamp)
    # ${ENV:FOO}
    # 簡易置換（存在しないときは空文字）
    idx = 0
    while True:
        start = out.find("${ENV:", idx)
        if start == -1:
            break
        end = out.find("}", start)
        if end == -1:
            break
        key = out[start + 6 : end]  # ENV: の後
        repl = os_env.get(key, "")
        out = out[:start] + repl + out[end + 1 :]
        idx = start + len(repl)
    return out

def _expand_list(values: Iterable[str], *, asset: Optional[str], os_env: Mapping[str, str], timestamp: Optional[str]) -> list[str]:
    ts = timestamp or datetime.now().isoformat(timespec="seconds")
    return [_expand(v, asset=asset, os_env=os_env, timestamp=ts) for v in values]

# ---- PATH 風の結合（重複除去） ----
def _join_pathlike(items: Iterable[str], *, base: str = "") -> str:
    sep = os.pathsep
    seen = set()
    out: list[str] = []
    # 既存 base を先頭に展開
    if base:
        for p in base.split(sep):
            if p and p not in seen:
                seen.add(p)
                out.append(p)
    # 新規 items を後段に追加（同一は弾く）
    for p in items:
        if p and p not in seen:
            seen.add(p)
            out.append(p)
    return sep.join(out)

# ---- envOps を一段適用 ----
def _apply_ops(
    base_env: Dict[str, str],
    env_ops: Optional[Mapping],
    *,
    asset_name: Optional[str],
    now_iso: Optional[str],
) -> Dict[str, str]:
    if not env_ops:
        return base_env

    result = dict(base_env)
    sep = os.pathsep
    # 置換コンテキスト
    def expand_str(s: str) -> str:
        return _expand(s, asset=asset_name, os_env=result, timestamp=now_iso)

    def expand_list(xs: Iterable[str]) -> list[str]:
        return _expand_list(xs, asset=asset_name, os_env=result, timestamp=now_iso)

    # 1) unset
    unset = env_ops.get("unset")
    if unset:
        for k in unset:
            result.pop(k, None)

    # 2) set
    set_map = env_ops.get("set")
    if set_map:
        for k, v in set_map.items():
            result[k] = expand_str(v)

    # 3) prepend
    pre = env_ops.get("prepend")
    if pre:
        for k, vs in pre.items():
            real_key = _lib_env_key() if k == "lib_path" else k
            values = expand_list(vs)
            cur = result.get(real_key, "")
            # prepend は「新規 + 既存」
            result[real_key] = _join_pathlike(values + ([] if not cur else [cur]), base="")

    # 4) append
    app = env_ops.get("append")
    if app:
        for k, vs in app.items():
            real_key = _lib_env_key() if k == "lib_path" else k
            values = expand_list(vs)
            cur = result.get(real_key, "")
            # append は「既存 + 新規」
            result[real_key] = _join_pathlike(values=[], base=cur)
            result[real_key] = _join_pathlike(values, base=result[real_key])

    return result

# ---- パブリックAPI ----
def merge_env(
    *,
    os_env: Optional[Mapping[str, str]] = None,
    defaults_env: Optional[Mapping] = None,
    asset_env: Optional[Mapping] = None,
    asset_name: Optional[str] = None,
) -> Dict[str, str]:
    """
    環境変数を合成して返す。
      1) OS 環境（既存）をベース
      2) defaults の envOps を適用
      3) asset の envOps で上書き適用
    """
    base = dict(os_env or os.environ)
    now_iso = datetime.now().isoformat(timespec="seconds")

    base = _apply_ops(base, defaults_env, asset_name=asset_name, now_iso=now_iso)
    base = _apply_ops(base, asset_env, asset_name=asset_name, now_iso=now_iso)
    return base


# ---- 動作確認：python -m hako_launch.envmerge ----
if __name__ == "__main__":
    # 例: defaults で lib_path を足し、asset でさらに追加＆ENV 展開
    defaults_env = {
        "prepend": {"lib_path": ["/usr/local/hakoniwa/lib"]},
        "set": {"HAKO_MODE": "dev"},
    }
    asset_env = {
        "append": {"lib_path": ["${ENV:HOME}/.hakoniwa/lib"]},
        "set": {"RUN_ASSET": "${asset}"},
    }
    merged = merge_env(defaults_env=defaults_env, asset_env=asset_env, asset_name="drone")
    key = _lib_env_key()
    print("lib key:", key)
    print(key, "=", merged.get(key, ""))
    print("RUN_ASSET =", merged.get("RUN_ASSET"))
    print("HAKO_MODE =", merged.get("HAKO_MODE"))
