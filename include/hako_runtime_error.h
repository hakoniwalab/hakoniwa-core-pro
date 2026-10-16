#pragma once

#include "hako_export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAKO_RUNTIME_ERROR_NONE = 0,
    HAKO_RUNTIME_ERROR_ABORT = 1,
    HAKO_RUNTIME_ERROR_UNKNOWN = 2,
    HAKO_RUNTIME_ERROR_OTHER = 3,
} HakoRuntimeErrorStatus;

/*
 * Thread-local result of the most recent guarded Hakoniwa C API call.
 * A successful or ordinary validation-failure call clears this state to NONE.
 */
HAKO_API HakoRuntimeErrorStatus hako_runtime_last_error_status(void);
HAKO_API const char* hako_runtime_last_error(void);
HAKO_API const char* hako_runtime_error_status_name(HakoRuntimeErrorStatus status);
HAKO_API void hako_runtime_clear_error(void);

#ifdef __cplusplus
}
#endif

