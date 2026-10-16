#!/usr/bin/env python3
"""Cross-process acceptance tests for the explicit runtime recovery gate."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time


def wait_path(path: Path, process: subprocess.Popen[str], timeout: float = 8.0) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if path.exists():
            return
        if process.poll() is not None:
            stdout, stderr = process.communicate()
            raise AssertionError(
                f"process exited before barrier: rc={process.returncode}, "
                f"stdout={stdout!r}, stderr={stderr!r}"
            )
        time.sleep(0.02)
    process.kill()
    stdout, stderr = process.communicate(timeout=3)
    raise AssertionError(
        f"barrier timeout: {path}; stdout={stdout!r}, stderr={stderr!r}"
    )


def start(command: list[str], env: dict[str, str]) -> subprocess.Popen[str]:
    return subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        env=env,
    )


def stop(process: subprocess.Popen[str]) -> None:
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=3)


def release_and_expect_success(process: subprocess.Popen[str], release: Path) -> None:
    release.write_text("release\n", encoding="utf-8")
    stdout, stderr = process.communicate(timeout=8)
    if process.returncode != 0:
        raise AssertionError(
            f"process failed: rc={process.returncode}, "
            f"stdout={stdout!r}, stderr={stderr!r}"
        )


def recover(hako_cmd: str, env: dict[str, str]) -> tuple[int, dict]:
    result = subprocess.run(
        [hako_cmd, "recover"],
        capture_output=True,
        text=True,
        env=env,
        timeout=8,
        check=False,
    )
    try:
        payload = json.loads(result.stdout)
    except json.JSONDecodeError as error:
        raise AssertionError(
            f"invalid recovery JSON: rc={result.returncode}, "
            f"stdout={result.stdout!r}, stderr={result.stderr!r}"
        ) from error
    return result.returncode, payload


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--hako-cmd", required=True)
    parser.add_argument("--holder", required=True)
    parser.add_argument("--helper", required=True)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="hako-recovery-lifecycle-") as temp:
        base = Path(temp)
        root = base / "runtime"
        root.mkdir()
        config = root / "cpp_core_config.json"
        config.write_text(
            json.dumps({"shm_type": "mmap", "core_mmap_path": str(root)}),
            encoding="utf-8",
        )
        env = dict(os.environ)
        env["HAKO_CONFIG_PATH"] = str(config)

        # An init_for_external() process is not in the asset PID table. Its
        # lifecycle lease must nevertheless make recovery refuse safely.
        master_ready = base / "master.ready"
        master_release = base / "master.release"
        master = start(
            [
                args.holder,
                "--session",
                str(master_ready),
                str(master_release),
            ],
            env,
        )
        try:
            wait_path(master_ready, master)

            external_ready = base / "external.ready"
            external_release = base / "external.release"
            external = start(
                [args.helper, "external", str(external_ready), str(external_release)],
                env,
            )
            wait_path(external_ready, external)
            stop(master)

            rc, payload = recover(args.hako_cmd, env)
            if rc != 4 or payload.get("status") != "REFUSED":
                raise AssertionError(
                    f"live external client did not prohibit recovery: "
                    f"rc={rc}, payload={payload!r}"
                )
            if "BUSY" not in payload.get("reason", ""):
                raise AssertionError(f"missing BUSY diagnostic: {payload!r}")
            if not (root / "mmap-0xff.bin").exists():
                raise AssertionError("refused recovery deleted active session data")
            release_and_expect_success(external, external_release)
        finally:
            stop(master)
            if "external" in locals():
                stop(external)

        rc, payload = recover(args.hako_cmd, env)
        if rc != 0 or payload.get("status") != "RECOVERED":
            raise AssertionError(f"cleanup after external exit failed: {payload!r}")

        # Seed a second stale session and pause real recovery inside its first
        # delete callback. A real init_for_external() attempt must not cross
        # the gate during the verification-to-deletion window.
        second_master_ready = base / "second-master.ready"
        second_master_release = base / "second-master.release"
        master = start(
            [
                args.holder,
                "--session",
                str(second_master_ready),
                str(second_master_release),
            ],
            env,
        )
        try:
            wait_path(second_master_ready, master)
        finally:
            stop(master)

        recovery_ready = base / "recovery.ready"
        recovery_release = base / "recovery.release"
        recovery_process = start(
            [
                args.helper,
                "recover-hold",
                str(recovery_ready),
                str(recovery_release),
            ],
            env,
        )
        wait_path(recovery_ready, recovery_process)

        racing_ready = base / "racing-external.ready"
        racing_release = base / "racing-external.release"
        racing = start(
            [args.helper, "external", str(racing_ready), str(racing_release)],
            env,
        )
        try:
            time.sleep(0.3)
            if racing_ready.exists() or racing.poll() is not None:
                raise AssertionError(
                    "external attachment crossed the held recovery gate"
                )
            release_and_expect_success(recovery_process, recovery_release)
            stdout, stderr = racing.communicate(timeout=8)
            if racing.returncode == 0 or racing_ready.exists():
                raise AssertionError(
                    "external client attached to artifacts deleted by recovery: "
                    f"rc={racing.returncode}, stdout={stdout!r}, stderr={stderr!r}"
                )
        finally:
            stop(recovery_process)
            stop(racing)

        if (root / "mmap-0xff.bin").exists() or (root / "flock.bin").exists():
            raise AssertionError("recovery left session artifacts behind")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
