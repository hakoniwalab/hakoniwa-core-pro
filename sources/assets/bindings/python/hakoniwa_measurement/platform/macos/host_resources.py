from __future__ import annotations

import ctypes
import os

from ...models import MachineResourceSample
from ..base import CpuCounterTracker


class MacOSHostResourceBackend:
    _HOST_CPU_LOAD_INFO = 3
    _HOST_VM_INFO64 = 4
    _CPU_STATE_COUNT = 4
    _VM_INFO64_COUNT = 64

    def __init__(self) -> None:
        self._lib = ctypes.CDLL("/usr/lib/libSystem.B.dylib")
        self._host = int(self._lib.mach_host_self())
        self._cpu = CpuCounterTracker()

    @property
    def backend_id(self) -> str:
        return "macos-mach"

    def _statistics(self, flavor: int, count: int) -> list[int]:
        values = (ctypes.c_uint32 * count)()
        value_count = ctypes.c_uint32(count)
        result = self._lib.host_statistics64(
            ctypes.c_uint32(self._host),
            ctypes.c_int(flavor),
            ctypes.cast(values, ctypes.POINTER(ctypes.c_int)),
            ctypes.byref(value_count),
        )
        if result != 0:
            raise RuntimeError(f"host_statistics64 failed: kern_return={result}")
        return [int(values[index]) for index in range(value_count.value)]

    def _total_memory(self) -> int:
        value = ctypes.c_uint64()
        size = ctypes.c_size_t(ctypes.sizeof(value))
        result = self._lib.sysctlbyname(
            b"hw.memsize", ctypes.byref(value), ctypes.byref(size), None, 0
        )
        if result != 0:
            raise RuntimeError("sysctlbyname(hw.memsize) failed")
        return int(value.value)

    def sample(self, monotonic_time_ns: int) -> MachineResourceSample:
        cpu = self._statistics(self._HOST_CPU_LOAD_INFO, self._CPU_STATE_COUNT)
        total_ticks = sum(cpu[: self._CPU_STATE_COUNT])
        idle_ticks = cpu[2]

        vm = self._statistics(self._HOST_VM_INFO64, self._VM_INFO64_COUNT)
        page_size = int(os.sysconf("SC_PAGE_SIZE"))
        free_pages = vm[0] + vm[2] + vm[14]
        memory_total = self._total_memory()
        memory_used = max(0, memory_total - free_pages * page_size)
        return MachineResourceSample(
            monotonic_time_ns=int(monotonic_time_ns),
            cpu_percent=self._cpu.update(total_ticks, idle_ticks),
            memory_used_bytes=memory_used,
            memory_total_bytes=memory_total,
        )
