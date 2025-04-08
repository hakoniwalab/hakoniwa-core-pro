#include "hako_asset_impl.hpp"
#ifdef WIN32
#else
#include <unistd.h>
#ifdef ENABLE_HAKO_TIME_MEASURE
#include "hako_asset_impl_measure.hpp"
#endif /* ENABLE_HAKO_TIME_MEASURE */
#endif
#include "hako_impl.hpp"

HakoAssetType hako_asset_instance;

/***********************
 * simulation control
 ***********************/
static hako::asset::PduReader* create_reader(const nlohmann::json &reader_json)
{
    hako::asset::PduReader* reader = new hako::asset::PduReader{
        reader_json["type"],
        reader_json["org_name"],
        reader_json["name"],
        reader_json["channel_id"],
        reader_json["pdu_size"],
        reader_json["write_cycle"],
    };
    HAKO_ASSET_ASSERT(reader != nullptr);
    return reader;
}
static hako::asset::PduWriter* create_writer(const nlohmann::json &writer_json)
{
    hako::asset::PduWriter* writer = new hako::asset::PduWriter{
        writer_json["type"],
        writer_json["org_name"],
        writer_json["name"],
        writer_json["write_cycle"],
        writer_json["channel_id"],
        writer_json["pdu_size"],
        writer_json["method_type"]
    };
    HAKO_ASSET_ASSERT(writer != nullptr);
    return writer;
}
static void hako_asset_impl_parse_robots(bool is_plant)
{
    const json& robots_json = hako_asset_instance.param["robots"];
    
    for (const auto& robot_json : robots_json) {
        hako::asset::Robot* robot = new hako::asset::Robot;
        robot->name = robot_json["name"];
        if (robot_json.find("shm_pdu_writers") != robot_json.end()) {
            const json& pdu_readers_json = robot_json["shm_pdu_writers"];
            for (const auto& reader_json : pdu_readers_json) {
                if (is_plant) {
                    robot->pdu_writers.push_back(*create_writer(reader_json));
                }
                else {
                    robot->pdu_readers.push_back(*create_reader(reader_json));
                }
            }
        } else {
            // nothing to do
        }
        if (robot_json.find("rpc_pdu_writers") != robot_json.end()) {
            const json& pdu_readers_json = robot_json["rpc_pdu_writers"];
            for (const auto& reader_json : pdu_readers_json) {
                if (is_plant) {
                    robot->pdu_writers.push_back(*create_writer(reader_json));
                }
                else {
                    robot->pdu_readers.push_back(*create_reader(reader_json));
                }
            }
        } else {
            // nothing to do
        }
        if (robot_json.find("shm_pdu_readers") != robot_json.end()) {
            const json& pdu_writers_json = robot_json["shm_pdu_readers"];
            for (const auto& writer_json : pdu_writers_json) {
                if (is_plant) {
                    robot->pdu_readers.push_back(*create_reader(writer_json));
                }
                else {
                    robot->pdu_writers.push_back(*create_writer(writer_json));
                }
            }
        } else {
            // nothing to do
        }
        if (robot_json.find("rpc_pdu_readers") != robot_json.end()) {
            const json& pdu_writers_json = robot_json["rpc_pdu_readers"];
            for (const auto& writer_json : pdu_writers_json) {
                if (is_plant) {
                    robot->pdu_readers.push_back(*create_reader(writer_json));
                }
                else {
                    robot->pdu_writers.push_back(*create_writer(writer_json));
                }
            }
        } else {
            // nothing to do
        }        
        hako_asset_instance.robots.push_back(*robot);
    }
}

