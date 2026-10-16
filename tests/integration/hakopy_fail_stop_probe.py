#!/usr/bin/env python3
"""Verify Python fail-stop exceptions in an isolated asset process."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys


def expect_fail_stop(callable_, name: str) -> None:
    try:
        callable_()
    except RuntimeError as error:
        if "master lock owner process is no longer alive" not in str(error):
            raise AssertionError(f"unexpected {name} RuntimeError: {error}") from error
    else:
        raise AssertionError(f"{name} did not raise RuntimeError")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--hakopy-dir", required=True)
    parser.add_argument("--native-library-dir")
    parser.add_argument("--pdudef", required=True)
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
        expect_fail_stop(hakopy.init_for_external, "init_for_external")
        expect_fail_stop(
            lambda: hakopy.asset_register(
                "PythonAsset",
                args.pdudef,
                callbacks,
                1000,
                hakopy.HAKO_ASSET_MODEL_CONTROLLER,
            ),
            "asset_register",
        )
    finally:
        sys.path.pop(0)
        if dll_directory is not None:
            dll_directory.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
