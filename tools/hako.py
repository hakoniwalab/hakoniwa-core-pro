#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, Mapping


DEFAULT_MANIFEST = "hakoniwa-build.yaml"
MANIFEST_TO_NATIVE_KEY = {
    "asset_num": "HAKO_DATA_MAX_ASSET_NUM",
    "pdu_channel_max": "HAKO_PDU_CHANNEL_MAX",
    "recv_event_max": "HAKO_RECV_EVENT_MAX",
    "service_client_max": "HAKO_SERVICE_CLIENT_MAX",
    "service_max": "HAKO_SERVICE_MAX",
    "client_name_len_max": "HAKO_CLIENT_NAMELEN_MAX",
    "service_name_len_max": "HAKO_SERVICE_NAMELEN_MAX",
}


class HakoError(RuntimeError):
    pass


class ConfigError(HakoError):
    pass


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _strip_comment(text: str) -> str:
    quote: str | None = None
    escaped = False
    out: list[str] = []
    for ch in text:
        if escaped:
            out.append(ch)
            escaped = False
            continue
        if ch == "\\" and quote:
            out.append(ch)
            escaped = True
            continue
        if ch in {"'", '"'}:
            if quote is None:
                quote = ch
            elif quote == ch:
                quote = None
            out.append(ch)
            continue
        if ch == "#" and quote is None:
            break
        out.append(ch)
    return "".join(out).rstrip()


def _parse_scalar(text: str) -> Any:
    value = text.strip()
    if value == "":
        return {}
    lowered = value.lower()
    if lowered == "true":
        return True
    if lowered == "false":
        return False
    if lowered in {"null", "~"}:
        return None
    if value.startswith(('"', "'")):
        if len(value) < 2 or value[-1] != value[0]:
            raise ConfigError(f"unterminated quoted scalar: {value}")
        if value[0] == '"':
            try:
                return json.loads(value)
            except json.JSONDecodeError as exc:
                raise ConfigError(f"invalid quoted scalar: {value}") from exc
        return value[1:-1].replace("''", "'")
    try:
        return int(value)
    except ValueError:
        return value


def load_simple_yaml(path: Path) -> Dict[str, Any]:
    """Load the dependency-free mapping/scalar subset used by build manifest v1."""
    root: Dict[str, Any] = {}
    stack: list[tuple[int, Dict[str, Any]]] = [(-1, root)]
    for lineno, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if "\t" in raw:
            raise ConfigError(f"{path}:{lineno}: tabs are not allowed")
        line = _strip_comment(raw)
        if not line.strip():
            continue
        stripped = line.lstrip(" ")
        indent = len(line) - len(stripped)
        if stripped.startswith("-"):
            raise ConfigError(
                f"{path}:{lineno}: sequences are not supported in build manifest v1"
            )
        if ":" not in stripped:
            raise ConfigError(f"{path}:{lineno}: expected 'key: value'")
        key, raw_value = stripped.split(":", 1)
        key = key.strip()
        if not key:
            raise ConfigError(f"{path}:{lineno}: empty key")
        while stack and indent <= stack[-1][0]:
            stack.pop()
        if not stack:
            raise ConfigError(f"{path}:{lineno}: invalid indentation")
        parent = stack[-1][1]
        if key in parent:
            raise ConfigError(f"{path}:{lineno}: duplicate key: {key}")
        parsed = _parse_scalar(raw_value)
        parent[key] = parsed
        if isinstance(parsed, dict):
            stack.append((indent, parsed))
    return root


def resolve_config(raw: Mapping[str, Any]) -> Dict[str, Any]:
    root_keys = {"version", "limits"}
    unknown_root = sorted(set(raw) - root_keys)
    if unknown_root:
        raise ConfigError(f"unknown key(s) under root: {', '.join(unknown_root)}")
    missing_root = sorted(root_keys - set(raw))
    if missing_root:
        raise ConfigError(f"missing required key(s) under root: {', '.join(missing_root)}")
    if raw["version"] != 1:
        raise ConfigError("version must be 1")
    limits = raw["limits"]
    if not isinstance(limits, Mapping):
        raise ConfigError("limits must be a mapping")

    required_limits = set(MANIFEST_TO_NATIVE_KEY)
    unknown_limits = sorted(set(limits) - required_limits)
    if unknown_limits:
        raise ConfigError(
            f"unknown key(s) under limits: {', '.join(unknown_limits)}"
        )
    missing_limits = sorted(required_limits - set(limits))
    if missing_limits:
        raise ConfigError(
            f"missing required key(s) under limits: {', '.join(missing_limits)}"
        )

    resolved_limits: Dict[str, int] = {}
    for key in MANIFEST_TO_NATIVE_KEY:
        value = limits[key]
        if not isinstance(value, int) or isinstance(value, bool) or value <= 0:
            raise ConfigError(f"limits.{key} must be a positive integer")
        resolved_limits[key] = value
    return {"version": 1, "limits": resolved_limits}


def _manifest_path(value: str | None) -> Path:
    if value is None:
        path = repo_root() / DEFAULT_MANIFEST
        if not path.is_file():
            raise ConfigError(f"default build manifest not found: {path}")
        return path

    path = Path(value)
    if not path.is_absolute():
        path = (Path.cwd() / path).resolve()
    if not path.is_file():
        raise ConfigError(f"build manifest not found: {path}")
    return path


def _atomic_write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.parent / f".{path.name}.{os.getpid()}.tmp"
    try:
        temporary.write_text(content, encoding="utf-8")
        temporary.replace(path)
    finally:
        if temporary.exists():
            temporary.unlink()


