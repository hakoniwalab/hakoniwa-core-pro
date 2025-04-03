#ifndef _HAKO_CONDUCTOR_IMPL_HPP_
#define _HAKO_CONDUCTOR_IMPL_HPP_

#include "hako_conductor.h"
#include "hako.hpp"
#include <fstream>
#include <iostream>
#include "stdio.h"
#include "assert.h"

#define HAKO_CONDUCTOR_ASSERT(expr)	\
do {	\
	if (!(expr))	{	\
		printf("HAKO ASSERTION FAILED:%s:%s:%d:%s", __FILE__, __FUNCTION__, __LINE__, #expr);	\
		assert(!(expr));	\
	}	\
} while (0)
#define HAKO_CONDUCTOR_WAIT_TIME_USEC (1000 * 10)

extern bool hako_conductor_impl_start(hako_time_t delta_usec, hako_time_t max_delay_usec);
extern void hako_conductor_impl_stop(void);

#endif /* _HAKO_ASSET_IMPL_HPP_ */
