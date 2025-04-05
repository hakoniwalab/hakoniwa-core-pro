#pragma once

#include "hako.hpp"
#include "hako_master_impl.hpp"
#include "hako_asset_impl.hpp"
#include "hako_simevent_impl.hpp"
#include "utils/hako_config_loader.hpp"
//#include "utils/hako_logger.hpp"
#include "core/context/hako_context.hpp"
#include "hako_log.hpp"
#include <cstdarg>
#include "hako_pro_data.hpp"

namespace hako::data::pro {
    std::shared_ptr<HakoProData> hako_pro_get_data();    
}