bool hako_asset_impl_init(const char* asset_name, const char* config_path, hako_time_t delta_usec, bool is_plant)
{
    hako_asset_instance.is_initialized = false;
    hako_asset_instance.external_use = false;
    std::ifstream ifs(config_path);
    
    if (!ifs.is_open()) {
        std::cerr << "Error: Failed to open config file." << std::endl;
        return false;
    }

    hako_asset_instance.asset_name_str = asset_name;
    hako_asset_instance.delta_usec = delta_usec;
    hako_asset_instance.current_usec = 0;
    try {
        hako_asset_instance.param = json::parse(ifs);
        hako_asset_impl_parse_robots(is_plant);
    } catch (const json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return false;
    }
    hako_asset_instance.hako_asset = hako::create_asset_controller();
    if (hako_asset_instance.hako_asset == nullptr) {
        std::cerr << "ERROR: Not found hako-master on this PC" << std::endl;
        return false;
    }
    hako_asset_instance.hako_sim = hako::get_simevent_controller();
    if (hako_asset_instance.hako_sim == nullptr) {
        std::cerr << "ERROR: Not found hako-master on this PC" << std::endl;
        return false;
    }
    bool ret = hako_asset_instance.hako_asset->asset_register_polling(asset_name);
    if (ret == false) {
        std::cerr << "ERROR: Can not register asset" << std::endl;
        return false;
    }
    // PDUのチャネルを作成する
    for (const hako::asset::Robot& robot : hako_asset_instance.robots) {
        for (const hako::asset::PduWriter& writer : robot.pdu_writers) {
            std::cout << "Robot: " << robot.name << ", PduWriter: " << writer.name << std::endl;
            std::cout << "channel_id: " << writer.channel_id << " pdu_size: " << writer.pdu_size << std::endl;
            bool err = hako_asset_instance.hako_asset->create_pdu_lchannel(
                robot.name,
                writer.channel_id, 
                writer.pdu_size
            );
            if (err == false) {
                std::cerr << "ERROR: Can not create_pdu_channel()" << std::endl;
                return false;
            }            
        }
        for (const hako::asset::PduReader& reader : robot.pdu_readers) {
            std::cout << "Robot: " << robot.name << ", PduReader: " << reader.name << std::endl;
            std::cout << "channel_id: " << reader.channel_id << " pdu_size: " << reader.pdu_size << std::endl;
            bool err = hako_asset_instance.hako_asset->create_pdu_lchannel(
                robot.name,
                reader.channel_id, 
                reader.pdu_size
            );
            if (err == false) {
                std::cerr << "ERROR: Can not create_pdu_channel()" << std::endl;
                return false;
            }
        }
    }
#ifdef ENABLE_HAKO_TIME_MEASURE
    //create buffer
    hako_asset_instance.measure_vp = hako_asset_impl_measure_create_csv(asset_name);
    HAKO_IMPL_ASSERT(hako_asset_instance.measure_vp != nullptr);
#endif /* ENABLE_HAKO_TIME_MEASURE */
    hako_asset_instance.is_initialized = true;
    return true;
}
bool hako::asset::hako_asset_get_pdus(std::vector<hako::asset::Robot> &robots)
{
    if (!hako_asset_instance.is_initialized) {
        std::cerr << "ERROR: hako_asset_instance is not initialized." << std::endl;
        return false;
    }

    robots = hako_asset_instance.robots;
    return true;
}

