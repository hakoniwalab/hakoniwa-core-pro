#include "hako_service_impl.hpp"
#include "hako_impl.hpp"
#include <fstream>
#include <iostream>

hako::service::impl::HakoServiceImplType hako_service_instance;

int hako::service::impl::initialize(const char* service_config_path, std::shared_ptr<hako::IHakoAssetController> hako_asset)
{
    hako_service_instance.is_initialized = false;
    std::ifstream ifs(service_config_path);
    
    if (!ifs.is_open()) {
        std::cerr << "Error: Failed to open service config file: " << service_config_path << std::endl;
        return -1;
    }

    try {
        ifs >> hako_service_instance.param;
        int pduMetaDataSize = hako_service_instance.param["pduMetaDataSize"];
        for (const auto& item : hako_service_instance.param["services"]) {
            hako::service::impl::Service s;
            s.name = item["name"];
            s.type = item["type"];
            s.maxClients = item["maxClients"];

            const auto& pduSize = item["pduSize"];
            s.pdu_size_server_base = pduSize["pduSize"]["server"]["baseSize"];
            s.pdu_size_client_base = pduSize["pduSize"]["client"]["baseSize"];
            s.pdu_size_server_heap = pduSize["pduSize"]["server"]["heapSize"];
            s.pdu_size_client_heap = pduSize["pduSize"]["client"]["heapSize"];

            //create pdu channels for service
            s.serverPduSize = pduMetaDataSize + s.pdu_size_server_base + s.pdu_size_server_heap;
            s.clientPduSize = pduMetaDataSize + s.pdu_size_client_base + s.pdu_size_client_heap;
            bool ret = hako_asset->create_pdu_lchannel(s.name.c_str(), 0, (size_t)s.serverPduSize);
            if (ret == false) {
                std::cerr << "Error: Failed to create PDU channel for service server: " << s.name << std::endl;
                return -1;
            }
            ret = hako_asset->create_pdu_lchannel(s.name.c_str(), 1, (size_t)s.clientPduSize);
            if (ret == false) {
                std::cerr << "Error: Failed to create PDU channel for service client: " << s.name << std::endl;
                return -1;
            }
            hako_service_instance.services.push_back(s);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: Failed to parse service config JSON: " << e.what() << std::endl;
        return -1;
    }
    ifs.close();
    hako_service_instance.is_initialized = true;
    return 0;
}

bool hako::service::impl::get_services(std::vector<hako::service::impl::Service>& services)
{
    if (!hako_service_instance.is_initialized) {
        return false;
    }
    services = hako_service_instance.services;
    return true;
}
