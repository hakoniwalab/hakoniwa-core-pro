from __future__ import annotations

import math
import statistics
import time
from collections.abc import Callable

from .models import HakoniwaTimeResult, HakoniwaTimeSample


class HakoniwaTimeObserver:
    """Observe Core-to-slowest-participating-Asset lag without judging it."""

    def __init__(
        self,
        world_time: Callable[[], int],
        min_asset_time: Callable[[], int],
    ) -> None:
        self._world_time = world_time
        self._min_asset_time = min_asset_time
        self._samples: list[HakoniwaTimeSample] = []

    @classmethod
    def from_hakopy(cls) -> "HakoniwaTimeObserver":
        import hakopy

        return cls(hakopy.simulation_time, hakopy.min_asset_time)

    def observe(self, monotonic_time_ns: int | None = None) -> HakoniwaTimeSample:
        observed_ns = time.monotonic_ns() if monotonic_time_ns is None else int(monotonic_time_ns)
        world_before = int(self._world_time())
        minimum_asset = int(self._min_asset_time())
        world_after = int(self._world_time())
        accepted = world_before == world_after and minimum_asset <= world_before
        sample = HakoniwaTimeSample(
            monotonic_time_ns=observed_ns,
            world_time_before_usec=world_before,
            min_asset_time_usec=minimum_asset,
            world_time_after_usec=world_after,
            accepted=accepted,
            lag_usec=world_before - minimum_asset if accepted else None,
        )
        self._samples.append(sample)
        return sample

    def result(self) -> HakoniwaTimeResult:
        lags = sorted(
            sample.lag_usec
            for sample in self._samples
            if sample.accepted and sample.lag_usec is not None
        )
        accepted = len(lags)
        rejected = len(self._samples) - accepted
        total = len(self._samples)
        if lags:
            rank = max(0, math.ceil(0.95 * len(lags)) - 1)
            median = float(statistics.median(lags))
            p95 = float(lags[rank])
            maximum = int(lags[-1])
        else:
            median = None
            p95 = None
            maximum = None
        return HakoniwaTimeResult(
            sample_count=total,
            lag_median_usec=median,
            lag_p95_usec=p95,
            lag_max_usec=maximum,
            accepted_sample_count=accepted,
            rejected_sample_count=rejected,
            acceptance_ratio=accepted / total if total else 0.0,
        )

    @property
    def samples(self) -> tuple[HakoniwaTimeSample, ...]:
        return tuple(self._samples)
