#include "hako_service.h"
#include "hako_service_impl.hpp"

/* SERVICE */
int hako_service_initialize(const char* service_config_path)
{
    return hako::service::impl::initialize(service_config_path);
}
