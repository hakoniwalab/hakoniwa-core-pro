#include "hako_asset_impl.hpp"

struct hako_asset_context_s {
    void *data;
};

static inline bool file_exists(const char* path) {
    std::ifstream file(path);
    return file.good();
}

int hako_asset_register(const char *asset_name, const char *config_path, hako_asset_callbacks_t *callbacks, hako_time_t delta_usec, HakoAssetModelType model) {
    std::cout << "INFO: hako_asset_register :" << asset_name << std::endl;
    if (asset_name == nullptr || *asset_name == '\0') {
        std::cerr << "Error: Asset name is not set." << std::endl;
        return EINVAL;
    }
    if (config_path == nullptr || !file_exists(config_path)) {
        std::cerr << "Error: Config file does not exist." << std::endl;
        return ENOENT;
    }
    if (callbacks == nullptr) {
        std::cerr << "Error: callbacks is not set." << std::endl;
        return EINVAL;
    }
    if ((callbacks->on_simulation_step != nullptr) && (callbacks->on_manual_timing_control != nullptr)) {
        std::cerr << "Error: on_simulation_step and on_manual_timing_control are set." << std::endl;
        return EINVAL;
    }
    if (delta_usec < HAKO_ASSET_MIN_DELTA_TIME_USEC) {
        std::cerr << "Error: delta_usec(" << delta_usec << ") is small: MIN=" << HAKO_ASSET_MIN_DELTA_TIME_USEC << std::endl;
        return EINVAL;
    }
    hako_asset_impl_register_callback(callbacks);
    if (!hako_asset_impl_init(asset_name, config_path, delta_usec, (model == HAKO_ASSET_MODEL_PLANT))) {
        return EIO;
    }
    std::cout << "INFO: asset(" << asset_name << ") is registered." << std::endl;
    return 0;
}
int hako_initialize_for_external(void)
{
    if (!hako_asset_impl_initialize_for_external()) {
        return EIO;
    }
    std::cout << "INFO: Success for external initialization." << std::endl;
    return 0;
}
int hako_asset_pdu_create(const char *robo_name, HakoPduChannelIdType lchannel, size_t pdu_size)
{
    return hako_asset_impl_pdu_create(robo_name, lchannel, pdu_size);
}
int hako_asset_start(void) {
    if (hako_asset_instance.is_initialized == false) {
        std::cerr << "Error: not initialized." << std::endl;
        return EINVAL;
    }
    if (hako_asset_impl_state() != HakoSim_Stopped) {
        std::cerr << "Error: simulation state(" << hako_asset_impl_state() << ") is invalid, expeting HakoSim_Stopeed." << std::endl;
        return EINVAL;
    }
    bool ret = hako_asset_impl_wait_running();
    if (ret == false) {
        std::cerr << "Error: can not wait running for start." << std::endl;
        return EIO;
    }
    std::cout << "INFO: start simulation" << std::endl;
    if (hako_asset_instance.callback->on_manual_timing_control != nullptr) {
        return hako_asset_instance.callback->on_manual_timing_control(nullptr);
    }
    //on_simulation_step
    while (true) {
        if (hako_asset_impl_step(1) == false) {
            std::cout << "INFO: stopped simulation" << std::endl;
            return EINTR;
        }
    }
    return 0;
}

int hako_asset_pdu_read(const char *robo_name, HakoPduChannelIdType lchannel, char *buffer, size_t buffer_len) {
    if (hako_asset_instance.is_initialized == false) {
        std::cerr << "Error: not initialized." << std::endl;
        return EINVAL;
    }
    if (robo_name == nullptr || *robo_name == '\0') {
        std::cerr << "Error: robo_name is not set." << std::endl;
        return EINVAL;
    }
    if (buffer == nullptr || buffer_len == 0) {
        std::cerr << "Error: Invalid buffer or buffer_len." << std::endl;
        return EINVAL;
    }

    bool result = hako_asset_impl_pdu_read(robo_name, lchannel, buffer, buffer_len);
    if (!result) {
        std::cerr << "Error: Failed to read PDU data." << std::endl;
        return EIO;
    }
    return 0;
}

int hako_asset_pdu_write(const char *robo_name, HakoPduChannelIdType lchannel, const char *buffer, size_t buffer_len) {
    if (hako_asset_instance.is_initialized == false) {
        std::cerr << "Error: not initialized." << std::endl;
        return EINVAL;
    }
    if (robo_name == nullptr || *robo_name == '\0') {
        std::cerr << "Error: robo_name is not set." << std::endl;
        return EINVAL;
    }
    if (buffer == nullptr || buffer_len == 0) {
        std::cerr << "Error: Invalid buffer or buffer_len." << std::endl;
        return EINVAL;
    }

    bool result = hako_asset_impl_pdu_write(robo_name, lchannel, buffer, buffer_len);
    if (!result) {
        std::cerr << "Error: Failed to write PDU data." << std::endl;
        return EIO;
    }
    return 0;
}

hako_time_t hako_asset_simulation_time(void) {
    return hako_asset_impl_get_world_time();
}

int hako_asset_usleep(hako_time_t sleep_time_usec) {
    hako_time_t step;
    if (sleep_time_usec == 0) {
        step = 1;
    }
    else {
        step = (sleep_time_usec + (hako_asset_instance.delta_usec - 1)) / hako_asset_instance.delta_usec;
    }
    if (hako_asset_impl_step(step)) {
        return 0;
    }
    return EINTR;
}

int hako_asset_register_data_recv_event(const char *robo_name, HakoPduChannelIdType lchannel, void (*on_recv)())
{
    if (hako_asset_instance.is_initialized == false) {
        std::cerr << "Error: not initialized." << std::endl;
        return EINVAL;
    }
    if (robo_name == nullptr || *robo_name == '\0') {
        std::cerr << "Error: robo_name is not set." << std::endl;
        return EINVAL;
    }
    bool result = hako_asset_impl_register_data_recv_event(robo_name, lchannel, on_recv);
    if (!result) {
        std::cerr << "Error: Failed to register data receive event." << std::endl;
        return EINVAL;
    }
    return 0;
}
int hako_asset_check_data_recv_event(const char *robo_name, HakoPduChannelIdType lchannel)
{
    if (hako_asset_instance.is_initialized == false) {
        std::cerr << "Error: not initialized." << std::endl;
        return EINVAL;
    }
    if (robo_name == nullptr || *robo_name == '\0') {
        std::cerr << "Error: robo_name is not set." << std::endl;
        return EINVAL;
    }
    bool result = hako_asset_impl_check_data_recv_event(robo_name, lchannel);
    if (!result) {
        return ENOENT;
    }
    return 0;
}