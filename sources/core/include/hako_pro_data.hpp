#pragma once
#ifndef HAKO_CORE_EXTENSION
#define HAKO_CORE_EXTENSION
#endif

#include "nlohmann/json.hpp"

#include <hako_extension.hpp>
#include "utils/hako_share/hako_shared_memory_factory.hpp"
#include "data/hako_master_data.hpp"
#include "core/context/hako_context.hpp"
#include "hako_pro_config.hpp"
#include <memory>
namespace hako::data::pro {

/*
 * Hakoniwa Data Receive Event Definitions
 */
typedef enum {
    HAKO_RECV_EVENT_TYPE_FLAG = 0,
    HAKO_RECV_EVENT_TYPE_CALLBACK = 1
} HakoRecvEventType;

#define HAKO_ASSET_ID_EXTERNAL -1

typedef struct {
    bool enabled;
    bool recv_flag;
    pid_type proc_id;
    int real_channel_id;
    HakoRecvEventType type;
    void (*on_recv)(int recv_event_id);
} HakoRecvEventEntryType;

typedef struct {
    HakoRecvEventEntryType entries[HAKO_RECV_EVENT_MAX];
    int entry_num;
} HakoRecvEventTableType;

/*
 * Hakoniwa Service Definitions
 */
#define HAKO_SERVICE_SERVER_CHANNEL_ID 0
#define HAKO_SERVICE_CLIENT_CHANNEL_ID 1
#define HAKO_SERVICE_SERVER_CHANNEL_ID_MAX 2
typedef struct {
    bool enabled;
    int namelen;
    char clientName[HAKO_CLIENT_NAMELEN_MAX];
    int requestChannelId;
    int requestRecvEventId;
    int responseChannelId;
    int responseRecvEventId;
} HakoServiceClientChannelMapType;

typedef struct {
    bool enabled;
    int namelen;
    char serviceName[HAKO_SERVICE_NAMELEN_MAX];
    int maxClients;
    HakoServiceClientChannelMapType clientChannelMap[HAKO_SERVICE_CLIENT_MAX];
} HakoServiceEntryTye;

typedef struct {
    int entry_num;
    HakoServiceEntryTye entries[HAKO_SERVICE_MAX];
} HakoServiceTableType;

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
        std::shared_ptr<hako::utils::HakoSharedMemory> get_shared_memory()
        {
            return shmp_;
        }
        void set_recv_event_table(HakoRecvEventTableType* table)
        {
            recv_event_table_ = table;
        }        
        HakoRecvEventTableType* get_recv_event_table()
        {
            return recv_event_table_;
        }
        void set_service_table(HakoServiceTableType* table)
        {
            service_table_ = table;
        }
        bool is_exist_service(const std::string& service_name)
        {
            for (int i = 0; i < HAKO_SERVICE_MAX; i++) {
                if (service_table_->entries[i].enabled == false) {
                    continue;
                }
                if (strcmp(service_table_->entries[i].serviceName, service_name.c_str()) == 0) {
                    return true;
                }
            }
            return false;
        }
        bool is_exist_client_on_service(const std::string& service_name, const std::string& client_name)
        {
            for (int i = 0; i < HAKO_SERVICE_MAX; i++) {
                if (service_table_->entries[i].enabled == false) {
                    continue;
                }
                if (strcmp(service_table_->entries[i].serviceName, service_name.c_str()) != 0) {
                    continue;
                }
                for (int j = 0; j < service_table_->entries[i].maxClients; j++) {
                    if (service_table_->entries[i].clientChannelMap[j].enabled == false) {
                        continue;
                    }
                    if (strcmp(service_table_->entries[i].clientChannelMap[j].clientName, client_name.c_str()) == 0) {
                        return true;
                    }
                }
            }
            return false;
        }
        HakoServiceEntryTye& get_service_entry(const std::string& service_name)
        {
            for (int i = 0; i < HAKO_SERVICE_MAX; i++) {
                if (service_table_->entries[i].enabled == false) {
                    continue;
                }
                if (strcmp(service_table_->entries[i].serviceName, service_name.c_str()) == 0) {
                    return service_table_->entries[i];
                }
            }
            throw std::runtime_error("Service not found");
        }
        int get_service_id(const std::string& service_name)
        {
            for (int i = 0; i < HAKO_SERVICE_MAX; i++) {
                if (service_table_->entries[i].enabled == false) {
                    continue;
                }
                if (strcmp(service_table_->entries[i].serviceName, service_name.c_str()) == 0) {
                    return i;
                }
            }
            return -1;
        }
        // lock memory is user's responsibility
        // this function is only for creating service
        int create_service(const std::string& serviceName);
        int get_request(int asset_id, int service_id, int client_id, char* packet, size_t packet_len);
        int put_response(int asset_id, int service_id, int client_id, char* packet, size_t packet_len);

