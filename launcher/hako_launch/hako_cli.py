from __future__ import annotations

import sys
import shutil
import subprocess
from typing import Optional, Literal

from .effective_model import EffectiveSpec
from .envmerge import merge_env


class HakoCli:
    """
    hako-cmd を前面実行（フォアグラウンド）で叩く薄いラッパー。
    - PATH は defaults.env を OS 環境にマージして解決
    - 実行は launch.json のディレクトリ（base_dir）をカレントにして行う
    - 出力は親プロセスにそのまま流す（capture しない）
    """

    def __init__(
        self,
        spec: EffectiveSpec,
        *,
        defaults_env_ops: Optional[dict] = None,
        cmd: str = "hako-cmd",
    ) -> None:
        self.spec = spec
        self.defaults_env_ops = defaults_env_ops  # PATH/lib_path はここで合成
        self.cmd = cmd

    # ---- public ----
    def start(self, *, timeout: Optional[float] = None) -> int:
        return self._run("start", timeout=timeout)

    def stop(self, *, timeout: Optional[float] = None) -> int:
        return self._run("stop", timeout=timeout)

    def reset(self, *, timeout: Optional[float] = None) -> int:
        return self._run("reset", timeout=timeout)

    # ---- internals ----
    def _run(self, subcmd: Literal["start", "stop", "reset"], *, timeout: Optional[float]) -> int:
        # env は defaults.env のみ（アセット個別は関係ない）
        env = merge_env(defaults_env=self.defaults_env_ops, asset_env=None, asset_name="hako_cli")

        # which でコマンド確認（PATH は合成済み env で探索）
        resolved = shutil.which(self.cmd, path=env.get("PATH"))
        if resolved is None:
            raise FileNotFoundError(
                f"'{self.cmd}' が見つかりません。PATH を確認してください "
                f"(現在の PATH 先頭: { (env.get('PATH') or '').split(':')[0] if env.get('PATH') else '<empty>' })"
            )

        # 前面実行：親の stdout/stderr を引き継ぐ
        try:
            proc = subprocess.run(
                [self.cmd, subcmd],
                cwd=str(self.spec.base_dir),
                env=env,
                check=False,
                timeout=timeout,
            )
            return int(proc.returncode)
        except subprocess.TimeoutExpired:
            # タイムアウトも呼び出し側で扱いやすいよう非例外で返す
            return 124  # bash 由来の慣例（timeout の終了コード）
