from __future__ import annotations

import statistics
import time

from .models import MachineResourceResult, MachineResourceSample
from .platform import HostResourceBackend, create_host_resource_backend


class MachineResourceMonitor:
    """Collect host CPU and memory samples on a wall-clock schedule."""

    def __init__(
        self,
        sampling_interval_sec: float,
        backend: HostResourceBackend | None = None,
    ) -> None:
        if sampling_interval_sec <= 0:
            raise ValueError("sampling_interval_sec must be > 0")
        self._interval_sec = float(sampling_interval_sec)
        self._interval_ns = int(sampling_interval_sec * 1_000_000_000)
        self._backend = backend or create_host_resource_backend()
        self._next_sample_ns: int | None = None
        self._samples: list[MachineResourceSample] = []
        self._finished = False

    def start(self, monotonic_time_ns: int | None = None) -> None:
        if self._next_sample_ns is not None:
            raise RuntimeError("machine resource measurement already started")
        start_ns = time.monotonic_ns() if monotonic_time_ns is None else int(monotonic_time_ns)
        # Prime cumulative CPU counters. The first sample has no interval CPU
        # percentage and is intentionally excluded from the raw result.
        self._backend.sample(start_ns)
        self._next_sample_ns = start_ns + self._interval_ns

    def poll_if_due(
        self, monotonic_time_ns: int | None = None
    ) -> MachineResourceSample | None:
        if self._next_sample_ns is None:
            raise RuntimeError("machine resource measurement has not started")
        if self._finished:
            raise RuntimeError("machine resource measurement already finished")
        observed_ns = time.monotonic_ns() if monotonic_time_ns is None else int(monotonic_time_ns)
        if observed_ns < self._next_sample_ns:
            return None
        sample = self._backend.sample(observed_ns)
        self._samples.append(sample)
        skipped_intervals = max(
            1, (observed_ns - self._next_sample_ns) // self._interval_ns + 1
        )
        self._next_sample_ns += skipped_intervals * self._interval_ns
        return sample

    def sample_now(
        self, monotonic_time_ns: int | None = None
    ) -> MachineResourceSample:
        """Collect one sample immediately, independent of the polling schedule."""
        if self._next_sample_ns is None:
            raise RuntimeError("machine resource measurement has not started")
        if self._finished:
            raise RuntimeError("machine resource measurement already finished")
        observed_ns = time.monotonic_ns() if monotonic_time_ns is None else int(monotonic_time_ns)
        sample = self._backend.sample(observed_ns)
        self._samples.append(sample)
        return sample

    def finish(self) -> MachineResourceResult:
        if self._next_sample_ns is None:
            raise RuntimeError("machine resource measurement has not started")
        if self._finished:
            raise RuntimeError("machine resource measurement already finished")
        self._finished = True
        cpu = [sample.cpu_percent for sample in self._samples if sample.cpu_percent is not None]
        memory = [sample.memory_used_bytes for sample in self._samples]
        memory_percent = [sample.memory_used_percent for sample in self._samples]
        invalid = sum(
            1
            for sample in self._samples
            if (sample.cpu_percent is not None and not 0.0 <= sample.cpu_percent <= 100.0)
            or not 0 <= sample.memory_used_bytes <= sample.memory_total_bytes
        )
        return MachineResourceResult(
            backend_id=self._backend.backend_id,
            sampling_interval_sec=self._interval_sec,
            sample_count=len(self._samples),
            cpu_sample_count=len(cpu),
            invalid_sample_count=invalid,
            cpu_average_percent=float(statistics.fmean(cpu)) if cpu else None,
            cpu_max_percent=max(cpu) if cpu else None,
            memory_used_average_bytes=float(statistics.fmean(memory)) if memory else None,
            memory_used_max_bytes=max(memory) if memory else None,
            memory_used_average_percent=(
                float(statistics.fmean(memory_percent)) if memory_percent else None
            ),
            memory_used_max_percent=max(memory_percent) if memory_percent else None,
        )

    @property
    def samples(self) -> tuple[MachineResourceSample, ...]:
        return tuple(self._samples)
