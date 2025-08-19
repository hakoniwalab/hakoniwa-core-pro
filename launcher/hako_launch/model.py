# launcher/hako_launch/model.py
from __future__ import annotations

from typing import Dict, List, Optional, Union, Literal, Iterable
from pydantic import BaseModel, Field, AnyHttpUrl, ConfigDict, NonNegativeFloat, field_validator


# =========================
# EnvOps（環境変数操作の素データ）
# =========================
class EnvOps(BaseModel):
    """環境変数の合成指定（実際の合成は envmerge 側で行う）"""
    model_config = ConfigDict(extra="forbid")

    set: Optional[Dict[str, str]] = None
    prepend: Optional[Dict[str, List[str]]] = None
    append: Optional[Dict[str, List[str]]] = None
    unset: Optional[List[str]] = None

    @field_validator("unset")
    @classmethod
    def _dedup_unset(cls, v: Optional[List[str]]) -> Optional[List[str]]:
        if not v:
            return v
        seen, out = set(), []
        for x in v:
            if x not in seen:
                seen.add(x)
                out.append(x)
        return out


# =========================
# Defaults / Asset
# =========================
class Defaults(BaseModel):
    """assets 共通の既定値。asset 側で上書き可能。"""
    model_config = ConfigDict(extra="forbid")

    cwd: Optional[str] = None
    stdout: Optional[str] = None
    stderr: Optional[str] = None
    env: Optional[EnvOps] = None

    # 秒（小数可）
    start_grace_sec: NonNegativeFloat = 5.0
    delay_sec: NonNegativeFloat = 3.0


class Asset(BaseModel):
    """起動対象1件の定義。"""
    model_config = ConfigDict(extra="forbid")

    name: str
    command: str
    args: List[str] = Field(default_factory=list)

    cwd: Optional[str] = None
    env: Optional[EnvOps] = None
    stdout: Optional[str] = None
    stderr: Optional[str] = None

    # このアセット起動後に挟む待ち時間（秒。0で待機なし）
    delay_sec: Optional[NonNegativeFloat] = None

    # 依存（必要なら使用）。未使用なら空配列のままでOK。
    depends_on: List[str] = Field(default_factory=list)

    # 起動安定化の猶予（将来の判定用）。未指定なら defaults を使用。
    start_grace_sec: Optional[NonNegativeFloat] = None

    @field_validator("depends_on")
    @classmethod
    def _dedup_depends(cls, v: List[str]) -> List[str]:
        seen, out = set(), []
        for x in v:
            if x not in seen:
                seen.add(x)
                out.append(x)
        return out


# =========================
# Notify（判別ユニオン）
# =========================
class NotifyWebhook(BaseModel):
    model_config = ConfigDict(extra="forbid")
    type: Literal["webhook"] = "webhook"
    url: AnyHttpUrl
    secret: Optional[str] = None


class NotifyExec(BaseModel):
    model_config = ConfigDict(extra="forbid")
    type: Literal["exec"] = "exec"
    command: str
    args: List[str] = Field(default_factory=list)


Notify = Union[NotifyWebhook, NotifyExec]  # discriminator は LaunchSpec 側で指定


# =========================
# ルート：LauncherSpec
# =========================
class LauncherSpec(BaseModel):
    """ランチャ設定のルート。"""
    model_config = ConfigDict(extra="forbid", use_enum_values=True)

    version: Optional[str] = None
    defaults: Optional[Defaults] = None
    assets: List[Asset]
    notify: Optional[Notify] = Field(default=None, discriminator="type")

    # -------- ユーティリティ（軽いものだけ） --------
    def asset_names(self) -> List[str]:
        return [a.name for a in self.assets]

    def find_asset(self, name: str) -> Optional[Asset]:
        for a in self.assets:
            if a.name == name:
                return a
        return None

    def toposort_assets(self) -> List[Asset]:
        """
        depends_on を考慮したトポロジカルソート。
        依存が空なら元の順序を保つ。循環があれば ValueError。
        """
        # グラフ構築
        name_to_asset = {a.name: a for a in self.assets}
        indeg: Dict[str, int] = {a.name: 0 for a in self.assets}
        adj: Dict[str, List[str]] = {a.name: [] for a in self.assets}

        for a in self.assets:
            for dep in a.depends_on:
                if dep not in name_to_asset:
                    raise ValueError(f"depends_on refers to unknown asset: {a.name} -> {dep}")
                indeg[a.name] += 1
                adj[dep].append(a.name)

        # Kahn法（元の並び順を保つため、キュー初期順は self.assets の順）
        queue: List[str] = [a.name for a in self.assets if indeg[a.name] == 0]
        result: List[str] = []
        while queue:
            n = queue.pop(0)
            result.append(n)
            for m in adj[n]:
                indeg[m] -= 1
                if indeg[m] == 0:
                    queue.append(m)

        if len(result) != len(self.assets):
            raise ValueError("depends_on has a cycle")

        return [name_to_asset[n] for n in result]

    def iter_assets_with_defaults(self) -> Iterable[tuple[Asset, Defaults]]:
        """
        各 asset と、適用対象の Defaults（欠落時は空の既定値）を返す。
        実際の値の確定（None の補完や env の合成）は loader/envmerge 側で行う想定。
        """
        d = self.defaults or Defaults()
        for a in self.assets:
            yield a, d
