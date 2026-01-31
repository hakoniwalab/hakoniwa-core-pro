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
struct PduIo {
    std::string type;
    int channel_id;
    int pdu_size;
};
struct PduIoEntry {
    PduIo io;
    std::string name; // org_name (robot名は含まない)
};
struct Robot {
    std::string name;
    std::vector<PduReader> pdu_readers;
    std::vector<PduWriter> pdu_writers;
};
struct RobotCompact {
    std::string name;
    std::vector<PduIoEntry> pdus;
};
extern bool hako_asset_get_pdus(std::vector<Robot> &robots);
extern bool hako_asset_get_pdus_compact(std::vector<RobotCompact> &robots);

}
}

#endif /* _HAKO_ASSET_PDU_HPP_ */
