# Polling service example

A service (RPC) server and client that use no Hakoniwa callbacks. Each program
checks the simulation state in its own loop and calls only the APIs allowed in
that state. This is the style a game engine has to use, because it cannot hand
its main loop to Hakoniwa.

| Program | Role |
|---|---|
| `polling_conductor` | The conductor, in its own process |
| `polling_server` | `Service/Add` server that does not register as an asset |
| `polling_asset_server` | `Service/Add` server that registers as a polling asset and takes part in the time synchronization (`asset_loop.hpp`). `--hold-first-request` leaves the first request unanswered |
| `polling_client` | `Service/Add` client. A polling asset that gives start/stop/reset feedback itself |
| `callback_server` | The same server in the callback style, for comparison. Same `--hold-first-request` option |
| `bench_server` / `bench_client` | Stage 5 benchmark with `hako_srv_msgs/RenderCamera` (no sleep). `bench_server --register` registers as a polling asset |
| `bench_callback_server` | The benchmark server in the callback style |

## When each API may be called

| API | Allowed |
|---|---|
| `hako_asset_service_initialize` | While Stopped, before the first start |
| `hako_asset_service_server_create` / `client_create` | After start, once `hakoniwa_asset_is_pdu_created()` returns true. That call also loads the PDU data and the service tables into the process |
| server poll / response, client request / poll | While Runnable or Running |

Service registrations are kept across start, stop and reset (hakoniwa-core-pro#109).
Only one run's state is cleared: the shared flags on start/reset, and the in-process
server/client state (for example a request left DOING) when the next start is seen.

A server that answers inside its simulation step needs the world time to move, so a
client keeps advancing its asset time while it waits for a response. A server that
does not register as an asset does not hold the world time back, so the simulation
time that passes before the response arrives is not bounded.

## Build

From the repository root:

```sh
cmake -S . -B cmake-build
cmake --build cmake-build -j4
```

## Run by hand

Use a runtime directory of your own so that no other Hakoniwa run is touched:

```sh
mkdir -p /tmp/hako-polling/mmap
printf '{"shm_type":"mmap","core_mmap_path":"/tmp/hako-polling/mmap","asset_timeout_usec":600000000}\n' > /tmp/hako-polling/cpp_core_config.json
export HAKO_CONFIG_PATH=/tmp/hako-polling/cpp_core_config.json
export DYLD_LIBRARY_PATH=./cmake-build/sources/assets/polling:./cmake-build/sources/assets/callback:./cmake-build/sources/conductor
```

On Linux, set `LD_LIBRARY_PATH` to the same value. Then start each program in its
own terminal (with the same environment), in this order:

```sh
./cmake-build/examples/service_polling/polling_conductor
```

```sh
./cmake-build/examples/service_polling/polling_server ./examples/service_polling/service.json
```

```sh
./cmake-build/examples/service_polling/polling_client ./examples/service_polling/service.json 3
```

```sh
./cmake-build/sources/command/hako-cmd start
./cmake-build/sources/command/hako-cmd stop
./cmake-build/sources/command/hako-cmd reset
./cmake-build/sources/command/hako-cmd start
```

The client prints `RESULT count=3 ...` after each start. Stop the programs with Ctrl-C.

## Restart test

Starts the simulation, stops it (and resets it), starts it again in the same
processes, and checks that the server still answers. Covers the server without
asset registration, the polling asset server and the callback server, and a
restart while a request is DOING:

```sh
bash examples/service_polling/test_restart.bash
```

## Benchmark (stage 5)

Round trip and its parts for image-sized responses, Hakoniwa only:

```sh
bash examples/service_polling/bench.bash 300
```

The sizes can be chosen with `SIZES="640x480 1920x1080"`, the server with
`SERVER_MODE=register` (polling asset) or `SERVER_MODE=callback`. Results are
recorded in hakoniwa-core-pro#110.