bool hako_asset_impl_initialize_for_external()
{
    if (hako_asset_instance.is_initialized) {
        return false;
    }
    hako_asset_instance.is_initialized = false;
    hako_asset_instance.external_use = true;
    hako_asset_instance.asset_name_str = "None";
    hako_asset_instance.delta_usec = 0;
    hako_asset_instance.current_usec = 0;

    hako_asset_instance.hako_asset = hako::create_asset_controller();
    if (hako_asset_instance.hako_asset == nullptr) {
        std::cerr << "ERROR: Not found hako-master on this PC" << std::endl;
        return false;
    }
    hako_asset_instance.hako_sim = hako::get_simevent_controller();
    if (hako_asset_instance.hako_sim == nullptr) {
        std::cerr << "ERROR: Not found hako-master on this PC" << std::endl;
        return false;
    }

    hako_asset_instance.is_initialized = true;
    return true;
}
int hako_asset_impl_pdu_create(const char *robo_name, HakoPduChannelIdType lchannel, size_t pdu_size)
{
    if (!hako_asset_instance.is_initialized) {
        return false;
    }
    std::string robotName = robo_name;
    bool err = hako_asset_instance.hako_asset->create_pdu_lchannel(robotName, lchannel, pdu_size);
    if (err == false) {
        std::cerr << "ERROR: Can not create_pdu_channel()" << std::endl;
        return false;
    }    
    return true;
}
bool hako_asset_impl_register_callback(const hako_asset_callbacks_t* callback)
{
    if (hako_asset_instance.external_use) {
        return false;
    }
    hako_asset_instance.callback = callback;
    return true;
}
HakoSimulationStateType hako_asset_impl_state()
{
    return hako_asset_instance.hako_sim->state();
}
static bool hako_asset_impl_wait_state(HakoSimulationStateType target)
{
    do {
        HakoSimulationStateType curr = hako_asset_instance.hako_sim->state();
        if (curr == target) {
            break;
        }
        usleep(HAKO_ASSET_WAIT_TIME_USEC);
    } while (true);
    return true;
}

static bool hako_asset_impl_wait_event(HakoSimulationAssetEventType target)
{
    bool target_event_is_occureed = false;
    while (target_event_is_occureed == false) {
        auto event = hako_asset_instance.hako_asset->asset_get_event(hako_asset_instance.asset_name_str);
        target_event_is_occureed = (target == event);
        switch (event) {
            case HakoSimAssetEvent_None:
                usleep(HAKO_ASSET_WAIT_TIME_USEC);
                break;
            case HakoSimAssetEvent_Start:
                hako_asset_instance.hako_asset->start_feedback(hako_asset_instance.asset_name_str, true);
                break;
            case HakoSimAssetEvent_Stop:
                hako_asset_instance.hako_asset->stop_feedback(hako_asset_instance.asset_name_str, true);
                break;
            case HakoSimAssetEvent_Reset:
                if (hako_asset_instance.callback != NULL) {
                    if (hako_asset_instance.callback->on_reset != NULL) {
                        hako_asset_instance.callback->on_reset(nullptr);
                    }
                }
                hako_asset_instance.current_usec = 0;
                hako_asset_instance.hako_asset->reset_feedback(hako_asset_instance.asset_name_str, true);
                break;
            default:
                std::cerr << "ERROR: hako_asset_impl_wait_event() unkonwn event= " << event << std::endl;
                return false;
        }
    }
    return true;
}

static bool hako_asset_impl_wait_pdu_created()
{
    do {
        bool result = hako_asset_instance.hako_asset->is_pdu_created();
        if (result) {
            break;
        }
        usleep(HAKO_ASSET_WAIT_TIME_USEC);
    } while (true);
    return true;
}

