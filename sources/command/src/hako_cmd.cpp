#include <hako.hpp>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include "windows.h"
#else
#include <unistd.h>
#endif
#include <signal.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include "dump_meta.hpp"
#include "cxxopts.hpp"

// Type definitions
using HakoSimCtrlPtr = std::shared_ptr<hako::IHakoSimulationEventController>;
using Args = const std::vector<std::string>&;

// Signal handler
static void hako_cmd_signal_handler(int sig) {
    printf("SIGNAL RECV: %d\n", sig);
}

// File I/O
#ifdef WIN32
static int read_file_data(const std::string& filepath, size_t size, char* buffer) {
    HANDLE hFile = CreateFile(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("ERROR: cannot open file %s\n", filepath.c_str());
        return -1;
    }
    DWORD bytesRead;
    BOOL bResult = ReadFile(hFile, buffer, size, &bytesRead, NULL);
    if (!bResult || bytesRead == 0) {
        printf("ERROR: cannot read file %s\n", filepath.c_str());
        CloseHandle(hFile);
        return -1;
    }
    CloseHandle(hFile);
    return 0;
}
#else
static int read_file_data(const std::string& filepath, size_t size, char* buffer) {
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd < 0) {
        printf("ERROR: can not open file %s errno=%d\n", filepath.c_str(), errno);
        return -1;
    }
    ssize_t ret = read(fd, buffer, size);
    if (ret < 0) {
        printf("ERROR: can not read file %s errno=%d\n", filepath.c_str(), errno);
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}
#endif

// Command handlers
int do_start(HakoSimCtrlPtr& hako_sim_ctrl, Args) {
    printf("start\n");
    hako_sim_ctrl->start();
    return 0;
}

int do_stop(HakoSimCtrlPtr& hako_sim_ctrl, Args) {
    printf("stop\n");
    hako_sim_ctrl->stop();
    return 0;
}

int do_reset(HakoSimCtrlPtr& hako_sim_ctrl, Args) {
    printf("reset\n");
    hako_sim_ctrl->reset();
    return 0;
}

int do_status(HakoSimCtrlPtr& hako_sim_ctrl, Args) {
    const std::vector<std::string> hako_status = {
        "stopped", "runnable", "running", "stopping", "resetting", "error", "terminated"
    };
    printf("status=%s\n", hako_status[hako_sim_ctrl->state()].c_str());
    return 0;
}

int do_ls(HakoSimCtrlPtr& hako_sim_ctrl, Args) {
    std::vector<std::shared_ptr<std::string>> asset_list;
    hako_sim_ctrl->assets(asset_list);
    for(const auto& name : asset_list) {
        std::cout << *name << std::endl;
    }
    return 0;
}

int do_pmeta(HakoSimCtrlPtr& hako_sim_ctrl, Args) {
    hako_sim_ctrl->print_master_data();
    return 0;
}

int do_plog(HakoSimCtrlPtr& hako_sim_ctrl, Args) {
    hako_sim_ctrl->print_memory_log();
    return 0;
}

int do_jmeta(HakoSimCtrlPtr&, Args) {
    hako::command::HakoMetaDumper dumper;
    dumper.parse();
    std::string json_data = dumper.dump_json();
    std::cout << json_data << std::endl;
    return 0;
}

int do_dump(HakoSimCtrlPtr& hako_sim_ctrl, Args args) {
    if (args.size() < 2) {
        std::cerr << "Usage: hako-cmd dump <channel_id>\n";
        return 1;
    }
    int channel_id = std::stoi(args[1]);
    size_t size = hako_sim_ctrl->pdu_size(channel_id);
    if (size == (size_t)(-1)) {
        printf("ERROR: channel id is invalid\n");
        return 1;
    }
    char *pdu_data = (char*)malloc(size);
    if (pdu_data == nullptr) {
        printf("ERROR: can not allocate memory...\n");
        return 1;
    }
    bool ret = hako_sim_ctrl->read_pdu(channel_id, pdu_data, size);
    if (ret) {
#ifdef WIN32
        DWORD written;
        HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
        WriteFile(hStdout, pdu_data, size, &written, NULL);
#else
        if (write(1, pdu_data, size) < 0) { perror("write"); }
#endif
    } else {
        printf("ERROR: internal error..\n");
        free(pdu_data);
        return 1;
    }
    free(pdu_data);
    return 0;
}

