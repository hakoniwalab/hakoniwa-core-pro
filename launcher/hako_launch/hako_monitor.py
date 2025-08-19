# launcher/hako_launch/hako_monitor.py
from __future__ import annotations

import os
import time
from pathlib import Path
import subprocess
from dataclasses import dataclass
from typing import Dict, List, Optional

from .effective_model import EffectiveSpec, EffectiveAsset
from .envmerge import merge_env
from .hako_asset_runner import AssetRunner

@dataclass
class Running:
    asset: EffectiveAsset
    runner: AssetRunner


def _expand_path(p: Path | str | None, *, asset: str, base_dir: Path) -> str | None:
    if p is None:
        return None
    s = str(p)
    ts = time.strftime("%Y-%m-%dT%H-%M-%S")  # ファイル名に安全なタイムスタンプ
    s = s.replace("${asset}", asset).replace("${timestamp}", ts).replace("${base_dir}", str(base_dir))
    # 相対なら base_dir 起点で絶対化
    return s if os.path.isabs(s) else str((base_dir / s).resolve())

class HakoMonitor:
    """
    全アセットの起動・監視・終了を司るオーケストレータ。
    - 起動順は EffectiveSpec.assets の順（既に depends_on で解決済み）
    - 各アセットは start_grace_sec の間“生存”で安定化とみなす
    - だれか落ちた時点で全体を停止し、notify を発火
    """

    def __init__(self, spec: EffectiveSpec, *, defaults_env_ops: Optional[dict] = None) -> None:
        self.spec = spec
        # loader の設計上、EffectiveAsset.env には「asset個別 or defaults のどちらか」が入っている。
        # defaults と個別を分けて合成したい場合は defaults_env_ops を渡す。
        self.defaults_env_ops = defaults_env_ops
        self.procs: List[Running] = []
        self._aborted = False

    # ---------- public API ----------
    def start_all(self) -> None:
        """
        配列順に起動。各アセットごとに:
          1) env を合成
          2) spawn
          3) start_grace_sec 生存したら安定とみなし、delay_sec だけ待って次へ
        途中で死んだら即 abort。
        """
        for a in self.spec.assets:
            if self._aborted:
                break

            env = merge_env(
                defaults_env=self.defaults_env_ops,   # None の場合は OS 環境 + asset.env のみ適用
                asset_env=(a.env or None),
                asset_name=a.name,
            )
            out_path = _expand_path(a.stdout, asset=a.name, base_dir=self.spec.base_dir)
            err_path = _expand_path(a.stderr, asset=a.name, base_dir=self.spec.base_dir)
            print(f'[INFO] activating {a.name}: (stdout: {out_path}, stderr: {err_path})')
            r = AssetRunner(env=env)
            r.spawn(
                a.command,
                a.args,
                cwd=str(a.cwd),
                stdout=(str(out_path) if out_path else None),
                stderr=(str(err_path) if err_path else None),
            )
            self.procs.append(Running(asset=a, runner=r))

            # 起動安定チェック（Tier0: “猶予時間中に生きていればOK”）
            print(f'[INFO] waiting for {a.name} to stabilize (grace: {a.start_grace_sec} seconds)...')
            if not self._wait_alive(r, a.start_grace_sec):
                self._notify("asset_start_failed", a.name)
                self.abort("start_failed")
                return

            # 次の起動までの待機
            print(f'[INFO] waiting for {a.name} to delay for {a.delay_sec} seconds before next asset...')
            if a.delay_sec > 0:
                time.sleep(a.delay_sec)

    def watch(self) -> None:
        """
        常駐監視。誰か終了したら abort。
        """
        while not self._aborted and self.procs:
            for rp in list(self.procs):
                if not rp.runner.is_alive():
                    self._notify("asset_exit", rp.asset.name)
                    self.abort("asset_exit")
                    return
            time.sleep(0.2)

    def abort(self, reason: str = "abort") -> None:
        """
        逆順で優雅停止（TERM→猶予→KILL）。二重呼び出しは無視。
        """
        if self._aborted:
            return
        self._aborted = True

        # 逆順で止める
        for rp in reversed(self.procs):
            # 猶予は各アセットの start_grace_sec を流用（統一猶予でもOK）
            rp.runner.terminate(grace_sec=rp.asset.start_grace_sec)
        # 念のため最後に KILL 保険（生き残りがいれば）
        for rp in reversed(self.procs):
            if rp.runner.is_alive():
                rp.runner.kill()

    # ---------- internals ----------
    def _wait_alive(self, runner: AssetRunner, grace: float) -> bool:
        """
        猶予時間中にプロセスが落ちないことを確認。True=安定、False=失敗。
        """
        end = time.time() + float(grace)
        while time.time() < end:
            if not runner.is_alive():
                return False
            time.sleep(0.1)
        return True

    def _notify(self, event: str, asset: str) -> None:
        """
        notify 設定（exec/webhook）に従って通知。
        設定が無ければ何もしない。失敗しても監視は続ける。
        """
        n = self.spec.notify
        if not n:
            return
        try:
            if n.get("type") == "exec":
                cmd = [n["command"], *[str(x) for x in n.get("args", [])]]
                # 変数置換（最低限）
                ts = time.strftime("%Y-%m-%dT%H:%M:%S")
                cmd = [s.replace("${asset}", asset).replace("${timestamp}", ts) for s in cmd]
                subprocess.Popen(cmd, close_fds=True)
            elif n.get("type") == "webhook":
                import urllib.request, json as _json
                payload = _json.dumps({"event": event, "asset": asset, "ts": time.time()}).encode("utf-8")
                req = urllib.request.Request(n["url"], data=payload, headers={"Content-Type": "application/json"})
                urllib.request.urlopen(req, timeout=2)  # fire & forget
        except Exception:
            # 通知失敗は致命ではないので握りつぶす（ログは将来のjournalで）
            pass
