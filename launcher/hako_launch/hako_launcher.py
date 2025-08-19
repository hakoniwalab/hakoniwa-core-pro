# launcher/hako_launch/hako_launcher.py
from __future__ import annotations
import sys
import argparse
import signal

from .loader import load
from .hako_monitor import HakoMonitor
from .model import LauncherSpec
from .effective_model import EffectiveSpec
from .hako_cli import HakoCli

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Hakoniwa Launcher CLI")
    parser.add_argument("launch_file", help="Path to launcher JSON")
    parser.add_argument("--no-watch", action="store_true", help="Start assets then exit (no monitoring)")
    args = parser.parse_args(argv)

    try:
        launcher_spec, effective_spec = load(args.launch_file)
    except Exception as e:
        print(f"[launcher] Failed to load spec: {e}", file=sys.stderr)
        return 1

    defaults_env_ops = None
    if launcher_spec.defaults and launcher_spec.defaults.env:
        defaults_env_ops = launcher_spec.defaults.env.model_dump(exclude_none=True)
    monitor = HakoMonitor(effective_spec, defaults_env_ops=defaults_env_ops)

    def _sigint_handler(signum, frame):
        print("[launcher] SIGINT received → aborting...")
        monitor.abort("sigint")
        sys.exit(1)

    signal.signal(signal.SIGINT, _sigint_handler)

    print(f'[INFO] HakoLauncher started: assets:')
    for asset in effective_spec.assets:
        print(f' - {asset.name}')
        print(f'   env: {asset.env}')
        print(f'   cwd: {asset.cwd}')
        print(f'   cmd: {asset.command}')
        print(f'   args: {asset.args}')

    try:
        monitor.start_all()
        cli  = HakoCli(spec=effective_spec, defaults_env_ops=monitor.defaults_env_ops)
        cli.start()
        if not args.no_watch:
            monitor.watch()
    except Exception as e:
        print(f"[launcher] Exception: {e}", file=sys.stderr)
        monitor.abort("exception")
        return 1

    return 0

if __name__ == "__main__":
    sys.exit(main())
