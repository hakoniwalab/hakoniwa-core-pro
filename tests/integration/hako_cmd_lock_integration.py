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


def command_json(command: list[str], env: dict[str, str]) -> tuple[int, dict]:
    result = subprocess.run(
        command, capture_output=True, text=True, env=env,
        timeout=5, check=False,
    )
    try:
        payload = json.loads(result.stdout)
    except json.JSONDecodeError as error:
        raise AssertionError(
            f"invalid JSON, rc={result.returncode}, stdout={result.stdout!r}, "
            f"stderr={result.stderr!r}"
        ) from error
    return result.returncode, payload


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

            rc, check = command_json([args.hako_cmd, "check"], env)
            if (rc != 0 or check["status"] != "OK" or
                    check["checks"]["master_lock"]["state"] != "LOCKED"):
                raise AssertionError(f"live lock check failed: rc={rc}, json={check!r}")

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

        rc, check = command_json([args.hako_cmd, "check"], env)
        if rc != 2 or check["status"] != "ABORT":
            raise AssertionError(f"abandoned lock was not detected: rc={rc}, json={check!r}")

        rc, recovered = command_json([args.hako_cmd, "recover"], env)
        if rc != 0 or recovered["status"] != "RECOVERED":
            raise AssertionError(f"recovery failed: rc={rc}, json={recovered!r}")
        if (root / "flock.bin").exists() or (root / "mmap-0xff.bin").exists():
            raise AssertionError("known runtime files remain after recovery")

        (root / "unrelated.txt").write_text("keep", encoding="utf-8")
        rc, recovered = command_json([args.hako_cmd, "recover"], env)
        if rc != 0 or not (root / "unrelated.txt").exists():
            raise AssertionError("recovery touched an unrelated file")

        os_holder = subprocess.Popen(
            [args.holder, "--os-lock"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            env=env,
        )
        try:
            wait_until_ready(os_holder)
            started = time.monotonic()
            rc, check = command_json([args.hako_cmd, "check"], env)
            if rc != 3 or check["status"] != "UNKNOWN":
                raise AssertionError(f"try-lock check did not return UNKNOWN: {check!r}")
            if time.monotonic() - started >= 2:
                raise AssertionError("non-blocking check waited for the OS lock")

            started = time.monotonic()
            rc, check = command_json(
                [
                    args.hako_cmd, "check", "--force-wait",
                    "--lock-timeout-ms", "100",
                ],
                env,
            )
            if rc != 3 or check["status"] != "UNKNOWN":
                raise AssertionError(f"bounded check did not return UNKNOWN: {check!r}")
            if time.monotonic() - started >= 2:
                raise AssertionError("bounded check exceeded its outer bound")
        finally:
            os_holder.kill()
            os_holder.wait(timeout=5)

        rc, recovered = command_json([args.hako_cmd, "recover"], env)
        if rc != 0:
            raise AssertionError(f"post-lock recovery failed: {recovered!r}")

        restarted = subprocess.Popen(
            [args.holder], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, env=env,
        )
        try:
            wait_until_ready(restarted)
            rc, check = command_json([args.hako_cmd, "check"], env)
            if rc != 0 or check["status"] != "OK":
                raise AssertionError(f"new session after recovery failed: {check!r}")
        finally:
            restarted.kill()
            restarted.wait(timeout=5)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
