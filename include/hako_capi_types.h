#ifndef _HAKO_CAPI_TYPES_H_
#define _HAKO_CAPI_TYPES_H_

#include <stddef.h>
#include "hako_export.h"

typedef long long int hako_time_t;
typedef struct {
    void (*start) ();
    void (*stop) ();
    void (*reset) ();
} hako_asset_callback_t;

typedef int HakoPduChannelIdType;

#endif /* _HAKO_CAPI_TYPES_H_ */