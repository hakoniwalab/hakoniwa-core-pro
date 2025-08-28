from __future__ import annotations
import sys
import os
import argparse
import signal
import threading
import time
import asyncio
import logging
from typing import Optional

from .loader import load
from .hako_monitor import HakoMonitor
from .hako_cli import HakoCli

class LauncherService:
    """起動・監視・停止を状態付きで提供。"""

    def __init__(self, *, launch_path: str) -> None:
        self.launcher_spec, self.spec = load(launch_path)  # (LauncherSpec, EffectiveSpec)
        self.defaults_env_ops = None
        if self.launcher_spec.defaults and self.launcher_spec.defaults.env:
            self.defaults_env_ops = self.launcher_spec.defaults.env.model_dump(exclude_none=True)

        self.monitor = HakoMonitor(self.spec, defaults_env_ops=self.defaults_env_ops)
        self.cli     = HakoCli(spec=self.spec, defaults_env_ops=self.defaults_env_ops)

        self.state: str = "IDLE"
        self._watch_thread: Optional[threading.Thread] = None
        self._stop_watch = threading.Event()

    # -------- 状態遷移API --------
    def activate(self) -> None:
        if self.state not in ("IDLE", "TERMINATED"):
            print(f"[launcher] activate: invalid state={self.state}", file=sys.stderr)
            return
        # activate()は冪等ではない。再実行の場合はモニターを再生成する
        if self.state == "TERMINATED":
            self.monitor = HakoMonitor(self.spec, defaults_env_ops=self.defaults_env_ops)

        print("[INFO] activating all assets...")
        self.monitor.start_all()
        self._stop_watch.clear()
        self._watch_thread = threading.Thread(target=self._watch_loop, daemon=True)
        self._watch_thread.start()
        self.state = "ACTIVATED"
        print("[INFO] state -> ACTIVATED")

    def cmd(self, command: str) -> int:
        if self.state not in ("ACTIVATED", "RUNNING", "STOPPED"):
            print(f"[launcher] start: invalid state={self.state}", file=sys.stderr)
            return 2
        print(f"[INFO] starting simulation (hako-cmd {command})...")
        if command not in ("start", "stop", "reset"):
            return 1
        rc = 1
        match command:
            case "start":
                rc = self.cli.start()
                self.state = "RUNNING"
                print(f"[INFO] hako-cmd start exited with {rc}")
            case "stop":
                rc = self.cli.stop()
                self.state = "STOPPED"
                print(f"[INFO] hako-cmd stop exited with {rc}")
            case "reset":
                rc = self.cli.reset()
                self.state = "ACTIVATED"
                print(f"[INFO] hako-cmd reset exited with {rc}")
        return rc

    def terminate(self) -> None:
        if self.state in ("TERMINATED", "IDLE"):
            print(f"[launcher] terminate: already {self.state}")
            return
        print("[INFO] terminating all assets...")
        self.monitor.abort("terminate")
        self._stop_watch.set()
        self.state = "TERMINATED"
        print("[INFO] state -> TERMINATED")

    def status(self) -> str:
        return self.state

    # -------- 内部：監視ループ（非ブロッキング） --------
    def _watch_loop(self):
        try:
            while not self._stop_watch.is_set() and self.monitor.procs:
                for rp in list(self.monitor.procs):
                    if not rp.runner.is_alive():
                        print(f"[WARN] asset exited: {rp.asset.name} -> abort all")
                        self.monitor.abort("asset_exit")
                        self.state = "TERMINATED"
                        self._stop_watch.set()
                        return
                time.sleep(0.2)
        except Exception as e:
            print(f"[watch] exception: {e}", file=sys.stderr)
            self.monitor.abort("watch_exception")
            self.state = "TERMINATED"
            self._stop_watch.set()

# -------- CLI エントリ --------

def _install_sigint(service: LauncherService):
    def _sigint_handler(signum, frame):
        print("[launcher] SIGINT received → aborting...")
        try:
            service.terminate()
        finally:
            sys.exit(1)
    signal.signal(signal.SIGINT, _sigint_handler)

async def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Hakoniwa Launcher")
    parser.add_argument("launch_file", help="Path to launcher JSON")
    parser.add_argument(
        "--mode",
        choices=["immediate", "serve"],
        default="immediate",
        help="immediate: activate→start→watch / serve: 待機して外部コマンドを受け付ける",
    )
    parser.add_argument("--no-watch", action="store_true",
                        help="(immediate時) 監視せず起動だけして終了")
    args = parser.parse_args(argv)

    # Setup logging
    if os.environ.get('HAKO_PDU_DEBUG') == '1':
        log_level = logging.DEBUG
    else:
        log_level = logging.INFO

    logging.basicConfig(
        level=log_level,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        stream=sys.stdout
    )

    try:
        service = LauncherService(launch_path=args.launch_file)
    except Exception as e:
        print(f"[launcher] Failed to load spec: {e}", file=sys.stderr)
        return 1

    _install_sigint(service)

    print("[INFO] HakoLauncher ready. assets:")
    for a in service.spec.assets:
        print(f" - {a.name} (cwd={a.cwd}, cmd={a.command}, args={a.args})")

    if args.mode == "immediate":
        try:
            service.activate()
            rc = service.cmd("start")
            if not args.no_watch:
                while service.status() not in ("TERMINATED",):
                    time.sleep(0.5)
            return 0 if rc == 0 else rc
        except Exception as e:
            print(f"[launcher] Exception: {e}", file=sys.stderr)
            service.terminate()
            return 1

    elif args.mode == "serve":
        print("[INFO] serve mode. commands: activate | start | stop | reset | terminate | status | quit")
        while True:
            try:
                sys.stdout.write("> ")
                sys.stdout.flush()
                line = sys.stdin.readline()
                if not line:
                    break
                cmd = line.strip().lower()
                if cmd == "activate":
                    service.activate()
                elif cmd == "start":
                    service.cmd("start")
                elif cmd == "stop":
                    service.cmd("stop")
                elif cmd == "reset":
                    service.cmd("reset")
                elif cmd == "terminate":
                    service.terminate()
                elif cmd == "status":
                    print(service.status())
                elif cmd in ("quit", "exit"):
                    service.terminate()
                    break
                elif cmd == "":
                    continue
                else:
                    print(f"unknown command: {cmd}")
            except KeyboardInterrupt:
                service.terminate()
                break
            except Exception as e:
                print(f"[serve] error: {e}", file=sys.stderr)

    return 0

if __name__ == "__main__":
    asyncio.run(main())
