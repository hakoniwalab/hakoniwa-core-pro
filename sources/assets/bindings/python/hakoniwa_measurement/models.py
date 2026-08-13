from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


@dataclass(frozen=True)
class MachineResourceSample:
    monotonic_time_ns: int
    cpu_percent: float | None
    memory_used_bytes: int
    memory_total_bytes: int
    memory_used_percent: float = field(init=False)

    def __post_init__(self) -> None:
        percent = (
            100.0 * self.memory_used_bytes / self.memory_total_bytes
            if self.memory_total_bytes > 0
            else 0.0
        )
        object.__setattr__(self, "memory_used_percent", percent)


@dataclass(frozen=True)
class MachineResourceResult:
    backend_id: str
    sampling_interval_sec: float
    sample_count: int
    cpu_sample_count: int
    invalid_sample_count: int
    cpu_average_percent: float | None
    cpu_max_percent: float | None
    memory_used_average_bytes: float | None
    memory_used_max_bytes: int | None
    memory_used_average_percent: float | None
    memory_used_max_percent: float | None


@dataclass(frozen=True)
class HakoniwaTimeSample:
    monotonic_time_ns: int
    world_time_before_usec: int
    min_asset_time_usec: int
    world_time_after_usec: int
    accepted: bool
    lag_usec: int | None


@dataclass(frozen=True)
class HakoniwaTimeResult:
    sample_count: int
    lag_median_usec: float | None
    lag_p95_usec: float | None
    lag_max_usec: int | None
    accepted_sample_count: int
    rejected_sample_count: int
    acceptance_ratio: float


@dataclass(frozen=True)
class SimulationExecutionResult:
    world_time_start_usec: int
    world_time_end_usec: int
    world_elapsed_usec: int
    world_step_usec: int
    step_count: int
    step_remainder_usec: int
    wall_clock_sec: float
    average_step_wall_clock_sec: float | None
    rtf: float | None


@dataclass(frozen=True)
class ValidationCheck:
    name: str
    passed: bool
    expected: Any = None
    actual: Any = None
    message: str | None = None


@dataclass(frozen=True)
class ValidationResult:
    passed: bool
    checks: tuple[ValidationCheck, ...]


@dataclass
class MeasurementResultSet:
    run_id: str
    mode: str
    minimum_machine_cpu_sample_count: int = 1
    status: str = "success"
    failure_type: str | None = None
    performance: SimulationExecutionResult | None = None
    machine_preflight: MachineResourceResult | None = None
    machine: MachineResourceResult | None = None
    temporal: HakoniwaTimeResult | None = None
    metadata: dict[str, Any] = field(default_factory=dict)
    validation: ValidationResult | None = None

    def validate(self) -> ValidationResult:
        from .validation import validate_result_set

        return validate_result_set(self)
