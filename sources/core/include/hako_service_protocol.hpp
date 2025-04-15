#pragma once

#include "hako_service.hpp"
#include <memory>

namespace hako::service {
    class HakoServiceServerProtocol {
        public:
            HakoServiceServerProtocol(std::shared_ptr<IHakoServiceServer> server)
            {
                server_ = server;
            }
            ~HakoServiceServerProtocol() = default;

            HakoServiceServerEventType run();
            HakoServiceServerStateType state();
            void* get_request();
            void  put_reply(void* packet, int result_code);
            void  cancel_done();
            void  put_progress(int percentage);

        private:
            std::shared_ptr<IHakoServiceServer> server_;
    };
    class HakoServiceClientProtocol {
        public:
            HakoServiceClientProtocol(std::shared_ptr<IHakoServiceClient> client)
            {
                client_ = client;
            }
            ~HakoServiceClientProtocol() = default;

            HakoServiceClientEventType run();
            HakoServiceClientStateType state();
            bool  put_request(void* packet);
            void* get_response();
            void  cancel_request();
            int   get_progress();

        private:
            std::shared_ptr<IHakoServiceClient> client_;
    };
}