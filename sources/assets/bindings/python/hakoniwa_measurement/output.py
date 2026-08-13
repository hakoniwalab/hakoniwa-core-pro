from __future__ import annotations

import json
import os
import tempfile
from dataclasses import asdict, is_dataclass
from pathlib import Path
from typing import Any


def _json_value(value: Any) -> Any:
    if is_dataclass(value):
        return {key: _json_value(item) for key, item in asdict(value).items()}
    if isinstance(value, dict):
        return {str(key): _json_value(item) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [_json_value(item) for item in value]
    return value


class JsonLinesWriter:
    """Create a new JSONL stream; never append to a previous trial."""

    def __init__(self, path: Path) -> None:
        self.path = Path(path)
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self._file = self.path.open("x", encoding="utf-8")

    def write(self, value: Any) -> None:
        self._file.write(
            json.dumps(_json_value(value), ensure_ascii=True, allow_nan=False)
            + "\n"
        )
        self._file.flush()

    def close(self) -> None:
        if not self._file.closed:
            self._file.close()

    def __enter__(self) -> "JsonLinesWriter":
        return self

    def __exit__(self, _exc_type, _exc, _traceback) -> None:
        self.close()


def write_json_atomic(path: Path, value: Any, *, replace: bool = False) -> None:
    target = Path(path)
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists() and not replace:
        raise FileExistsError(f"measurement result already exists: {target}")
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{target.name}.", suffix=".tmp", dir=target.parent
    )
    temporary = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as output:
            json.dump(
                _json_value(value),
                output,
                ensure_ascii=True,
                indent=2,
                allow_nan=False,
            )
            output.write("\n")
            output.flush()
            os.fsync(output.fileno())
        temporary.replace(target)
    except Exception:
        temporary.unlink(missing_ok=True)
        raise