int do_restore(HakoSimCtrlPtr& hako_sim_ctrl, Args args) {
    if (args.size() < 3) {
        std::cerr << "Usage: hako-cmd restore <channel_id> <filepath>\n";
        return 1;
    }
    int channel_id = std::stoi(args[1]);
    std::string restore_file = args[2];
    
    size_t size = hako_sim_ctrl->pdu_size(channel_id);
    if (size == (size_t)(-1)) { printf("ERROR: channel id is invalid\n"); return 1; }

    char *pdu_data = (char*)malloc(size);
    if (pdu_data == nullptr) { printf("ERROR: can not allocate memory...\n"); return 1; }

    if (read_file_data(restore_file, size, pdu_data) == 0) {
        if (!hako_sim_ctrl->write_pdu(channel_id, pdu_data, size)) {
            printf("ERROR: hako_sim_ctrl->write_pdu error.. channel_id=%d\n", channel_id);
            free(pdu_data);
            return 1;
        }
    } else {
        printf("ERROR: internal error..\n");
        free(pdu_data);
        return 1;
    }
    free(pdu_data);
    return 0;
}

int do_real_cid(HakoSimCtrlPtr& hako_sim_ctrl, Args args) {
    if (args.size() < 3) {
        std::cerr << "Usage: hako-cmd real_cid <asset_name> <channel_id>\n";
        return 1;
    }
    std::string asset_name = args[1];
    int channel_id = std::stoi(args[2]);
    auto real_cid = hako_sim_ctrl->get_pdu_channel(asset_name, channel_id);
    printf("%d\n", real_cid);
    return 0;
}

void setup_options(cxxopts::Options& options) {
    options.add_options()
        ("h,help", "Print usage")
        ("v,version", "Print version")
        ("positional", "Command and arguments", cxxopts::value<std::vector<std::string>>());
    
    options.parse_positional({"positional"});
    options.custom_help("[command] [options]");
}

int main(int argc, char* argv[]) {
    signal(SIGINT, hako_cmd_signal_handler);
    signal(SIGTERM, hako_cmd_signal_handler);

    cxxopts::Options options(argv[0], "Hakoniwa command-line tool");
    setup_options(options);

    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);
    } catch (const cxxopts::exceptions::exception& e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        return 1;
    }

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        // Add custom help for subcommands
        std::cout << "\nCommands:\n"
                  << "  start          Start simulation\n"
                  << "  stop           Stop simulation\n"
                  << "  reset          Reset simulation\n"
                  << "  status         Show simulation status\n"
                  << "  ls             List assets\n"
                  << "  dump           Dump PDU channel data\n"
                  << "  restore        Restore PDU channel data\n"
                  << "  real_cid       Get real PDU channel ID\n"
                  << "  pmeta          Print master metadata\n"
                  << "  plog           Print memory log\n"
                  << "  jmeta          Dump simulation metadata as JSON\n";
        return 0;
    }
    if (result.count("version")) {
        std::cout << "hako-cmd version 1.0.0" << std::endl;
        return 0;
    }

    if (!result.count("positional")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    auto& positional_args = result["positional"].as<std::vector<std::string>>();
    std::string command = positional_args[0];

    std::map<std::string, std::function<int(HakoSimCtrlPtr&, Args)>> command_map = {
        {"start", do_start}, {"stop", do_stop}, {"reset", do_reset},
        {"status", do_status}, {"ls", do_ls}, {"pmeta", do_pmeta},
        {"plog", do_plog}, {"jmeta", do_jmeta}, {"dump", do_dump},
        {"restore", do_restore}, {"real_cid", do_real_cid}
    };

    HakoSimCtrlPtr hako_sim_ctrl = nullptr;
    if (command != "jmeta") {
        hako_sim_ctrl = hako::get_simevent_controller();
        if (hako_sim_ctrl == nullptr) {
            std::cout << "ERROR: Not found hako-master on this PC" << std::endl;
            return 1;
        }
    }

    int ret = 0;
    auto it = command_map.find(command);
    if (it != command_map.end()) {
        ret = it->second(hako_sim_ctrl, positional_args);
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        std::cout << options.help() << std::endl;
        ret = 1;
    }

    hako::destroy();
    return ret;
}
