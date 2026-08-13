from __future__ import annotations

import json
import platform
import sys
import tempfile
import unittest
from pathlib import Path


PACKAGE_ROOT = (
    Path(__file__).resolve().parents[2] / "sources" / "assets" / "bindings" / "python"
)
sys.path.insert(0, str(PACKAGE_ROOT))

from hakoniwa_measurement import (  # noqa: E402
    HakoniwaTimeObserver,
    JsonLinesWriter,
    MachineResourceMonitor,
    MachineResourceSample,
    MeasurementResultSet,
    SimulationExecutionMeter,
    validate_result_set,
    write_json_atomic,
)
from hakoniwa_measurement.platform.factory import (  # noqa: E402
    create_host_resource_backend,
)
from hakoniwa_measurement.platform.linux.host_resources import (  # noqa: E402
    LinuxHostResourceBackend,
)


class _FakeBackend:
    backend_id = "fake"

    def __init__(self) -> None:
        self.calls = 0

    def sample(self, monotonic_time_ns: int) -> MachineResourceSample:
        self.calls += 1
        return MachineResourceSample(
            monotonic_time_ns=monotonic_time_ns,
            cpu_percent=None if self.calls == 1 else float(10 * self.calls),
            memory_used_bytes=100 * self.calls,
            memory_total_bytes=1000,
        )


class MeasurementTest(unittest.TestCase):
    def test_simulation_execution_uses_world_time_delta_for_step_count(self) -> None:
        meter = SimulationExecutionMeter(world_step_usec=20_000)
        meter.start(world_time_usec=2_000_000, monotonic_time_ns=1_000_000_000)
        result = meter.finish(
            world_time_usec=20_000_000,
            monotonic_time_ns=3_500_000_000,
        )
        self.assertEqual(result.world_elapsed_usec, 18_000_000)
        self.assertEqual(result.step_count, 900)
        self.assertEqual(result.step_remainder_usec, 0)
        self.assertAlmostEqual(result.wall_clock_sec, 2.5)
        self.assertAlmostEqual(result.average_step_wall_clock_sec, 2.5 / 900)
        self.assertAlmostEqual(result.rtf, 7.2)

    def test_temporal_observer_accepts_only_stable_world_time(self) -> None:
        world_values = iter([100, 100, 120, 140])
        minimum_values = iter([80, 90])
        observer = HakoniwaTimeObserver(
            lambda: next(world_values), lambda: next(minimum_values)
        )
        accepted = observer.observe(monotonic_time_ns=1)
        rejected = observer.observe(monotonic_time_ns=2)
        self.assertTrue(accepted.accepted)
        self.assertEqual(accepted.lag_usec, 20)
        self.assertFalse(rejected.accepted)
        self.assertIsNone(rejected.lag_usec)
        result = observer.result()
        self.assertEqual(result.accepted_sample_count, 1)
        self.assertEqual(result.rejected_sample_count, 1)
        self.assertEqual(result.acceptance_ratio, 0.5)

    def test_machine_monitor_polls_only_when_due(self) -> None:
        backend = _FakeBackend()
        monitor = MachineResourceMonitor(1.0, backend=backend)
        monitor.start(monotonic_time_ns=0)
        self.assertIsNone(monitor.poll_if_due(monotonic_time_ns=999_999_999))
        first = monitor.poll_if_due(monotonic_time_ns=1_000_000_000)
        self.assertIsNotNone(first)
        self.assertIsNone(monitor.poll_if_due(monotonic_time_ns=1_500_000_000))
        second = monitor.poll_if_due(monotonic_time_ns=2_000_000_000)
        self.assertIsNotNone(second)
        result = monitor.finish()
        self.assertEqual(result.sample_count, 2)
        self.assertEqual(len(monitor.samples), 2)
        self.assertEqual(result.cpu_average_percent, 25.0)
        self.assertEqual(result.memory_used_average_bytes, 250.0)

    def test_result_validation_marks_misaligned_world_time_invalid(self) -> None:
        meter = SimulationExecutionMeter(world_step_usec=20_000)
        meter.start(0, 0)
        performance = meter.finish(25_000, 1_000_000_000)
        result = MeasurementResultSet(
            run_id="run-1", mode="performance", performance=performance
        )
        validation = validate_result_set(result)
        self.assertFalse(validation.passed)
        self.assertEqual(result.status, "invalid")
        checks = {check.name: check for check in validation.checks}
        self.assertFalse(checks["world_step_aligned"].passed)

    def test_failed_startup_is_a_valid_result_record_without_performance(self) -> None:
        result = MeasurementResultSet(
            run_id="run-failed",
            mode="performance",
            status="failed",
            failure_type="startup_failure",
        )
        self.assertTrue(result.validate().passed)
        self.assertEqual(result.status, "failed")

    def test_json_outputs_refuse_to_mix_with_existing_trial_data(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            samples = root / "samples.jsonl"
            with JsonLinesWriter(samples) as writer:
                writer.write({"run_id": "run-1", "sample_index": 0})
            with self.assertRaises(FileExistsError):
                JsonLinesWriter(samples)

            result_path = root / "result.json"
            meter = SimulationExecutionMeter(20_000)
            meter.start(0, 0)
            result = MeasurementResultSet(
                run_id="run-1",
                mode="performance",
                performance=meter.finish(20_000, 1_000_000),
            )
            result.validate()
            write_json_atomic(result_path, result)
            with self.assertRaises(FileExistsError):
                write_json_atomic(result_path, {"run_id": "run-2"})
            payload = json.loads(result_path.read_text(encoding="utf-8"))
            self.assertEqual(payload["run_id"], "run-1")
            self.assertNotIn("samples", payload)

    def test_linux_backend_parses_procfs_without_commands(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            proc = Path(temporary)
            (proc / "stat").write_text(
                "cpu  100 0 50 850 0 0 0 0 0 0\n", encoding="utf-8"
            )
            (proc / "meminfo").write_text(
                "MemTotal: 1000 kB\nMemAvailable: 400 kB\n", encoding="utf-8"
            )
            backend = LinuxHostResourceBackend(proc)
            first = backend.sample(1)
            self.assertIsNone(first.cpu_percent)
            self.assertEqual(first.memory_used_bytes, 600 * 1024)
            (proc / "stat").write_text(
                "cpu  160 0 70 870 0 0 0 0 0 0\n", encoding="utf-8"
            )
            second = backend.sample(2)
            self.assertEqual(second.cpu_percent, 80.0)

    def test_current_platform_backend_can_sample(self) -> None:
        backend = create_host_resource_backend()
        first = backend.sample(1)
        second = backend.sample(2)
        expected_backend = {
            "Linux": "linux-procfs",
            "Darwin": "macos-mach",
            "Windows": "windows-kernel32",
        }[platform.system()]
        self.assertEqual(backend.backend_id, expected_backend)
        self.assertGreater(first.memory_total_bytes, 0)
        self.assertGreaterEqual(first.memory_used_bytes, 0)
        self.assertLessEqual(first.memory_used_bytes, first.memory_total_bytes)
        self.assertTrue(second.cpu_percent is None or 0 <= second.cpu_percent <= 100)


if __name__ == "__main__":
    unittest.main()
