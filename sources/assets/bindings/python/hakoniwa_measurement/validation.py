from __future__ import annotations

import math

from .models import MeasurementResultSet, ValidationCheck, ValidationResult


def validate_result_set(result: MeasurementResultSet) -> ValidationResult:
    checks: list[ValidationCheck] = []

    def check(
        name: str,
        passed: bool,
        *,
        expected=None,
        actual=None,
        message: str | None = None,
    ) -> None:
        checks.append(
            ValidationCheck(
                name=name,
                passed=bool(passed),
                expected=expected,
                actual=actual,
                message=message,
            )
        )

    check("run_id_present", bool(result.run_id), expected="non-empty", actual=result.run_id)
    check(
        "mode_supported",
        result.mode in {"performance", "temporal"},
        expected=["performance", "temporal"],
        actual=result.mode,
    )
    check(
        "status_supported",
        result.status in {"success", "failed", "invalid"},
        expected=["success", "failed", "invalid"],
        actual=result.status,
    )
    check(
        "failure_type_present_when_failed",
        result.status != "failed" or bool(result.failure_type),
        expected="non-empty for failed runs",
        actual=result.failure_type,
    )

    performance = result.performance
    check(
        "performance_result_present_when_successful",
        result.status != "success" or performance is not None,
        expected="SimulationExecutionResult for successful runs",
        actual=None if performance is None else "present",
    )
    if performance is not None:
        check("world_time_monotonic", performance.world_elapsed_usec >= 0, actual=performance.world_elapsed_usec)
        check("world_time_advanced", performance.world_elapsed_usec > 0, expected="> 0", actual=performance.world_elapsed_usec)
        check("world_step_positive", performance.world_step_usec > 0, expected="> 0", actual=performance.world_step_usec)
        check("world_step_aligned", performance.step_remainder_usec == 0, expected=0, actual=performance.step_remainder_usec)
        check("step_count_positive", performance.step_count > 0, expected="> 0", actual=performance.step_count)
        check("wall_clock_positive", performance.wall_clock_sec > 0, expected="> 0", actual=performance.wall_clock_sec)
        check(
            "average_step_finite",
            performance.average_step_wall_clock_sec is not None
            and math.isfinite(performance.average_step_wall_clock_sec),
            actual=performance.average_step_wall_clock_sec,
        )
        check(
            "rtf_finite",
            performance.rtf is not None and math.isfinite(performance.rtf),
            actual=performance.rtf,
        )

    if result.mode == "performance":
        check(
            "temporal_observer_disabled",
            result.temporal is None,
            expected=None,
            actual=None if result.temporal is None else "present",
        )
    elif result.mode == "temporal":
        check(
            "temporal_result_present_when_successful",
            result.status != "success" or result.temporal is not None,
            expected="HakoniwaTimeResult for successful temporal runs",
            actual=None if result.temporal is None else "present",
        )

    if result.machine_preflight is not None:
        check(
            "machine_preflight_samples_valid",
            result.machine_preflight.invalid_sample_count == 0,
            expected=0,
            actual=result.machine_preflight.invalid_sample_count,
        )
        check(
            "machine_preflight_cpu_available",
            result.machine_preflight.cpu_average_percent is not None,
            expected="at least one CPU interval sample",
            actual=result.machine_preflight.cpu_average_percent,
        )

    if result.machine is not None:
        check(
            "machine_samples_valid",
            result.machine.invalid_sample_count == 0,
            expected=0,
            actual=result.machine.invalid_sample_count,
        )
        check(
            "machine_cpu_available",
            result.machine.cpu_average_percent is not None,
            expected="at least one CPU interval sample",
            actual=result.machine.cpu_average_percent,
        )
        check(
            "machine_cpu_sample_count",
            result.machine.cpu_sample_count
            >= result.minimum_machine_cpu_sample_count,
            expected=f">= {result.minimum_machine_cpu_sample_count}",
            actual=result.machine.cpu_sample_count,
        )

    temporal = result.temporal
    if temporal is not None:
        sample_count = temporal.sample_count
        check(
            "temporal_sample_accounting",
            temporal.accepted_sample_count + temporal.rejected_sample_count
            == sample_count,
            expected=sample_count,
            actual=sample_count,
        )
        expected_ratio = (
            temporal.accepted_sample_count / sample_count if sample_count else 0.0
        )
        check(
            "temporal_acceptance_ratio",
            math.isclose(temporal.acceptance_ratio, expected_ratio),
            expected=expected_ratio,
            actual=temporal.acceptance_ratio,
        )

    validation = ValidationResult(
        passed=all(item.passed for item in checks), checks=tuple(checks)
    )
    result.validation = validation
    if result.status == "success" and not validation.passed:
        result.status = "invalid"
    return validation