bool hako_asset_impl_wait_running(void)
{
    std::cout << "WAIT START" << std::endl;
    if (hako_asset_impl_wait_event(HakoSimAssetEvent_Start) == false) {
        return false;
    }
    std::cout << "WAIT RUNNING" << std::endl;
    if (hako_asset_impl_wait_state(HakoSim_Running) == false) {
        return false;
    }
    std::cout << "PDU CREATED" << std::endl;
    if (hako_asset_impl_wait_pdu_created() == false) {
        return false;
    }
    if (hako_asset_instance.callback != NULL) {
        if (hako_asset_instance.callback->on_initialize != NULL) {
            hako_asset_instance.callback->on_initialize(nullptr);
        }
    }
    else {
	// nothing to do
    }
    return true;
}
static bool hako_asset_impl_execute(void)
{
    hako_asset_instance.hako_asset->notify_simtime(hako_asset_instance.asset_name_str, hako_asset_instance.current_usec);
    HakoSimulationStateType curr = hako_asset_instance.hako_sim->state();
    if (curr != HakoSim_Running) {
        std::cout << "NOT RUNNING: curr = " << curr << std::endl;
        return false;
    }
    bool result = hako_asset_instance.hako_asset->is_pdu_created();
    if (result == false) {
        std::cout << "PDU IS NOT CREATED" << std::endl;
        return false;
    }
    result = hako_asset_instance.hako_asset->is_pdu_sync_mode(hako_asset_instance.asset_name_str);
    if (result == true) {
        std::cout << "SYNC MODE: true" << std::endl;
	hako_asset_instance.hako_asset->notify_write_pdu_done(hako_asset_instance.asset_name_str);
        return false;
    }
    result = hako_asset_instance.hako_asset->is_simulation_mode();
    if (result == false) {
        //std::cout << "NOT SIMULATION MODE" << std::endl;
        return false;
    }
    hako_time_t world_time = hako_asset_instance.hako_asset->get_worldtime();
    hako_time_t next_time = hako_asset_instance.current_usec + hako_asset_instance.delta_usec;
    if (next_time > world_time) {
        return false;
    }
    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (pro_data) {
        const char* asset_name = hako_asset_instance.asset_name_str.c_str();
        if (hako_asset_instance.external_use) {
            asset_name = nullptr;
        }
        pro_data->call_recv_event_callbacks(asset_name);
    }
    if (hako_asset_instance.callback != NULL) {
        if (hako_asset_instance.callback->on_simulation_step != NULL) {
            hako_asset_instance.callback->on_simulation_step(nullptr);
        }
    }
    hako_asset_instance.current_usec = next_time;
    //std::cout << "# current_usec = " << hako_asset_instance.current_usec << std::endl;
#ifdef ENABLE_HAKO_TIME_MEASURE
    //write csv
    hako_asset_impl_measure_write_csv(hako_asset_instance.measure_vp, world_time, hako_asset_instance.current_usec);
#endif /* ENABLE_HAKO_TIME_MEASURE */
    return true;
}
static void hako_asset_impl_pdus_write_done(void)
{
    for (const hako::asset::Robot& robot : hako_asset_instance.robots) {
        for (const hako::asset::PduWriter& writer : robot.pdu_writers) {
            std::cout << "INFO: hako_asset_impl_pdus_write_done() Robot: " << robot.name << ", PduWriter: " << writer.name << std::endl;
            std::cout << "channel_id: " << writer.channel_id << " pdu_size: " << writer.pdu_size << std::endl;
            bool is_called = false;
            if (is_called == false) {
                char * buffer = (char*) malloc(writer.pdu_size);
                HAKO_ASSET_ASSERT(buffer != NULL);
                memset(buffer, 0, writer.pdu_size);
                auto ret = hako_asset_impl_pdu_write(robot.name.c_str(), writer.channel_id, buffer, writer.pdu_size);
                HAKO_ASSET_ASSERT(ret == true);
                free(buffer);
            }
        }
    }

}
static bool hako_asset_impl_proc(void)
{
    while (hako_asset_impl_execute() == false) {
        HakoSimulationStateType curr = hako_asset_instance.hako_sim->state();
        if (curr != HakoSim_Running) {
#ifdef ENABLE_HAKO_TIME_MEASURE
            //close file
            hako_asset_impl_measure_flush_csv(hako_asset_instance.measure_vp);
#endif /* ENABLE_HAKO_TIME_MEASURE */
            std::cout << "WAIT STOP" << std::endl;
            auto ret = hako_asset_impl_wait_event(HakoSimAssetEvent_Stop);
            HAKO_ASSET_ASSERT(ret == true);
            std::cout << "WAIT RESET" << std::endl;
            ret = hako_asset_impl_wait_event(HakoSimAssetEvent_Reset);
            HAKO_ASSET_ASSERT(ret == true);
            // WAINT FOR RUNNING STATE
            //ret = hako_asset_impl_wait_running();
            //HAKO_ASSET_ASSERT(ret == true);
            return false;
        }
        else if (hako_asset_instance.hako_asset->is_pdu_sync_mode(hako_asset_instance.asset_name_str) == true) {
            hako_asset_impl_pdus_write_done();
        }
    }
    return true;
}

