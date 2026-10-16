#!/usr/bin/env python3
"""Keep hakopy attached while another process abandons the master lock."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys


def expect_abort(callable_, name: str) -> None:
    try:
        callable_()
    except RuntimeError as error:
        if getattr(error, "hako_status", None) != "ABORT":
            raise AssertionError(f"{name} did not expose ABORT: {error!r}") from error
        if getattr(error, "hako_status_code", None) != 1:
            raise AssertionError(f"{name} did not expose status code 1: {error!r}") from error
    else:
        raise AssertionError(f"{name} did not raise RuntimeError")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--hakopy-dir", required=True)
    parser.add_argument("--native-library-dir")
    parser.add_argument("--service-config", required=True)
    args = parser.parse_args()

    dll_directory = None
    if sys.platform == "win32":
        if args.native_library_dir is None:
            raise AssertionError("native library directory is required on Windows")
        dll_directory = os.add_dll_directory(str(Path(args.native_library_dir)))
    sys.path.insert(0, str(Path(args.hakopy_dir)))
    try:
        import hakopy

        if not hakopy.init_for_external():
            raise AssertionError("healthy init_for_external failed")
        print("READY", flush=True)
        if sys.stdin.readline().strip() != "CHECK":
            raise AssertionError("missing CHECK command")

        expect_abort(
            lambda: hakopy.pdu_create("RuntimeProbe", 77, 8),
            "pdu_create",
        )
        expect_abort(
            lambda: hakopy.service_initialize(args.service_config),
            "service_initialize",
        )
        print("ABORT_OK", flush=True)
    finally:
        sys.path.pop(0)
        if dll_directory is not None:
            dll_directory.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

