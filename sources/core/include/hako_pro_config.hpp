#pragma once

/* Keep fallback values aligned with cmake/hako_build_defaults.conf. */
#ifndef HAKO_RECV_EVENT_MAX
#define HAKO_RECV_EVENT_MAX 4096
#endif
#ifndef HAKO_CLIENT_NAMELEN_MAX
#define HAKO_CLIENT_NAMELEN_MAX 64
#endif
#ifndef HAKO_SERVICE_NAMELEN_MAX
#define HAKO_SERVICE_NAMELEN_MAX 128
#endif
#ifndef HAKO_SERVICE_CLIENT_MAX
#define HAKO_SERVICE_CLIENT_MAX 256
#endif
#ifndef HAKO_SERVICE_MAX
#define HAKO_SERVICE_MAX 1024
#endif