bool hako_asset_impl_step(hako_time_t increment_step)
{
    hako_time_t target_time_usec = hako_asset_instance.current_usec + (increment_step * hako_asset_instance.delta_usec);

    // RUNNING PROC
    //std::cout << "current_usec = " << hako_asset_instance.current_usec << std::endl;
    //std::cout << "target_time_usec = " << target_time_usec << std::endl;
    while (hako_asset_instance.current_usec < target_time_usec) {
        if (hako_asset_impl_proc() == false) {
            return false;
        }
    }
    return true;
}
/***********************
 * pdu io
 ***********************/

bool hako_asset_impl_pdu_read(const char* robo_name, HakoPduChannelIdType lchannel, char* buffer, size_t buffer_len)
{
    if (hako_asset_instance.external_use) {
        return hako_asset_instance.hako_asset->read_pdu_for_external(robo_name, lchannel, buffer, buffer_len);
    }
    bool ret = hako_asset_instance.hako_asset->read_pdu(hako_asset_instance.asset_name_str, robo_name, lchannel, buffer, buffer_len);
    if (ret == false) {
        return false;
    }
    return true;
}

bool hako_asset_impl_pdu_write(const char* robo_name, HakoPduChannelIdType lchannel, const char* buffer, size_t buffer_len)
{
    if (hako_asset_instance.external_use) {
        return hako_asset_instance.hako_asset->write_pdu_for_external(robo_name, lchannel, buffer, buffer_len);
    }
    bool ret = hako_asset_instance.hako_asset->write_pdu(hako_asset_instance.asset_name_str, robo_name, lchannel, buffer, buffer_len);
    if (ret == false) {
        return false;
    }
    hako_asset_instance.hako_asset->notify_write_pdu_done(hako_asset_instance.asset_name_str);
    return true;
}

hako_time_t hako_asset_impl_get_world_time()
{
    return hako_asset_instance.hako_asset->get_worldtime();
}

bool hako_asset_impl_register_data_recv_event(const char *robo_name, HakoPduChannelIdType lchannel, void (*on_recv)(int), int& recv_event_id)
{
    if (!hako_asset_instance.is_initialized) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): asset not initialized" << std::endl;
        return false;
    }

    if (!hako_asset_instance.hako_asset) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): hako_asset controller not available" << std::endl;
        return false;
    }

    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_register_data_recv_event(): pro_data is null" << std::endl;
        return false;
    }
    return pro_data->register_data_recv_event(robo_name, lchannel, on_recv, recv_event_id);
}

bool hako_asset_impl_check_data_recv_event(const char *robo_name, HakoPduChannelIdType lchannel)
{
    if (!hako_asset_instance.is_initialized) {
        std::cerr << "ERROR: hako_asset_impl_check_data_recv_event(): asset not initialized" << std::endl;
        return false;
    }

    if (!hako_asset_instance.hako_asset) {
        std::cerr << "ERROR: hako_asset_impl_check_data_recv_event(): hako_asset controller not available" << std::endl;
        return false;
    }

    auto pro_data = hako::data::pro::hako_pro_get_data();
    if (!pro_data) {
        std::cerr << "ERROR: hako_asset_impl_check_data_recv_event(): pro_data is null" << std::endl;
        return false;
    }
    const char* asset_name = hako_asset_instance.asset_name_str.c_str();
    if (hako_asset_instance.external_use) {
        asset_name = nullptr;
    }
    int recv_event_id;
    return pro_data->get_recv_event(asset_name, robo_name, lchannel, recv_event_id);
}
