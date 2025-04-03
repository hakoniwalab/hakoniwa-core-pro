#ifndef _HAKO_ASSET_IMPL_MEASURE_HPP_
#define _HAKO_ASSET_IMPL_MEASURE_HPP_

#if !defined(NDEBUG)
#define HAKO_IMPL_ASSERT(expr)	\
do {	\
	if (!(expr))	{	\
		printf("ASSERTION FAILED:%s:%s:%d:%s", __FILE__, __FUNCTION__, __LINE__, #expr);	\
		assert(!(expr));	\
	}	\
} while (0)
#else
#define HAKO_IMPL_ASSERT(ignore) ((void)0)
#endif
extern void * hako_asset_impl_measure_create_csv(const char* asset_name);
extern void hako_asset_impl_measure_write_csv(void* vp, long long int core_time, long long int asset_time);
extern void hako_asset_impl_measure_flush_csv(void* vp);
extern void hako_asset_impl_measure_close_csv(void* vp);

#endif /* _HAKO_ASSET_IMPL_MEASURE_HPP_ */
