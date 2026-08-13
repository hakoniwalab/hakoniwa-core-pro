from __future__ import annotations

from typing import Protocol

from ..models import MachineResourceSample


class HostResourceBackend(Protocol):
    @property
    def backend_id(self) -> str:
        ...

    def sample(self, monotonic_time_ns: int) -> MachineResourceSample:
        ...


class CpuCounterTracker:
    """Convert cumulative total/idle counters into interval CPU utilization."""

    def __init__(self) -> None:
        self._previous: tuple[int, int] | None = None

    def update(self, total: int, idle: int) -> float | None:
        current = (int(total), int(idle))
        previous = self._previous
        self._previous = current
        if previous is None:
            return None
        total_delta = current[0] - previous[0]
        idle_delta = current[1] - previous[1]
        if total_delta <= 0 or idle_delta < 0:
            return None
        busy_delta = max(0, total_delta - idle_delta)
        return min(100.0, max(0.0, 100.0 * busy_delta / total_delta))
