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
            try:
                returncode = process.wait(timeout=0.2)
            except subprocess.TimeoutExpired:
                returncode = process.poll()
            stderr = ""
            if process.stderr is not None:
                stderr = process.stderr.read()
            raise AssertionError(
                f"process exited before READY: rc={returncode}, stderr={stderr!r}"
            )
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


def check_named(payload: dict, name: str) -> dict:
    if payload.get("schema_version") != 1:
        raise AssertionError(f"unexpected check schema: {payload!r}")
    checks = payload.get("checks")
    if not isinstance(checks, list):
        raise AssertionError(f"checks must be an array: {payload!r}")
    for check in checks:
        if check.get("name") == name:
            if not all(key in check for key in ("status", "reason")):
                raise AssertionError(f"incomplete check entry: {check!r}")
            return check
    raise AssertionError(f"missing check {name!r}: {payload!r}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--hako-cmd", required=True)
    parser.add_argument("--holder", required=True)
    parser.add_argument("--runtime-c-api-probe", required=True)
    parser.add_argument("--hakopy-dir")
    parser.add_argument("--native-library-dir")
    parser.add_argument("--hakopy-running-probe")
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
        # The hakopy assertion below runs in this process, while native helper
        # commands receive the copied environment explicitly.
        os.environ["HAKO_CONFIG_PATH"] = str(config_path)

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
                    check_named(check, "master_lock")["state"] != "LOCKED"):
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

        (root / "pdutypes.json").write_text(
            json.dumps([
                {
                    "channel_id": 0,
                    "pdu_size": 8,
                    "name": "test",
                    "type": "test/Test",
                }
            ]),
            encoding="utf-8",
        )
        pdudef_path = root / "pdudef.json"
        pdudef_path.write_text(
            json.dumps({
                "paths": [{"id": "default", "path": "pdutypes.json"}],
                "robots": [{"name": "Robot1", "pdutypes_id": "default"}],
            }),
            encoding="utf-8",
        )
        if args.hakopy_dir is not None:
            probe_command = [
                sys.executable,
                str(Path(__file__).with_name("hakopy_fail_stop_probe.py")),
                "--hakopy-dir", args.hakopy_dir,
                "--pdudef", str(pdudef_path),
            ]
            if args.native_library_dir is not None:
                probe_command.extend([
                    "--native-library-dir", args.native_library_dir,
                ])
            probe = subprocess.run(
                probe_command,
                capture_output=True,
                text=True,
                env=env,
                timeout=5,
                check=False,
            )
            if probe.returncode != 0:
                raise AssertionError(
                    f"hakopy fail-stop probe failed: rc={probe.returncode}, "
                    f"stdout={probe.stdout!r}, stderr={probe.stderr!r}"
                )

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

            rc, recovery = command_json([args.hako_cmd, "recover"], env)
            if rc != 4 or recovery["status"] != "REFUSED":
                raise AssertionError(
                    f"recovery did not refuse busy OS lock: rc={rc}, json={recovery!r}"
                )
            if not (root / "flock.bin").exists():
                raise AssertionError("recovery removed a busy flock.bin")

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

        rc, recovered = command_json([args.hako_cmd, "recover"], env)
        if rc != 0:
            raise AssertionError(f"pre-running-probe recovery failed: {recovered!r}")

        if args.hakopy_running_probe is not None:
            service_config = root / "service.json"
            service_config.write_text(
                json.dumps({
                    "pduMetaDataSize": 8,
                    "services": [{
                        "name": "RuntimeProbeService",
                        "type": "test/RuntimeProbe",
                        "maxClients": 1,
                        "pduSize": {
                            "server": {"baseSize": 8, "heapSize": 0},
                            "client": {"baseSize": 8, "heapSize": 0},
                        },
                    }],
                }),
                encoding="utf-8",
            )
            running_probe_command = [
                sys.executable,
                args.hakopy_running_probe,
                "--hakopy-dir", args.hakopy_dir,
                "--service-config", str(service_config),
            ]
            if args.native_library_dir is not None:
                running_probe_command.extend([
                    "--native-library-dir", args.native_library_dir,
                ])
            idle_master = subprocess.Popen(
                [args.holder, "--master-abandonable"],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                env=env,
            )
            running_probe = None
            native_probe = None
            try:
                wait_until_ready(idle_master)
                running_probe = subprocess.Popen(
                    running_probe_command,
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    env=env,
                )
                wait_until_ready(running_probe)
                native_probe = subprocess.Popen(
                    [args.runtime_c_api_probe, str(service_config)],
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    env=env,
                )
                wait_until_ready(native_probe)
                assert idle_master.stdin is not None
                idle_master.stdin.write("ABANDON\n")
                idle_master.stdin.flush()
                idle_master.wait(timeout=5)
                if idle_master.returncode != 0:
                    raise AssertionError(
                        "master fixture did not abandon the lock cleanly: "
                        f"rc={idle_master.returncode}"
                    )
                assert running_probe.stdin is not None
                running_probe.stdin.write("CHECK\n")
                running_probe.stdin.flush()
                assert native_probe.stdin is not None
                native_probe.stdin.write("CHECK\n")
                native_probe.stdin.flush()
                stdout, stderr = running_probe.communicate(timeout=5)
                if running_probe.returncode != 0 or "ABORT_OK" not in stdout:
                    raise AssertionError(
                        "attached hakopy process did not contain runtime failure: "
                        f"rc={running_probe.returncode}, stdout={stdout!r}, "
                        f"stderr={stderr!r}"
                    )
                native_stdout, native_stderr = native_probe.communicate(timeout=5)
                if native_probe.returncode != 0 or "ABORT_OK" not in native_stdout:
                    raise AssertionError(
                        "attached native process did not contain runtime failure: "
                        f"rc={native_probe.returncode}, stdout={native_stdout!r}, "
                        f"stderr={native_stderr!r}"
                    )
            finally:
                if running_probe is not None and running_probe.poll() is None:
                    running_probe.kill()
                    running_probe.wait(timeout=5)
                if native_probe is not None and native_probe.poll() is None:
                    native_probe.kill()
                    native_probe.wait(timeout=5)
                if idle_master.poll() is None:
                    idle_master.kill()
                    idle_master.wait(timeout=5)

            rc, recovered = command_json([args.hako_cmd, "recover"], env)
            if rc != 0:
                raise AssertionError(
                    f"pre-UNKNOWN-probe recovery failed: {recovered!r}"
                )
            legacy_master = subprocess.Popen(
                [args.holder, "--master-idle"],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                env=env,
            )
            wait_until_ready(legacy_master)
            legacy_master.kill()
            legacy_master.wait(timeout=5)
            (root / "flock.bin").write_bytes(bytes(4096))
            unknown_probe = subprocess.run(
                [
                    sys.executable,
                    str(Path(__file__).with_name("hakopy_fail_stop_probe.py")),
                    "--hakopy-dir", args.hakopy_dir,
                    "--pdudef", str(pdudef_path),
                    "--expected-status", "UNKNOWN",
                    *(
                        ["--native-library-dir", args.native_library_dir]
                        if args.native_library_dir is not None else []
                    ),
                ],
                capture_output=True,
                text=True,
                env=env,
                timeout=5,
                check=False,
            )
            if unknown_probe.returncode != 0:
                raise AssertionError(
                    "hakopy UNKNOWN probe failed: "
                    f"rc={unknown_probe.returncode}, "
                    f"stdout={unknown_probe.stdout!r}, "
                    f"stderr={unknown_probe.stderr!r}"
                )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