def render_native_defaults(cfg: Mapping[str, Any]) -> str:
    lines = [
        "# Generated by tools/hako.py from the selected user-facing build manifest.",
    ]
    limits = cfg["limits"]
    for manifest_key, native_key in MANIFEST_TO_NATIVE_KEY.items():
        lines.append(f"{native_key}={limits[manifest_key]}")
    return "\n".join(lines) + "\n"


def _yaml_scalar(value: Any) -> str:
    if isinstance(value, int):
        return str(value)
    return json.dumps(str(value), ensure_ascii=False)


def render_resolved_manifest(
    manifest_path: Path,
    native_defaults_path: Path,
    cfg: Mapping[str, Any],
) -> str:
    lines = [
        "version: 1",
        f"manifest: {_yaml_scalar(manifest_path)}",
        f"native_defaults: {_yaml_scalar(native_defaults_path)}",
        "limits:",
    ]
    for key, value in cfg["limits"].items():
        lines.append(f"  {key}: {value}")
    return "\n".join(lines) + "\n"


def prepare_build_config(config_path: str | None) -> tuple[Path, Path, Dict[str, Any]]:
    root = repo_root()
    manifest = _manifest_path(config_path)
    cfg = resolve_config(load_simple_yaml(manifest))
    output_dir = root / ".hako"
    native_defaults = output_dir / "hako_build_defaults.conf"
    resolved_manifest = output_dir / "resolved-build.yaml"
    _atomic_write(native_defaults, render_native_defaults(cfg))
    _atomic_write(
        resolved_manifest,
        render_resolved_manifest(manifest, native_defaults, cfg),
    )
    return manifest, native_defaults, cfg


def powershell() -> str:
    for name in ("pwsh", "powershell.exe", "powershell"):
        found = shutil.which(name)
        if found:
            return found
    raise HakoError("PowerShell was not found on PATH")


def doctor(manifest: Path, native_defaults: Path) -> int:
    root = repo_root()
    checks: list[tuple[str, bool, str]] = []

    checks.append(("Python 3.12+", sys.version_info >= (3, 12), sys.version.split()[0]))
    checks.append(
        ("CMake", shutil.which("cmake") is not None, shutil.which("cmake") or "not found")
    )
    checks.append(("Git", shutil.which("git") is not None, shutil.which("git") or "not found"))

    cpp = root / "hakoniwa-core-cpp"
    checks.append(("hakoniwa-core-cpp", cpp.is_dir(), str(cpp)))
    checks.append(("build manifest", manifest.is_file(), str(manifest)))
    checks.append(
        ("resolved build defaults", native_defaults.is_file(), str(native_defaults))
    )
    checks.append(
        (
            "native default compatibility file",
            (root / "cmake" / "hako_build_defaults.conf").is_file(),
            "cmake/hako_build_defaults.conf",
        )
    )
    checks.append(("CMakeLists.txt", (root / "CMakeLists.txt").is_file(), "CMakeLists.txt"))

    if sys.platform == "win32":
        win_build = root / "win-build.ps1"
        checks.append(("Windows build driver", win_build.is_file(), "win-build.ps1"))
        ps = next(
            (
                shutil.which(name)
                for name in ("pwsh", "powershell.exe", "powershell")
                if shutil.which(name)
            ),
            None,
        )
        checks.append(("PowerShell", ps is not None, ps or "not found"))
    else:
        build_script = root / "build.bash"
        checks.append(("POSIX build driver", build_script.is_file(), "build.bash"))
        checks.append(("Bash", shutil.which("bash") is not None, shutil.which("bash") or "not found"))

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


def build(native_defaults: Path, native_args: list[str]) -> int:
    root = repo_root()
    child_env = dict(os.environ)
    child_env["HAKO_BUILD_DEFAULTS_FILE"] = str(native_defaults)
    if sys.platform == "win32":
        script = root / "win-build.ps1"
        if not script.is_file():
            raise HakoError(f"Windows build script not found: {script}")
        cmd = [
            powershell(),
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            str(script),
            *native_args,
        ]
    else:
        script = root / "build.bash"
        if not script.is_file():
            raise HakoError(f"build script not found: {script}")
        cmd = ["bash", str(script), *native_args]

    print(">", subprocess.list2cmdline(cmd))
    return subprocess.run(cmd, cwd=root, env=child_env, check=False).returncode


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Standard entry point for hakoniwa-core-pro")
    sub = parser.add_subparsers(dest="command", required=True)

    doctor_parser = sub.add_parser("doctor", help="check build prerequisites")
    doctor_parser.add_argument(
        "--config",
        default=None,
        help=f"user-facing build manifest (default: repository root/{DEFAULT_MANIFEST})",
    )

    build_parser = sub.add_parser("build", help="delegate to the existing platform build")
    build_parser.add_argument(
        "--config",
        default=None,
        help=f"user-facing build manifest (default: repository root/{DEFAULT_MANIFEST})",
    )
    build_parser.add_argument("native_args", nargs=argparse.REMAINDER)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = create_parser().parse_args(argv)
    manifest, native_defaults, _cfg = prepare_build_config(args.config)
    print(f"Build manifest          : {manifest}")
    print(f"Resolved native defaults: {native_defaults}")

    if args.command == "doctor":
        return doctor(manifest, native_defaults)
    if args.command == "build":
        native_args = args.native_args
        if native_args and native_args[0] == "--":
            native_args = native_args[1:]
        return build(native_defaults, native_args)
    raise HakoError(f"unsupported command: {args.command}")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except HakoError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2)
