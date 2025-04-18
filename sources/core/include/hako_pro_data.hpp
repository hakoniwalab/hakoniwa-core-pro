#pragma once
#ifndef HAKO_CORE_EXTENSION
#define HAKO_CORE_EXTENSION
#endif

#include "nlohmann/json.hpp"

#include <hako_extension.hpp>
#include "hako_pro_data_types.hpp"
#include "utils/hako_share/hako_shared_memory_factory.hpp"

namespace hako::data::pro {

struct Service {
    std::string name;
    std::string type;
    int maxClients;

    int pdu_size_server_base;
    int pdu_size_client_base;
    int pdu_size_server_heap;
    int pdu_size_client_heap;

    size_t server_total_size;
    size_t client_total_size;
};
struct HakoServiceImplType {
    nlohmann::json param;
    std::vector<Service> services;
};

#define TOTAL_HAKO_PRO_DATA_SIZE (sizeof(HakoRecvEventTableType) + sizeof(HakoServiceTableType) + 1024)
#define OFFSET_HAKO_RECV_EVENT_TABLE (0)
#define OFFSET_HAKO_SERVICE_TABLE (sizeof(HakoRecvEventTableType))
class HakoProAssetExtension;
class HakoProData : public std::enable_shared_from_this<HakoProData>, public hako::extension::IHakoMasterExtension {
    public:
        HakoProData(std::shared_ptr<data::HakoMasterData> master_data)
        {
            this->master_data_ = master_data;
            this->shmp_ = nullptr;
            this->recv_event_table_ = nullptr;
            this->service_table_ = nullptr;
            this->shm_type_ = "shm";
        }
        void init(const std::string& type)
        {
            this->shm_type_ = type;
            std::cout <<"INFO: HakoProData::init() : type: " << this->shm_type_ << std::endl;
            this->shmp_ = hako::utils::hako_shared_memory_create(this->shm_type_);
            if (this->shmp_ == nullptr) {
                throw std::runtime_error("Failed to create shared memory");
            }
            asset_extension_ = std::make_shared<HakoProAssetExtension>(shared_from_this());
            master_data_->register_master_extension(shared_from_this());
        }
        void destroy()
        {
            if (this->shmp_ != nullptr) {
                this->shmp_->destroy_memory(HAKO_SHARED_MEMORY_ID_1);
                this->shmp_ = nullptr;
                this->recv_event_table_ = nullptr;
                this->service_table_ = nullptr;
            }
        }
        virtual ~HakoProData()
        {
        }
        HakoTimeType get_world_time_usec()
        {
            return this->master_data_->ref_time_nolock().current;
        }

        std::shared_ptr<hako::utils::HakoSharedMemory> get_shared_memory()
        {
            return shmp_;
        }
        virtual bool on_pdu_data_create() override;
        virtual bool on_pdu_data_load() override;
        virtual bool on_pdu_data_reset() override;
        virtual bool on_pdu_data_destroy() override;
    
        void lock_memory()
        {
            this->shmp_->lock_memory(HAKO_SHARED_MEMORY_ID_2);
        }
        void unlock_memory()
        {
            this->shmp_->unlock_memory(HAKO_SHARED_MEMORY_ID_2);
        }
        std::shared_ptr<hako::extension::IHakoAssetExtension> get_asset_extension()
        {
            return std::static_pointer_cast<hako::extension::IHakoAssetExtension>(asset_extension_);
        }
        int get_asset_id(const std::string& asset_name)
        {
            auto* asset = this->master_data_->get_asset_nolock(asset_name);
            if (asset == nullptr) {
                return -1;
            }
            return asset->id;
        }
        /*
         * Receive event API
         */
        void set_recv_event_table(HakoRecvEventTableType* table)
        {
            recv_event_table_ = table;
        }        
        HakoRecvEventTableType* get_recv_event_table()
        {
            return recv_event_table_;
        }

        bool register_data_recv_event(const std::string& robot_name, int channel_id, void (*on_recv)(int), int& recv_event_id);
        bool get_recv_event(const char* asset_name, const std::string& robot_name, int channel_id, int& recv_event_id);
        bool get_recv_event(int asset_id, const std::string& robot_name, int channel_id, int& recv_event_id);
        /*
         * if asset_id < 0 then it is external 
         * channel_id must be real channel id!
         */
        bool get_recv_event(int asset_id, int channel_id, int& recv_event_id);
        bool call_recv_event_callbacks(const char* asset_name);
        bool call_recv_event_callback(int recv_event_id);

        /*
         * Service API
         */
        bool initialize_service(const std::string& service_config_path);
        void set_service_data();
        void set_service_table(HakoServiceTableType* table)
        {
            service_table_ = table;
        }
        HakoServiceTableType* get_service_table()
        {
            return service_table_;
        }

        bool is_exist_service(const std::string& service_name);
        bool is_exist_client_on_service(const std::string& service_name, const std::string& client_name);
        HakoServiceEntryTye& get_service_entry(const std::string& service_name);
        int get_service_id(const std::string& service_name);
        // lock memory is user's responsibility
        // this function is only for creating service
        int create_service(const std::string& serviceName);
        int get_request(int asset_id, int service_id, int client_id, char* packet, size_t packet_len);
        int put_response(int asset_id, int service_id, int client_id, char* packet, size_t packet_len);

        int create_service_client(const std::string& serviceName, const std::string& clientName, int& client_id);
        int get_response(int asset_id, int service_id, int client_id, char* packet, size_t packet_len);
        int put_request(int asset_id, int service_id, int client_id, char* packet, size_t packet_len);

    private:
        HakoRecvEventTableType* recv_event_table_;
        HakoServiceTableType* service_table_;
        std::shared_ptr<HakoProAssetExtension> asset_extension_;
        std::shared_ptr<hako::utils::HakoSharedMemory> shmp_;
        std::string shm_type_;
        std::shared_ptr<data::HakoMasterData> master_data_;
        HakoServiceImplType service_impl_;
};
    
class HakoProAssetExtension: public hako::extension::IHakoAssetExtension {
public:
    HakoProAssetExtension(std::shared_ptr<HakoProData> ptr)
    {
        pro_ = ptr;
    }
    virtual ~HakoProAssetExtension()
    {
    }
    virtual bool on_pdu_data_write(int real_channel_id) override;
    virtual bool on_pdu_data_before_write(int real_channel_id) override;
private:
    std::shared_ptr<HakoProData> pro_;
};

}