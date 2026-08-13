from __future__ import annotations

import ctypes

from ...models import MachineResourceSample
from ..base import CpuCounterTracker


class _FileTime(ctypes.Structure):
    _fields_ = [("low", ctypes.c_uint32), ("high", ctypes.c_uint32)]

    def value(self) -> int:
        return (int(self.high) << 32) | int(self.low)


class _MemoryStatusEx(ctypes.Structure):
    _fields_ = [
        ("length", ctypes.c_uint32),
        ("memory_load", ctypes.c_uint32),
        ("total_phys", ctypes.c_uint64),
        ("avail_phys", ctypes.c_uint64),
        ("total_page_file", ctypes.c_uint64),
        ("avail_page_file", ctypes.c_uint64),
        ("total_virtual", ctypes.c_uint64),
        ("avail_virtual", ctypes.c_uint64),
        ("avail_extended_virtual", ctypes.c_uint64),
    ]


class WindowsHostResourceBackend:
    def __init__(self) -> None:
        self._kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        self._cpu = CpuCounterTracker()

    @property
    def backend_id(self) -> str:
        return "windows-kernel32"

    def _cpu_counters(self) -> tuple[int, int]:
        idle = _FileTime()
        kernel = _FileTime()
        user = _FileTime()
        if not self._kernel32.GetSystemTimes(
            ctypes.byref(idle), ctypes.byref(kernel), ctypes.byref(user)
        ):
            raise ctypes.WinError(ctypes.get_last_error())
        return kernel.value() + user.value(), idle.value()

    def _memory(self) -> tuple[int, int]:
        status = _MemoryStatusEx()
        status.length = ctypes.sizeof(status)
        if not self._kernel32.GlobalMemoryStatusEx(ctypes.byref(status)):
            raise ctypes.WinError(ctypes.get_last_error())
        return int(status.total_phys - status.avail_phys), int(status.total_phys)

    def sample(self, monotonic_time_ns: int) -> MachineResourceSample:
        total, idle = self._cpu_counters()
        memory_used, memory_total = self._memory()
        return MachineResourceSample(
            monotonic_time_ns=int(monotonic_time_ns),
            cpu_percent=self._cpu.update(total, idle),
            memory_used_bytes=memory_used,
            memory_total_bytes=memory_total,
        )
