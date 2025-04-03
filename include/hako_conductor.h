#ifndef _HAKO_CONDUCTOR_H_
#define _HAKO_CONDUCTOR_H_


#include "hako_primitive_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int hako_conductor_start(hako_time_t delta_usec, hako_time_t max_delay_usec);
extern void hako_conductor_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* _HAKO_CONDUCTOR_H_ */