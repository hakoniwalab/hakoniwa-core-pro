from __future__ import annotations

from pathlib import Path

from ...models import MachineResourceSample
from ..base import CpuCounterTracker


class LinuxHostResourceBackend:
    def __init__(self, proc_root: Path = Path("/proc")) -> None:
        self._proc_root = proc_root
        self._cpu = CpuCounterTracker()

    @property
    def backend_id(self) -> str:
        return "linux-procfs"

    def _cpu_counters(self) -> tuple[int, int]:
        first = (self._proc_root / "stat").read_text(encoding="utf-8").splitlines()[0]
        fields = first.split()
        if not fields or fields[0] != "cpu" or len(fields) < 5:
            raise RuntimeError("invalid /proc/stat aggregate CPU row")
        counters = [int(value) for value in fields[1:]]
        total = sum(counters)
        idle = counters[3] + (counters[4] if len(counters) > 4 else 0)
        return total, idle

    def _memory(self) -> tuple[int, int]:
        values: dict[str, int] = {}
        for line in (self._proc_root / "meminfo").read_text(encoding="utf-8").splitlines():
            key, separator, rest = line.partition(":")
            if not separator:
                continue
            fields = rest.split()
            if fields and fields[0].isdigit():
                values[key] = int(fields[0]) * 1024
        total = values.get("MemTotal")
        available = values.get("MemAvailable")
        if total is None or available is None:
            raise RuntimeError("MemTotal or MemAvailable is missing from /proc/meminfo")
        return max(0, total - available), total

    def sample(self, monotonic_time_ns: int) -> MachineResourceSample:
        total, idle = self._cpu_counters()
        memory_used, memory_total = self._memory()
        return MachineResourceSample(
            monotonic_time_ns=int(monotonic_time_ns),
            cpu_percent=self._cpu.update(total, idle),
            memory_used_bytes=memory_used,
            memory_total_bytes=memory_total,
        )
