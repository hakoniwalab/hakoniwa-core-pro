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

static void hako_cmd_signal_handler(int sig)
{
    //hako::logger::get("cmd")->info("SIGNAL RECV: {0}", sig);
        printf("SIGNAL RECV: %d\n", sig);
}
#ifdef WIN32
static int read_file_data(char* filepath, int size, char* buffer) {
    HANDLE hFile = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("ERROR: cannot open file %s\n", filepath);
        return -1;
    }

    DWORD bytesRead;
    BOOL bResult = ReadFile(hFile, buffer, size, &bytesRead, NULL);
    if (!bResult || bytesRead == 0) {
        printf("ERROR: cannot read file %s\n", filepath);
        CloseHandle(hFile);
        return -1;
    }

    CloseHandle(hFile);
    return 0;
}
#else
static int read_file_data(char *filepath, int size, char* buffer)
{
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        printf("ERROR: can not open file %s errno=%d\n", filepath, errno);
        return -1;
    }
    int ret = read(fd, buffer, size);
    if (ret < 0) {
        printf("ERROR: can not read file %s errno=%d\n", filepath, errno);
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}
#endif

int main(int argc, const char* argv[])
{
    std::vector<std::string> hako_status;

    hako_status.push_back("stopped");
    hako_status.push_back("runnable");
    hako_status.push_back("running");
    hako_status.push_back("stopping");
    hako_status.push_back("resetting");
    hako_status.push_back("error");
    hako_status.push_back("terminated");

    if ((argc == 1) || (argc > 4)) {
        printf("Usage: %s {start|stop|reset|status|pmeta|plog|dump <cid>|restore <cid> <bin>}|real_cid <asset_name> <cid>\n", argv[0]);
        return 1;
    }
    signal(SIGINT, hako_cmd_signal_handler);
    signal(SIGTERM, hako_cmd_signal_handler);

    std::string cmd = argv[1];
    int channel_id = 0;
    char *restore_file = nullptr;
    if (cmd == "dump") {
        if (argc != 3) {
            printf("ERROR: not set channel id\n");
            return 1;
        }
        channel_id = atoi(argv[2]);
        //printf("channel_id=%d\n", channel_id);
    }
    else if (cmd == "restore") {
        if (argc != 4) {
            printf("ERROR: not set channel id or restore bin file\n");
            return 1;
        }
        channel_id = atoi(argv[2]);
        restore_file = (char*)argv[3];
        //printf("channel_id=%d\n", channel_id);
    }

    //hako::logger::init("core");
    //hako::logger::init("cmd");
    //hako::logger::get("cmd")->info("cmd={0}", cmd);

    std::shared_ptr<hako::IHakoSimulationEventController> hako_sim_ctrl = hako::get_simevent_controller();
    if (hako_sim_ctrl == nullptr) {
        std::cout << "ERROR: Not found hako-master on this PC" << std::endl;
        return 1;
    }

    if (cmd == "start") {
        printf("start\n");
        hako_sim_ctrl->start();
    }
    else if (cmd == "stop") {
        printf("stop\n");
        hako_sim_ctrl->stop();
    }
    else if (cmd == "reset") {
        printf("reset\n");
        hako_sim_ctrl->reset();
    }
    else if (cmd == "status") {
        printf("status=%s\n", hako_status[hako_sim_ctrl->state()].c_str());
    }
    else if (cmd == "ls") {
        std::vector<std::shared_ptr<std::string>> asset_list;
        hako_sim_ctrl->assets(asset_list);
        for(std::shared_ptr<std::string> name :asset_list) {
            std::cout << *name << std::endl;
        }
    }
    else if (cmd == "dump") {
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
            if (hStdout == INVALID_HANDLE_VALUE) {
                std::cerr << "Failed to get the standard output handle." << std::endl;
                free(pdu_data);
                return 1;
            }

            BOOL bResult = WriteFile(hStdout, pdu_data, size, &written, NULL);
            if (!bResult) {
                std::cerr << "Failed to write data to standard output." << std::endl;
                return 1;
            }
#else
            write(1, pdu_data, size);
#endif
            free(pdu_data);
        }
        else {
            printf("ERROR: internal error..\n");
            free(pdu_data);
            return 1;
        }
    }
    else if (cmd == "restore") {
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
        int ret_val = read_file_data(restore_file, size, pdu_data);
        if (ret_val == 0) {
            bool ret = hako_sim_ctrl->write_pdu(channel_id, pdu_data, size);
            if (ret == false) {
                printf("ERROR: hako_sim_ctrl->write_pdu error.. channel_id=%d\n", channel_id);
                ret_val = -1;
            }
        }
        if (ret_val == 0) {
            free(pdu_data);
        }
        else {
            printf("ERROR: internal error..\n");
            free(pdu_data);
            return 1;
        }
    }
    else if (cmd == "real_cid") {
        std::string asset_name = argv[2];
        channel_id = atoi(argv[3]);
        auto real_cid = hako_sim_ctrl->get_pdu_channel(asset_name, channel_id);
        printf("%d\n", real_cid);
    }
    else if (cmd == "pmeta") {
        hako_sim_ctrl->print_master_data();
    }
    else if (cmd == "plog") {
        hako_sim_ctrl->print_memory_log();
    }
    else {
        printf("error\n");
    }
    hako::destroy();
    return 0;
}
