# Native and Python runtime error contract

Hakoniwa contains C++ implementations behind C and CPython boundaries. No C++
exception may cross those boundaries. Public runtime operations use the common
guard in `hako_runtime_error_internal.hpp` and retain their historical success
and failure return values.

## Native status

Include `hako_runtime_error.h` and inspect the thread-local result immediately
after a failed call:

```c
int rc = hako_asset_pdu_create("Robot", 10, 64);
if (rc != 0) {
    HakoRuntimeErrorStatus status = hako_runtime_last_error_status();
    const char *detail = hako_runtime_last_error();
}
```

The status values are stable API values:

- `HAKO_RUNTIME_ERROR_NONE`: no guarded native exception occurred in the most
  recent call on this thread. Ordinary validation failures retain their
  existing return code and this status.
- `HAKO_RUNTIME_ERROR_ABORT`: Core positively identified an abandoned master
  lock or reused owner PID. The session must fail-stop.
- `HAKO_RUNTIME_ERROR_UNKNOWN`: Core cannot safely classify the lock owner or
  file format. This is not proof of owner death and must not be promoted to
  `ABORT`.
- `HAKO_RUNTIME_ERROR_OTHER`: another C++ exception was contained at the ABI.

The diagnostic string and status are thread-local. A later guarded call on the
same thread clears them before it runs, so callers must read them immediately.
No getter takes a Hakoniwa lock.

When linking the historical build where `conductor` is a separate shared
library backed by static Core state, read synchronous conductor failures with
`hako_conductor_last_error_status()` and `hako_conductor_last_error()`.
`hakopy` performs this selection internally. Other runtime libraries expose
the generic getters above.

## Python status

`hakopy` keeps the existing `RuntimeError` type. Runtime errors now carry two
stable attributes:

```python
try:
    hakopy.pdu_create("Robot", 10, 64)
except RuntimeError as error:
    print(error.hako_status)       # "ABORT", "UNKNOWN", or "OTHER"
    print(error.hako_status_code)  # 1, 2, or 3
```

Programs should branch on `hako_status` or `hako_status_code`, not English
message text. `UNKNOWN` remains diagnostically distinct from confirmed
`ABORT`. No binding performs recovery or lock takeover.

## Guarded surfaces

- callback asset lifecycle, start/wait/step, PDU, receive-event, and trigger
  operations in `hako_asset.h`;
- polling/master/simulation-event operations in `hako_capi.h` and their
  `hakoniwa_asset_polling.h` forwarding API;
- all server and client operations in `hako_asset_service.h`;
- conductor start/stop operations in `hako_conductor.h`;
- every corresponding CPython method, including the direct service lookup
  helper.

The following functions intentionally have no exception guard because they
only read or clear thread-local diagnostic storage:
`hako_runtime_last_error_status`, `hako_runtime_last_error`,
`hako_runtime_error_status_name`, `hako_runtime_clear_error`, and the legacy
alias `hako_asset_last_error`. The conductor-specific getters are likewise
read-only aliases for that library's thread-local state. The inline
`hako_asset_start` and
`hako_asset_usleep` helpers only forward to guarded functions.

The conductor worker thread also contains execution exceptions so they cannot
terminate the process; it logs the failure and marks the worker stopped. Its
diagnostic belongs to that worker thread, while synchronous start/stop errors
are reported to the calling thread through the native contract above.
