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


def _explicit_path(value: str | None, default: Path) -> Path:
    if value is None:
        return default
    path = Path(value)
    if not path.is_absolute():
        path = Path.cwd() / path
    return path.resolve()


def _command_output(command: list[str], cwd: Path) -> str:
    result = subprocess.run(
        command,
        cwd=cwd,
        check=False,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip() if result.returncode == 0 else "unknown"


def _cmake_cache_value(build_dir: Path, key: str) -> str:
    cache = build_dir / "CMakeCache.txt"
    if not cache.is_file():
        return "unknown"
    prefix = f"{key}:"
    for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith(prefix) and "=" in line:
            return line.split("=", 1)[1] or "unknown"
    return "unknown"


def _normalized_architecture(machine: str | None = None) -> str:
    value = (machine or platform.machine()).lower()
    return {
        "amd64": "x64",
        "x86_64": "x64",
        "aarch64": "arm64",
    }.get(value, value)


def _artifact_kind(path: Path, installed: Path) -> str:
    if installed.is_dir():
        return "directory"
    if path.parts and path.parts[0] == "bin":
        return "executable"
    if path.suffix in {".a", ".so", ".dylib", ".dll", ".lib", ".pyd"}:
        return "library"
    if "cmake" in path.parts:
        return "cmake-package"
    if path.suffix in {".h", ".hpp"}:
        return "header"
    return "data"


def _core_artifacts(install_dir: Path) -> list[tuple[Path, str]]:
    """Return stable Core install surfaces instead of every generated PDU file."""
    artifacts: list[Path] = []
    directory_roots = (
        Path("include/hakoniwa"),
        Path("lib/cmake/hakoniwa-core"),
        Path("lib/pkgconfig"),
        Path("share/hakoniwa/offset"),
    )
    for relative in directory_roots:
        if (install_dir / relative).is_dir():
            artifacts.append(relative)

    binary_dir = install_dir / "bin"
    if binary_dir.is_dir():
        artifacts.extend(
            path.relative_to(install_dir)
            for path in binary_dir.iterdir()
            if path.is_file() and path.name in {"hako-cmd", "hako-cmd.exe"}
        )

    library_suffixes = {".a", ".so", ".dylib", ".dll", ".lib"}
    library_dir = install_dir / "lib"
    if library_dir.is_dir():
        artifacts.extend(
            path.relative_to(install_dir)
            for path in library_dir.iterdir()
            if path.is_file() and path.suffix.lower() in library_suffixes
        )

    python_dir = install_dir / "share" / "hakoniwa" / "python"
    if python_dir.is_dir():
        artifacts.extend(
            path.relative_to(install_dir)
            for path in python_dir.iterdir()
            if path.is_file() and path.suffix.lower() in {".so", ".pyd"}
        )

    unique = sorted(set(artifacts), key=lambda item: item.as_posix())
    return [
        (relative, _artifact_kind(relative, install_dir / relative))
        for relative in unique
    ]


def write_receipt(
    build_dir: Path,
    install_dir: Path,
    cfg: Mapping[str, Any],
) -> Path:
    root = repo_root()
    receipt_root = install_dir / "share" / "hakoniwa" / "receipts"
    resolved_root = receipt_root / "resolved"
    resolved_root.mkdir(parents=True, exist_ok=True)
    resolved_source = root / ".hako" / "resolved-build.yaml"
    resolved_relative = (
        Path("share")
        / "hakoniwa"
        / "receipts"
        / "resolved"
        / "hakoniwa-core-pro.yaml"
    )
    shutil.copyfile(resolved_source, install_dir / resolved_relative)

    artifacts = _core_artifacts(install_dir)
    if not any(path.as_posix() in {"bin/hako-cmd", "bin/hako-cmd.exe"} for path, _ in artifacts):
        raise HakoError(f"installed hako-cmd not found under: {install_dir}")

    os_name = {
        "Darwin": "macos",
        "Linux": "linux",
        "Windows": "windows",
    }.get(platform.system(), platform.system().lower())
    compiler = _cmake_cache_value(build_dir, "CMAKE_CXX_COMPILER")
    revision = _command_output(["git", "rev-parse", "HEAD"], root)
    limits = cfg["limits"]
    lines = [
        "schema_version: 1",
        "component:",
        "  id: hakoniwa-core-pro",
        "  version: 1.0.0",
        f"  source_revision: {_yaml_scalar(revision)}",
        "platform:",
        f"  os: {_yaml_scalar(os_name)}",
        f"  architecture: {_yaml_scalar(_normalized_architecture())}",
        f"  toolchain: {_yaml_scalar(compiler)}",
        "install:",
        f"  prefix: {_yaml_scalar(install_dir)}",
        "capabilities:",
        "  shared_memory: true",
        "  hako_cmd: true",
        "  python_binding: true",
        "  cmake_package: true",
        "build_limits:",
    ]
    for key, value in limits.items():
        lines.append(f"  {key}: {value}")
    lines.extend(["dependencies: {}", "artifacts:"])
    for artifact, kind in artifacts:
        lines.extend(
            [
                f"  - path: {_yaml_scalar(artifact.as_posix())}",
                f"    kind: {kind}",
            ]
        )
    lines.append(f"resolved_manifest: {_yaml_scalar(resolved_relative.as_posix())}")
    receipt_path = receipt_root / "hakoniwa-core-pro.yaml"
    _atomic_write(receipt_path, "\n".join(lines) + "\n")
    return receipt_path


def install(
    build_dir: Path,
    install_dir: Path,
    configuration: str,
    cfg: Mapping[str, Any],
) -> int:
    root = repo_root()
    if not (build_dir / "CMakeCache.txt").is_file():
        raise HakoError(
            f"configured build tree not found: {build_dir}; run hako.py build first"
        )
    install_dir.mkdir(parents=True, exist_ok=True)
    cmd = ["cmake", "--install", str(build_dir), "--prefix", str(install_dir)]
    if sys.platform == "win32":
        cmd.extend(["--config", configuration])
    print(">", subprocess.list2cmdline(cmd))
    result = subprocess.run(cmd, cwd=root, check=False).returncode
    if result == 0:
        receipt = write_receipt(build_dir, install_dir, cfg)
        print(f"Component Receipt: {receipt}")
    return result


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
    build_parser.add_argument("--build-dir", default=None)
    build_parser.add_argument("--install-dir", default=None)
    build_parser.add_argument("--core-config-dir", default=None)
    build_parser.add_argument("--python-install-dir", default=None)
    build_parser.add_argument("--python-executable", default=None)
    build_parser.add_argument("--core-mmap-dir", default=None)
    build_parser.add_argument("native_args", nargs=argparse.REMAINDER)

    install_parser = sub.add_parser(
        "install", help="install the existing build tree to an explicit local prefix"
    )
    install_parser.add_argument(
        "--config",
        default=None,
        help=f"user-facing build manifest (default: repository root/{DEFAULT_MANIFEST})",
    )
    install_parser.add_argument("--build-dir", default=None)
    install_parser.add_argument("--install-dir", required=True)
    install_parser.add_argument("--configuration", default="Release")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = create_parser().parse_args(argv)
    manifest, native_defaults, _cfg = prepare_build_config(args.config)
    print(f"Build manifest          : {manifest}")
    print(f"Resolved native defaults: {native_defaults}")

    if args.command == "doctor":
        return doctor(manifest, native_defaults)
    if args.command == "build":
        root = repo_root()
        build_dir = _explicit_path(
            args.build_dir,
            root / ("build-win" if sys.platform == "win32" else "cmake-build"),
        )
        child_values = {
            "HAKO_BUILD_DIR": str(build_dir),
            "HAKO_INSTALL_PREFIX": (
                str(_explicit_path(args.install_dir, root)) if args.install_dir else None
            ),
            "HAKO_CORE_CONFIG_INSTALL_DIR": (
                str(_explicit_path(args.core_config_dir, root))
                if args.core_config_dir
                else None
            ),
            "HAKO_PYTHON_INSTALL_DIR": (
                str(_explicit_path(args.python_install_dir, root))
                if args.python_install_dir
                else None
            ),
            "HAKO_PYTHON_EXECUTABLE": (
                str(_explicit_path(args.python_executable, root))
                if args.python_executable
                else None
            ),
            "HAKO_CORE_MMAP_PATH": (
                str(_explicit_path(args.core_mmap_dir, root))
                if args.core_mmap_dir
                else None
            ),
        }
        previous = {key: os.environ.get(key) for key in child_values}
        for key, value in child_values.items():
            if value is not None:
                os.environ[key] = value
        native_args = args.native_args
        if native_args and native_args[0] == "--":
            native_args = native_args[1:]
        try:
            return build(native_defaults, native_args)
        finally:
            for key, value in previous.items():
                if value is None:
                    os.environ.pop(key, None)
                else:
                    os.environ[key] = value
    if args.command == "install":
        root = repo_root()
        build_dir = _explicit_path(
            args.build_dir,
            root / ("build-win" if sys.platform == "win32" else "cmake-build"),
        )
        install_dir = _explicit_path(args.install_dir, root)
        return install(build_dir, install_dir, args.configuration, _cfg)
    raise HakoError(f"unsupported command: {args.command}")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except HakoError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2)
