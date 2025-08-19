# launcher/hako_launch/hako_asset_runner.py
from __future__ import annotations

import os, sys, time, signal, subprocess, json
from dataclasses import dataclass
from typing import Mapping, Optional, Iterable, Union
from os import PathLike

IS_POSIX = (os.name == "posix")
IS_WIN   = (os.name == "nt")

PathStr = Union[str, PathLike[str]]

@dataclass
class ExitInfo:
    exited: bool
    exit_code: Optional[int] = None
    signal: Optional[int] = None
    started_at: float = 0.0
    exited_at: Optional[float] = None
    pid: Optional[int] = None

@dataclass
class ProcHandle:
    popen: subprocess.Popen
    started_at: float
    stdout_path: Optional[str] = None
    stderr_path: Optional[str] = None
    _stdout_f: Optional[object] = None   # 後で close するため保持
    _stderr_f: Optional[object] = None

class AssetRunner:
    """単一アセット（=1 OSプロセス）のライフサイクルを扱う最小クラス。"""

    def __init__(self, *, env: Optional[Mapping[str, str]] = None) -> None:
        # envmerge 済みの環境を受け取り、親環境とマージ済み前提（そのまま渡す）
        self.env = dict(os.environ) | dict(env or {})
        self.handle: Optional[ProcHandle] = None

    # ---------- helpers ----------
    @staticmethod
    def _open_sink(path: Optional[PathStr]):
        if path is None:
            return None, None
        path = os.fspath(path)
        parent = os.path.dirname(path)
        if parent:  # "" のとき mkdir しない
            os.makedirs(parent, exist_ok=True)
        # バイナリ新規作成・バッファリング無しで落とす
        f = open(path, "wb", buffering=0)
        return f, path

    @staticmethod
    def _creationflags() -> int:
        if IS_WIN:
            # CTRL_BREAK を送るには NEW_PROCESS_GROUP が必要
            return getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0)  # type: ignore[attr-defined]
        return 0

    # ---------- lifecycle ----------
    def spawn(
        self,
        command: str,
        args: Iterable[str] = (),
        *,
        cwd: Optional[PathStr] = None,
        stdout: Optional[PathStr] = None,
        stderr: Optional[PathStr] = None,
    ) -> ProcHandle:
        if self.handle and self.is_alive():
            raise RuntimeError("process already running")

        out_f, out_path = self._open_sink(stdout)
        err_f, err_path = self._open_sink(stderr)

        popen_kw: dict = dict(
            cwd=(os.fspath(cwd) if cwd is not None else None),
            env=self.env,
            stdout=(out_f or None),
            stderr=(err_f or None),
            creationflags=self._creationflags(),
            close_fds=True,  # FD漏れ防止（Windowsでstdout/errを指定してもTrueでOK）
        )
        if IS_POSIX:  # 後でグループシグナル送るため新PG
            popen_kw["preexec_fn"] = os.setsid

        popen = subprocess.Popen([command, *list(args)], **popen_kw)

        self.handle = ProcHandle(
            popen=popen,
            started_at=time.time(),
            stdout_path=(out_path or None),
            stderr_path=(err_path or None),
            _stdout_f=out_f,
            _stderr_f=err_f,
        )
        return self.handle

    def is_alive(self) -> bool:
        return bool(self.handle) and self.handle.popen.poll() is None  # type: ignore[union-attr]

    def wait(self, *, timeout: Optional[float] = None) -> Optional[int]:
        """終了を待つ（timeout秒以内）。戻り値はreturncode。"""
        if not self.handle:
            return 0
        try:
            return self.handle.popen.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            return None

    def terminate(self, *, grace_sec: float = 5.0) -> None:
        """優雅停止（TERM/CTRL_BREAK）→ 猶予 → 未終了なら kill()。"""
        h = self.handle
        if not h or not self.is_alive():
            self._cleanup_files()
            return
        try:
            if IS_POSIX:
                os.killpg(h.popen.pid, signal.SIGTERM)  # type: ignore[arg-type]
            elif IS_WIN:
                # GUIアプリは CTRL_BREAK を無視することがあるのでフォールバック想定
                h.popen.send_signal(getattr(signal, "CTRL_BREAK_EVENT", signal.SIGTERM))  # type: ignore[attr-defined]
            else:
                h.popen.terminate()
        except Exception:
            self.kill()
            return

        self._wait_for_exit(timeout=grace_sec)
        if self.is_alive():
            self.kill()
        else:
            self._cleanup_files()

    def kill(self) -> None:
        h = self.handle
        if not h or not self.is_alive():
            self._cleanup_files()
            return
        try:
            if IS_POSIX:
                os.killpg(h.popen.pid, signal.SIGKILL)  # type: ignore[arg-type]
            else:
                h.popen.kill()
        finally:
            self._wait_for_exit(timeout=2.0)
            self._cleanup_files()

    def exit_info(self) -> ExitInfo:
        h = self.handle
        if not h:
            return ExitInfo(exited=True, exit_code=None, signal=None, started_at=0.0, exited_at=time.time(), pid=None)

        rc = h.popen.poll()
        info = ExitInfo(
            exited=(rc is not None),
            started_at=h.started_at,
            exited_at=time.time() if rc is not None else None,
            pid=h.popen.pid,
        )
        if rc is None:
            return info
        if IS_POSIX and rc < 0:
            info.signal = -rc
        else:
            info.exit_code = rc
        return info

    # ---------- internals ----------
    def _wait_for_exit(self, *, timeout: float) -> None:
        end = time.time() + timeout
        while time.time() < end:
            if not self.is_alive():
                return
            time.sleep(0.05)

    def _cleanup_files(self) -> None:
        """stdout/stderr のファイルをクローズする（重複クローズ安全）。"""
        h = self.handle
        if not h:
            return
        for fattr in ("_stdout_f", "_stderr_f"):
            f = getattr(h, fattr, None)
            if f:
                try:
                    f.close()
                except Exception:
                    pass
                setattr(h, fattr, None)

# ----------------------
# 単体実行 (__main__)
# ----------------------
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python -m hako_launch.hako_asset_runner <command> [args...] [--grace 2.0]")
        sys.exit(1)

    # 簡易パース
    argv = sys.argv[1:]
    grace = 2.0
    if "--grace" in argv:
        i = argv.index("--grace")
        grace = float(argv[i+1])
        argv = argv[:i] + argv[i+2:]
    cmd, *args = argv

    runner = AssetRunner()
    h = runner.spawn(cmd, args)
    print(f"[runner] spawned PID={h.popen.pid} ({cmd})")

    try:
        while runner.is_alive():
            print("[runner] alive...")
            time.sleep(0.5)
    except KeyboardInterrupt:
        print("[runner] KeyboardInterrupt → terminate")
        runner.terminate(grace_sec=grace)

    info = runner.exit_info()
    print("[runner] exit info:", json.dumps(info.__dict__, indent=2))
