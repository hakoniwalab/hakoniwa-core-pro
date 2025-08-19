# launcher/hako_launch/loader.py
from __future__ import annotations

import json
from pathlib import Path
from typing import List, Optional, Tuple

from pydantic import BaseModel, Field
from .model import LauncherSpec, Asset, Defaults
from .effective_model import EffectiveAsset, EffectiveSpec

def _normalize_path(base: Path, p: Optional[str | Path]) -> Optional[Path]:
    if p is None:
        return None
    p = Path(p)
    return p if p.is_absolute() else (base / p).resolve()


def _defaults_or_empty(d: Optional[Defaults]) -> Defaults:
    return d or Defaults()  # pydantic のデフォルト値が効く


def load_spec(path: str | Path) -> Tuple[LauncherSpec, Path]:
    """
    JSON を読み込み pydantic で型チェックした生の LauncherSpec を返す。
    base_dir（ファイルの場所）も一緒に返す。
    """
    path = Path(path).expanduser().resolve()
    data = json.loads(path.read_text(encoding="utf-8"))
    spec = LauncherSpec(**data)
    return spec, path.parent


def _effective_asset(asset: Asset, defaults: Defaults, base_dir: Path) -> EffectiveAsset:
    # 欠落フィールドに defaults を適用
    cwd = asset.cwd if asset.cwd is not None else defaults.cwd
    stdout = asset.stdout if asset.stdout is not None else defaults.stdout
    stderr = asset.stderr if asset.stderr is not None else defaults.stderr

    delay = asset.delay_sec if asset.delay_sec is not None else defaults.delay_sec
    # start_grace は None の場合 defaults を使い、最終的に非 Optional へ
    start_grace = asset.start_grace_sec if asset.start_grace_sec is not None else defaults.start_grace_sec

    # パスを正規化（相対 → launch.json の場所基準で絶対化）
    cwd_abs = _normalize_path(base_dir, cwd) or base_dir
    # stdout/stderr は相対指定なら base_dir 基準に置く
    stdout_abs = _normalize_path(base_dir, stdout)
    stderr_abs = _normalize_path(base_dir, stderr)

    # command は cwd から相対解決される運用が多いので絶対化はしない
    # ただし、後段 runner では cwd を chdir に使うので十分。
    return EffectiveAsset(
        name=asset.name,
        command=asset.command,
        args=list(asset.args or []),
        cwd=cwd_abs,
        stdout=stdout_abs,
        stderr=stderr_abs,
        delay_sec=float(delay),
        depends_on=list(asset.depends_on or []),
        start_grace_sec=float(start_grace),
        env=(asset.env.dict(exclude_none=True) if asset.env else
             (defaults.env.dict(exclude_none=True) if defaults.env else None)),
    )


def apply_defaults_and_normalize(spec: LauncherSpec, base_dir: Path) -> EffectiveSpec:
    """
    defaults を各 asset に反映し、パスを launch.json からの相対で解決。
    depends_on のトポ順で並べ替える（循環は例外）。
    """
    defaults = _defaults_or_empty(spec.defaults)
    ordered = spec.toposort_assets()  # 依存関係が無ければ元順を保持

    eff_assets = [_effective_asset(a, defaults, base_dir) for a in ordered]

    notify_dict = spec.notify.model_dump() if spec.notify is not None else None
    return EffectiveSpec(
        base_dir=base_dir,
        version=spec.version,
        assets=eff_assets,
        notify=notify_dict,
    )


def load(path: str | Path) -> Tuple[LauncherSpec, EffectiveSpec]:
    """
    これ一発で OK：
      - JSON 読み込み & 型チェック
      - defaults 反映
      - パス正規化
      - トポ順適用
    """
    launcher_spec, base_dir = load_spec(path)
    return launcher_spec, apply_defaults_and_normalize(launcher_spec, base_dir)
