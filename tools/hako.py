#!/usr/bin/env python3
from __future__ import annotations

import argparse
import platform
import shutil
import subprocess
import sys
from pathlib import Path


class HakoError(RuntimeError):
    pass


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def doctor() -> int:
    root = repo_root()
    checks: list[tuple[str, bool, str]] = []

    checks.append(("Python", sys.version_info >= (3, 8), sys.version.split()[0]))
    checks.append(("CMake", shutil.which("cmake") is not None, shutil.which("cmake") or "not found"))
    checks.append(("Git", shutil.which("git") is not None, shutil.which("git") or "not found"))

    cpp = root / "hakoniwa-core-cpp"
    checks.append(("hakoniwa-core-cpp", cpp.is_dir(), str(cpp)))
    checks.append(("build defaults", (root / "cmake" / "hako_build_defaults.conf").is_file(), "cmake/hako_build_defaults.conf"))
    checks.append(("CMakeLists.txt", (root / "CMakeLists.txt").is_file(), "CMakeLists.txt"))

    print(f"Platform: {platform.system()} {platform.machine()}")
    failed = False
    for name, ok, detail in checks:
        print(f"[{'OK' if ok else 'NG'}] {name}: {detail}")
        failed = failed or not ok

    if failed:
        print("Doctor found missing build prerequisites.", file=sys.stderr)
        return 1

    print("Doctor passed.")
    return 0


def build(native_args: list[str]) -> int:
    root = repo_root()
    if sys.platform == "win32":
        raise HakoError(
            "native Windows build is not standardized in this repository yet; "
            "use the existing supported environment or add a component-owned Windows build driver first"
        )

    script = root / "build.bash"
    if not script.is_file():
        raise HakoError(f"build script not found: {script}")
    cmd = ["bash", str(script), *native_args]
    print(">", subprocess.list2cmdline(cmd))
    return subprocess.run(cmd, cwd=root, check=False).returncode


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Standard entry point for hakoniwa-core-pro")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("doctor", help="check build prerequisites")
    build_parser = sub.add_parser("build", help="delegate to the existing component build")
    build_parser.add_argument("native_args", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)

    if args.command == "doctor":
        return doctor()
    if args.command == "build":
        native_args = args.native_args
        if native_args and native_args[0] == "--":
            native_args = native_args[1:]
        return build(native_args)
    raise HakoError(f"unsupported command: {args.command}")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except HakoError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2)
