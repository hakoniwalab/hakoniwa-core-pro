#ifndef _HAKO_CONDUCTOR_H_
#define _HAKO_CONDUCTOR_H_


#include "hako_primitive_types.h"
#include "hako_runtime_error.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int hako_conductor_start(hako_time_t delta_usec, hako_time_t max_delay_usec);
extern void hako_conductor_stop(void);
/* Use these accessors when conductor is a separate shared library. */
extern HakoRuntimeErrorStatus hako_conductor_last_error_status(void);
extern const char* hako_conductor_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* _HAKO_CONDUCTOR_H_ */
