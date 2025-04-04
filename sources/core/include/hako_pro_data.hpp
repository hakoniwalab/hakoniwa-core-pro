#pragma once

#include <hako_extension.hpp>
#include "utils/hako_share/hako_shared_memory_factory.hpp"
#include <memory>

namespace hako::data::pro {

typedef enum {
    HAKO_RECV_EVENT_TYPE_FLAG = 0,
    HAKO_RECV_EVENT_TYPE_CALLBACK = 1
} HakoRecvEventType;

#define HAKO_ASSET_ID_EXTERNAL -1

typedef struct {
    bool recv_flag;
    int proc_id;
    int asset_id;
    int real_channel_id;
    HakoRecvEventType type;
    void (*on_recv)();
} HakoRecvEventEntryType;

#define HAKO_RECV_EVENT_MAX 32
typedef struct {
    HakoRecvEventEntryType entries[HAKO_RECV_EVENT_MAX];
    int entry_num;
} HakoRecvEventTableType;

class HakoProMasterExtension;
class HakoProAssetExtension;
class HakoProData : public std::enable_shared_from_this<HakoProData> {
    public:
        HakoProData()
        {
        }
        void init(const std::string& type="shm")
        {
            this->shm_type_ = type;
            this->shmp_ = hako::utils::hako_shared_memory_create(this->shm_type_);
            if (this->shmp_ == nullptr) {
                throw std::runtime_error("Failed to create shared memory");
            }
            master_extension_ = std::make_shared<HakoProMasterExtension>(shared_from_this());
            asset_extension_ = std::make_shared<HakoProAssetExtension>(shared_from_this());
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
        private:
        HakoRecvEventTableType* recv_event_table_;
        std::shared_ptr<HakoProMasterExtension> master_extension_;
        std::shared_ptr<HakoProAssetExtension> asset_extension_;
        std::shared_ptr<hako::utils::HakoSharedMemory> shmp_;
        std::string shm_type_;
    };
    
class HakoProMasterExtension: public hako::extension::IHakoMasterExtension {
public:
    HakoProMasterExtension(std::shared_ptr<HakoProData> ptr)
    {
        pro_ = ptr;
    }
    virtual ~HakoProMasterExtension()
    {
    }
    virtual bool on_pdu_data_create() override;
    virtual bool on_pdu_data_load() override;
    virtual bool on_pdu_data_reset() override;
    virtual bool on_pdu_data_destroy() override;
    
private:
    std::shared_ptr<HakoProData> pro_;
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
    virtual bool on_pdu_data_write(int asset_id, int real_channel_id) override;
    virtual bool on_pdu_data_write_for_external(int real_channel_id) override;
private:
    std::shared_ptr<HakoProData> pro_;
};

}