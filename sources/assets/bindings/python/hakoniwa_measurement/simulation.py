from __future__ import annotations

from .models import SimulationExecutionResult


class SimulationExecutionMeter:
    """Measure one virtual-time event window using Hakoniwa world time."""

    def __init__(self, world_step_usec: int) -> None:
        if world_step_usec <= 0:
            raise ValueError("world_step_usec must be > 0")
        self._world_step_usec = int(world_step_usec)
        self._world_start_usec: int | None = None
        self._wall_start_ns: int | None = None
        self._finished = False

    def start(self, world_time_usec: int, monotonic_time_ns: int) -> None:
        if self._world_start_usec is not None:
            raise RuntimeError("simulation execution measurement already started")
        self._world_start_usec = int(world_time_usec)
        self._wall_start_ns = int(monotonic_time_ns)

    def finish(
        self, world_time_usec: int, monotonic_time_ns: int
    ) -> SimulationExecutionResult:
        if self._world_start_usec is None or self._wall_start_ns is None:
            raise RuntimeError("simulation execution measurement has not started")
        if self._finished:
            raise RuntimeError("simulation execution measurement already finished")
        self._finished = True

        world_end_usec = int(world_time_usec)
        wall_end_ns = int(monotonic_time_ns)
        world_elapsed_usec = world_end_usec - self._world_start_usec
        wall_elapsed_ns = wall_end_ns - self._wall_start_ns
        step_count, remainder = divmod(
            max(0, world_elapsed_usec), self._world_step_usec
        )
        wall_clock_sec = wall_elapsed_ns / 1_000_000_000.0
        average_step = (
            wall_clock_sec / step_count if step_count > 0 and wall_clock_sec >= 0 else None
        )
        rtf = (
            (world_elapsed_usec / 1_000_000.0) / wall_clock_sec
            if world_elapsed_usec >= 0 and wall_clock_sec > 0
            else None
        )
        return SimulationExecutionResult(
            world_time_start_usec=self._world_start_usec,
            world_time_end_usec=world_end_usec,
            world_elapsed_usec=world_elapsed_usec,
            world_step_usec=self._world_step_usec,
            step_count=step_count,
            step_remainder_usec=remainder,
            wall_clock_sec=wall_clock_sec,
            average_step_wall_clock_sec=average_step,
            rtf=rtf,
        )
