#pragma once


#include "data/hako_master_data.hpp"
#include "core/context/hako_context.hpp"
#include "hako_pro_config.hpp"
#include <memory>

namespace hako::data::pro {

/*
 * Hakoniwa Data Receive Event Definitions
 */
typedef enum {
    HAKO_RECV_EVENT_TYPE_FLAG = 0,
    HAKO_RECV_EVENT_TYPE_CALLBACK = 1
} HakoRecvEventType;

#define HAKO_ASSET_ID_EXTERNAL -1

typedef struct {
    bool enabled;
    bool recv_flag;
    pid_type proc_id;
    int real_channel_id;
    HakoRecvEventType type;
    void (*on_recv)(int recv_event_id);
} HakoRecvEventEntryType;

typedef struct {
    HakoRecvEventEntryType entries[HAKO_RECV_EVENT_MAX];
    int entry_num;
} HakoRecvEventTableType;

/*
 * Hakoniwa Service Definitions
 */
#define HAKO_SERVICE_SERVER_CHANNEL_ID 0
#define HAKO_SERVICE_CLIENT_CHANNEL_ID 1
#define HAKO_SERVICE_SERVER_CHANNEL_ID_MAX 2
typedef struct {
    bool enabled;
    int namelen;
    char clientName[HAKO_CLIENT_NAMELEN_MAX];
    int requestChannelId;
    int requestRecvEventId;
    int responseChannelId;
    int responseRecvEventId;
} HakoServiceClientChannelMapType;

typedef struct {
    bool enabled;
    int namelen;
    char serviceName[HAKO_SERVICE_NAMELEN_MAX];
    int maxClients;
    HakoServiceClientChannelMapType clientChannelMap[HAKO_SERVICE_CLIENT_MAX];
} HakoServiceEntryTye;

typedef struct {
    int entry_num;
    HakoServiceEntryTye entries[HAKO_SERVICE_MAX];
} HakoServiceTableType;

}