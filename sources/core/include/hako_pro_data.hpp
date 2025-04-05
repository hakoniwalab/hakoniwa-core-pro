#pragma once
#ifndef HAKO_CORE_EXTENSION
#define HAKO_CORE_EXTENSION
#endif

#include <hako_extension.hpp>
#include "utils/hako_share/hako_shared_memory_factory.hpp"
#include "data/hako_master_data.hpp"
#include "core/context/hako_context.hpp"
#include <memory>
namespace hako::data::pro {

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
    void (*on_recv)();
} HakoRecvEventEntryType;

#define HAKO_RECV_EVENT_MAX 32
typedef struct {
    HakoRecvEventEntryType entries[HAKO_RECV_EVENT_MAX];
    int entry_num;
} HakoRecvEventTableType;

class HakoProAssetExtension;
class HakoProData : public std::enable_shared_from_this<HakoProData>, public hako::extension::IHakoMasterExtension {
    public:
        HakoProData(std::shared_ptr<data::HakoMasterData> master_data)
        {
            this->master_data_ = master_data;
            this->shmp_ = nullptr;
            this->recv_event_table_ = nullptr;
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
        virtual bool on_pdu_data_create() override;
        virtual bool on_pdu_data_load() override;
        virtual bool on_pdu_data_reset() override;
        virtual bool on_pdu_data_destroy() override;
    
        bool register_data_recv_event(const std::string& robot_name, int channel_id, void (*on_recv)())
        {
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
                if (recv_flag) {
                    recv_event_table_->entries[i].recv_flag = false;
                }
                this->master_data_->get_pdu_data()->read_pdu_spin_unlock(asset_id, channel_id);
                recv_event_id = i;
                return true;
            }
            return false;
        }
        bool call_recv_event_callbacks(const char* asset_name)
        {
            if (recv_event_table_ == nullptr) {
                return false;
            }
            if (recv_event_table_->entry_num == 0) {
                return false;
            }
            int asset_id = -1;
            if (asset_name != nullptr)
            {
                auto* asset = this->master_data_->get_asset_nolock(asset_name);
                if (asset == nullptr) {
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
                this->master_data_->get_pdu_data()->read_pdu_spin_lock(asset_id, recv_event_table_->entries[i].real_channel_id);
                bool recv_flag = recv_event_table_->entries[i].recv_flag;
                if (recv_flag) {
                    recv_event_table_->entries[i].recv_flag = false;
                }
                this->master_data_->get_pdu_data()->read_pdu_spin_unlock(asset_id, recv_event_table_->entries[i].real_channel_id);
                if (recv_event_table_->entries[i].on_recv != nullptr) {
                    recv_event_table_->entries[i].on_recv();
                }
            }
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
            recv_event_table_->entries[recv_event_id].on_recv();
            return true;
        }
        std::shared_ptr<hako::extension::IHakoAssetExtension> get_asset_extension()
        {
            return std::static_pointer_cast<hako::extension::IHakoAssetExtension>(asset_extension_);
        }
    private:
        HakoRecvEventTableType* recv_event_table_;
        std::shared_ptr<HakoProAssetExtension> asset_extension_;
        std::shared_ptr<hako::utils::HakoSharedMemory> shmp_;
        std::string shm_type_;
        std::shared_ptr<data::HakoMasterData> master_data_;
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
private:
    std::shared_ptr<HakoProData> pro_;
};

}