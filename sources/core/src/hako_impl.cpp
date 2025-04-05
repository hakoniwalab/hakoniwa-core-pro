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

static std::shared_ptr<hako::data::pro::HakoProData> pro_data_ptr = nullptr;
static std::shared_ptr<hako::data::HakoMasterData> master_data_ptr = nullptr;
static std::shared_ptr<hako::IHakoMasterController> master_ptr = nullptr;
static std::shared_ptr<hako::IHakoAssetController> asset_ptr = nullptr;
static std::shared_ptr<hako::IHakoSimulationEventController> simevent_ptr = nullptr;

bool hako::init()
{
    //hako::utils::logger::init("core");
    if (master_data_ptr == nullptr) {
        HakoConfigType config;
        hako_config_load(config);
        master_data_ptr = std::make_shared<hako::data::HakoMasterData>();
        if (config.param == nullptr) {
            std::cout << "WARN: hako::init() can not find cpp_core_config.json" << std::endl;
            master_data_ptr->init("shm");
        }
        else {
            master_data_ptr->init(config.param["shm_type"]);
        }
    }
    if (pro_data_ptr == nullptr) {
        pro_data_ptr = std::make_shared<hako::data::pro::HakoProData>(master_data_ptr);
    }
    HAKO_LOG_INFO("hako::init(): shared memory type = %s", master_data_ptr->get_shm_type().c_str());
    //hako::utils::logger::get("core")->info("hakoniwa initialized");
    return true;
}
void hako::destroy()
{
    hako::core::context::HakoContext context;
    if (master_ptr != nullptr) {
        master_ptr = nullptr;
    }
    if (!context.is_same(master_data_ptr->get_master_pid())) {
        return;
    }
    if (pro_data_ptr != nullptr) {
        pro_data_ptr->destroy();
        pro_data_ptr = nullptr;
    }
    if (master_data_ptr != nullptr) {
        master_data_ptr->destroy();
        master_data_ptr = nullptr;
    }
    if (asset_ptr != nullptr) {
        asset_ptr = nullptr;
    }
    if (simevent_ptr != nullptr) {
        simevent_ptr = nullptr;
    }
    //hako::utils::logger::get("core")->info("hakoniwa destroyed");
    //hako::utils::logger::get("core")->flush();
    return;
}

std::shared_ptr<hako::IHakoMasterController> hako::create_master()
{
    HAKO_ASSERT(master_data_ptr != nullptr);
    return master_ptr;
}

std::shared_ptr<hako::IHakoAssetController> hako::create_asset_controller()
{
    if (asset_ptr != nullptr) {
        return asset_ptr;
    }
    else if (master_data_ptr == nullptr) {
        master_data_ptr = std::make_shared<hako::data::HakoMasterData>();
        if (pro_data_ptr == nullptr) {
            pro_data_ptr = std::make_shared<hako::data::pro::HakoProData>(master_data_ptr);
        }
        HakoConfigType config;
        hako_config_load(config);
        if (config.param == nullptr) {
            std::cout << "hako::create_asset_controller() can not find cpp_core_config.json" << std::endl;
            return nullptr;
        }
        if (master_data_ptr->load(config.param["shm_type"]) == false) {
            return nullptr;
        }
    }
    asset_ptr = std::make_shared<hako::HakoAssetControllerImpl>(master_data_ptr);
    asset_ptr->register_asset_extension(pro_data_ptr->get_asset_extension());
    HAKO_LOG_INFO("hako::create_asset_controller(): shared memory type = %s", master_data_ptr->get_shm_type().c_str());

    return asset_ptr;
}

std::shared_ptr<hako::IHakoSimulationEventController> hako::get_simevent_controller()
{
    if (simevent_ptr != nullptr) {
        return simevent_ptr;
    }
    else if (master_data_ptr == nullptr) {
        master_data_ptr = std::make_shared<hako::data::HakoMasterData>();
        if (pro_data_ptr == nullptr) {
            pro_data_ptr = std::make_shared<hako::data::pro::HakoProData>(master_data_ptr);
        }    
        HakoConfigType config;
        hako_config_load(config);
        if (config.param == nullptr) {
            std::cout << "hako::get_simevent_controller() can not find cpp_core_config.json" << std::endl;
            return nullptr;
        }
        if (master_data_ptr->load(config.param["shm_type"]) == false) {
            return nullptr;
        }
    }
    simevent_ptr = std::make_shared<hako::HakoSimulationEventController>(master_data_ptr);
    HAKO_LOG_INFO("hako::get_simevent_controller(): shared memory type = %s", master_data_ptr->get_shm_type().c_str());
    return simevent_ptr;
}

void hako::log::add(const char* level, const char* file, int line, const char* function, const char* format, ...)
{
    if (!master_data_ptr) {
        return;
    }
    if (master_data_ptr->is_locked()) {
        std::cout << "WARN: hako log is discarded because of master lock is locked...." << std::endl;
        return;
    }
    va_list args;
    va_start(args, format);
    master_data_ptr->add_log_internal(level, file, line, function, format, args);    
    va_end(args);
}
