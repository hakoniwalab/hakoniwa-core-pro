#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time


def wait_until_ready(process: subprocess.Popen[str], timeout_sec: float = 10.0) -> None:
    deadline = time.monotonic() + timeout_sec
    assert process.stdout is not None
    while time.monotonic() < deadline:
        line = process.stdout.readline()
        if line == "":
            raise AssertionError(f"lock holder exited early: rc={process.poll()}")
        if line.strip() == "READY":
            return
    raise AssertionError("lock holder did not become ready")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--hako-cmd", required=True)
    parser.add_argument("--holder", required=True)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="hako-cmd-lock-") as temp_dir:
        root = Path(temp_dir)
        config_path = root / "cpp_core_config.json"
        config_path.write_text(
            json.dumps({"shm_type": "mmap", "core_mmap_path": str(root)}),
            encoding="utf-8",
        )
        env = dict(os.environ)
        env["HAKO_CONFIG_PATH"] = str(config_path)

        holder = subprocess.Popen(
            [args.holder],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            env=env,
        )
        try:
            wait_until_ready(holder)

            started = time.monotonic()
            timed = subprocess.run(
                [args.hako_cmd, "ls", "--lock-timeout-ms", "100"],
                capture_output=True,
                text=True,
                env=env,
                timeout=3,
                check=False,
            )
            elapsed = time.monotonic() - started
            if timed.returncode == 0 or "file-lock wait timed out" not in timed.stderr:
                raise AssertionError(
                    f"expected bounded lock timeout, rc={timed.returncode}, "
                    f"stdout={timed.stdout!r}, stderr={timed.stderr!r}"
                )
            if elapsed >= 2.0:
                raise AssertionError(f"lock timeout took too long: {elapsed:.3f}s")

            if sys.platform != "win32":
                interrupted = subprocess.Popen(
                    [args.hako_cmd, "ls", "--lock-timeout-ms", "5000"],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    env=env,
                )
                time.sleep(0.2)
                interrupted.send_signal(signal.SIGTERM)
                stdout, stderr = interrupted.communicate(timeout=3)
                expected = 128 + signal.SIGTERM
                if interrupted.returncode != expected:
                    raise AssertionError(
                        f"expected SIGTERM exit {expected}, rc={interrupted.returncode}, "
                        f"stdout={stdout!r}, stderr={stderr!r}"
                    )
                if "interrupted while waiting for a file lock" not in stderr:
                    raise AssertionError(f"missing interrupted diagnostic: {stderr!r}")
        finally:
            if holder.poll() is None:
                holder.terminate()
                try:
                    holder.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    holder.kill()
                    holder.wait(timeout=3)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
