#!/usr/bin/env python3
"""Verify Python fail-stop exceptions in an isolated asset process."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys


def expect_fail_stop(callable_, name: str, expected_status: str) -> None:
    try:
        callable_()
    except RuntimeError as error:
        if getattr(error, "hako_status", None) != expected_status:
            raise AssertionError(
                f"missing {expected_status} status on {name}: {error!r}"
            )
        expected_code = {"ABORT": 1, "UNKNOWN": 2}[expected_status]
        if getattr(error, "hako_status_code", None) != expected_code:
            raise AssertionError(
                f"missing {expected_status} status code on {name}: {error!r}"
            )
    else:
        raise AssertionError(f"{name} did not raise RuntimeError")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--hakopy-dir", required=True)
    parser.add_argument("--native-library-dir")
    parser.add_argument("--pdudef", required=True)
    parser.add_argument(
        "--expected-status", choices=("ABORT", "UNKNOWN"), default="ABORT",
    )
    args = parser.parse_args()

    dll_directory = None
    if sys.platform == "win32":
        if args.native_library_dir is None:
            raise AssertionError("native library directory is required on Windows")
        dll_directory = os.add_dll_directory(str(Path(args.native_library_dir)))
    sys.path.insert(0, str(Path(args.hakopy_dir)))
    try:
        import hakopy

        callbacks = {
            "on_initialize": None,
            "on_reset": None,
            "on_simulation_step": None,
            "on_manual_timing_control": None,
        }
        expect_fail_stop(
            hakopy.init_for_external, "init_for_external", args.expected_status,
        )
        expect_fail_stop(
            lambda: hakopy.asset_register(
                "PythonAsset",
                args.pdudef,
                callbacks,
                1000,
                hakopy.HAKO_ASSET_MODEL_CONTROLLER,
            ),
            "asset_register",
            args.expected_status,
        )
        # Keep this last: conductor is a separate library in the historical
        # static-Core build and must not influence later asset calls.
        expect_fail_stop(
            lambda: hakopy.conductor_start(1000, 1000),
            "conductor_start",
            args.expected_status,
        )
    finally:
        sys.path.pop(0)
        if dll_directory is not None:
            dll_directory.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
