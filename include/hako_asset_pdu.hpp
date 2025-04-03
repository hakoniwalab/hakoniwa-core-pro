#ifndef _HAKO_ASSET_PDU_HPP_
#define _HAKO_ASSET_PDU_HPP_

#include <string>
#include <vector>

namespace hako {
namespace asset {

struct PduReader {
    std::string type;
    std::string org_name;
    std::string name;
    int channel_id;
    int pdu_size;
    int write_cycle;
};

struct PduWriter {
    std::string type;
    std::string org_name;
    std::string name;
    int write_cycle;
    int channel_id;
    int pdu_size;
    std::string method_type;
};
struct Robot {
    std::string name;
    std::vector<PduReader> pdu_readers;
    std::vector<PduWriter> pdu_writers;
};
extern bool hako_asset_get_pdus(std::vector<Robot> &robots);

}
}

#endif /* _HAKO_ASSET_PDU_HPP_ */