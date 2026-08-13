from __future__ import annotations

import sys

from .base import HostResourceBackend


class UnsupportedPlatformError(RuntimeError):
    pass


def create_host_resource_backend() -> HostResourceBackend:
    if sys.platform.startswith("linux"):
        from .linux.host_resources import LinuxHostResourceBackend

        return LinuxHostResourceBackend()
    if sys.platform == "darwin":
        from .macos.host_resources import MacOSHostResourceBackend

        return MacOSHostResourceBackend()
    if sys.platform == "win32":
        from .windows.host_resources import WindowsHostResourceBackend

        return WindowsHostResourceBackend()
    raise UnsupportedPlatformError(f"unsupported measurement platform: {sys.platform}")
