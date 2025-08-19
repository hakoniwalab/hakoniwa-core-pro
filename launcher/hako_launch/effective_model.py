
from __future__ import annotations

from pathlib import Path
from typing import List, Optional

from pydantic import BaseModel, Field


class EffectiveAsset(BaseModel):
    """
    defaults を反映済みの“実行用ビュー”。
    env はまだ合成しない（envmerge でやる）。
    パスは launch.json の場所を基準に相対→絶対へ正規化しておく。
    """
    name: str
    command: str
    args: List[str] = Field(default_factory=list)
    cwd: Path
    stdout: Optional[Path] = None
    stderr: Optional[Path] = None
    delay_sec: float
    depends_on: List[str] = Field(default_factory=list)
    start_grace_sec: float  # defaults を必ず反映して non-optional にする

    # env は次段で扱うため素のまま保持
    env: Optional[dict] = None  # 型は EnvOps の dict 表現（合成は envmerge 側）


class EffectiveSpec(BaseModel):
    """ランチャー実行側がそのまま使える形。"""
    base_dir: Path                 # launch.json のあるディレクトリ
    version: Optional[str] = None
    assets: List[EffectiveAsset]
    notify: Optional[dict] = None  # runner 側で解釈（webhook/exec）