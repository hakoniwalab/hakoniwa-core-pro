#pragma once

#include "data/hako_master_data.hpp"
#include "utils/hako_share/hako_shared_memory.hpp"
#include <vector>
#include <string>
namespace hako::command {

    class HakoMetaDumper {
        public:
            HakoMetaDumper() = default;
            ~HakoMetaDumper() = default;

            void parse();
            std::string dump_json();
        private:
            hako::utils::SharedMemoryMetaDataType *shm_ptr_ = nullptr;
            hako::data::HakoMasterDataType *master_data_ptr_ = nullptr;
            std::vector<uint8_t> buffer_;


    };
}