        int create_service_client(const std::string& serviceName, const std::string& clientName, int& client_id);
        int get_response(int asset_id, int service_id, int client_id, char* packet, size_t packet_len);
        int put_request(int asset_id, int service_id, int client_id, char* packet, size_t packet_len);
        HakoServiceTableType* get_service_table()
        {
            return service_table_;
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
        bool register_data_recv_event(const std::string& robot_name, int channel_id, void (*on_recv)(int), int& recv_event_id)
        {
            recv_event_id = -1;
            if (recv_event_table_ == nullptr) {
                std::cout << "ERROR: recv_event_table_ is null" << std::endl;
                return false;
            }
            HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(robot_name, channel_id);
            if (real_id < 0) {
                std::cout << "ERROR: real_id is invalid: robot_name: " << robot_name << " channel_id: " << channel_id << std::endl;
                return false;
            }
        
            if (recv_event_table_->entry_num >= HAKO_RECV_EVENT_MAX) {
                std::cout << "ERROR: recv_event_table_ is full" << std::endl;
                return false;
            }
            bool ret = false;
            this->shmp_->lock_memory(HAKO_SHARED_MEMORY_ID_2);
            for (int i = 0; i < HAKO_RECV_EVENT_MAX; i++) {
                if (recv_event_table_->entries[i].enabled == false) {
                    hako::core::context::HakoContext context;
                    recv_event_table_->entries[i].enabled = true;
                    recv_event_table_->entries[i].recv_flag = false;
                    recv_event_table_->entries[i].proc_id = context.get_pid();
                    recv_event_table_->entries[i].real_channel_id = real_id;
                    if (on_recv != nullptr) {
                        recv_event_table_->entries[i].type = HAKO_RECV_EVENT_TYPE_CALLBACK;
                        recv_event_table_->entries[i].on_recv = on_recv;
                    }
                    else {
                        recv_event_table_->entries[i].type = HAKO_RECV_EVENT_TYPE_FLAG;
                        recv_event_table_->entries[i].on_recv = nullptr;
                    }
                    recv_event_id = i;
                    recv_event_table_->entry_num++;
                    ret = true;
                    break;
                }
            }
            this->shmp_->unlock_memory(HAKO_SHARED_MEMORY_ID_2);
            std::cout << "INFO: register_data_recv_event() robot_name: " << robot_name << " channel_id: " << channel_id << " ret: " << ret << std::endl;
            return ret;
        }
        bool get_recv_event(const char* asset_name, const std::string& robot_name, int channel_id, int& recv_event_id)
        {
            int asset_id = -1;
            if (asset_name != nullptr)
            {
                auto* asset = this->master_data_->get_asset_nolock(asset_name);
                if (asset == nullptr) {
                    return false;
                }
                asset_id = asset->id;
            }
            HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(robot_name, channel_id);
            if (real_id < 0) {
                return false;
            }
            return get_recv_event(asset_id, real_id, recv_event_id);
        }
        bool get_recv_event(int asset_id, const std::string& robot_name, int channel_id, int& recv_event_id)
        {
            HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(robot_name, channel_id);
            if (real_id < 0) {
                return false;
            }
            return get_recv_event(asset_id, real_id, recv_event_id);
        }
        /*
         * if asset_id < 0 then it is external 
         * channel_id must be real channel id!
         */
        bool get_recv_event(int asset_id, int channel_id, int& recv_event_id)
        {
            recv_event_id = -1;
            if (recv_event_table_ == nullptr) {
                return false;
            }
            if (channel_id >= HAKO_PDU_CHANNEL_MAX) {
                return false;
            }
            if (recv_event_table_->entry_num == 0) {
                return false;
            }
            hako::core::context::HakoContext context;
            auto pid = context.get_pid();
            for (int i = 0; i < recv_event_table_->entry_num; i++) {
                if (recv_event_table_->entries[i].enabled == false) {
                    continue;
                }
                if (recv_event_table_->entries[i].proc_id != pid) {
                    continue;
                }
                if (recv_event_table_->entries[i].real_channel_id != channel_id) {
                    continue;
                }
                this->master_data_->get_pdu_data()->read_pdu_spin_lock(asset_id, channel_id);
                bool recv_flag = recv_event_table_->entries[i].recv_flag;
                //std::cout << "INFO: get_recv_event() recv_flag: " << recv_flag << std::endl;
                if (recv_flag) {
                    recv_event_table_->entries[i].recv_flag = false;
                    recv_event_id = i;
                }
                this->master_data_->get_pdu_data()->read_pdu_spin_unlock(asset_id, channel_id);
                //std::cout << "INFO: get_recv_event() asset_id: " << asset_id << " channel_id: " << channel_id << " recv_event_id: " << recv_event_id << std::endl;
                return recv_flag;
            }
            return false;
        }
        bool call_recv_event_callbacks(const char* asset_name)
        {
            //std::cout << "INFO: call_recv_event_callbacks() asset_name: " << asset_name << std::endl;
            if (recv_event_table_ == nullptr) {
                //std::cout << "ERROR: recv_event_table_ is null" << std::endl;
                return false;
            }
            if (recv_event_table_->entry_num == 0) {
                //std::cout << "ERROR: recv_event_table_ is empty" << std::endl;
                return false;
            }
            int asset_id = -1;
            if (asset_name != nullptr)
            {
                auto* asset = this->master_data_->get_asset_nolock(asset_name);
                if (asset == nullptr) {
                    //std::cout << "ERROR: asset is null" << std::endl;
                    return false;
                }
                asset_id = asset->id;
            }
            hako::core::context::HakoContext context;
            auto pid = context.get_pid();
            for (int i = 0; i < recv_event_table_->entry_num; i++) {
                if (recv_event_table_->entries[i].enabled == false) {
                    continue;
                }
                if (recv_event_table_->entries[i].proc_id != pid) {
                    continue;
                }
                if (recv_event_table_->entries[i].on_recv == nullptr) {
                    continue;
                }
                this->master_data_->get_pdu_data()->read_pdu_spin_lock(asset_id, recv_event_table_->entries[i].real_channel_id);
                bool recv_flag = recv_event_table_->entries[i].recv_flag;
                if (recv_flag) {
                    recv_event_table_->entries[i].recv_flag = false;
                    //std::cout << "INFO: call_recv_event_callbacks() recv_flag: " << recv_flag << std::endl;
                }
                this->master_data_->get_pdu_data()->read_pdu_spin_unlock(asset_id, recv_event_table_->entries[i].real_channel_id);
                if (recv_flag && (recv_event_table_->entries[i].on_recv != nullptr)) {
                    recv_event_table_->entries[i].on_recv(i);
                }
            }
            //std::cout << "INFO: call_recv_event_callbacks() end" << std::endl;
            return true;
        }
        bool call_recv_event_callback(int recv_event_id)
        {
            if (recv_event_table_ == nullptr) {
                return false;
            }
            if (recv_event_id < 0 || recv_event_id >= HAKO_RECV_EVENT_MAX) {
                return false;
            }
            if (recv_event_table_->entries[recv_event_id].enabled == false) {
                return false;
            }
            if (recv_event_table_->entries[recv_event_id].type != HAKO_RECV_EVENT_TYPE_CALLBACK) {
                return false;
            }
            if (recv_event_table_->entries[recv_event_id].on_recv == nullptr) {
                return false;
            }
            recv_event_table_->entries[recv_event_id].on_recv(recv_event_id);
            return true;
        }
        std::shared_ptr<hako::extension::IHakoAssetExtension> get_asset_extension()
        {
            return std::static_pointer_cast<hako::extension::IHakoAssetExtension>(asset_extension_);
        }
        bool initialize_service(const std::string& service_config_path);
        void set_service_data();
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