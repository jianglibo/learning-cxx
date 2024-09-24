#pragma once
#ifndef SERVER_ASYNC_SETTINGS_H
#define SERVER_ASYNC_SETTINGS_H

#include <string>
#include <filesystem>

namespace server_async
{
    struct Settings
    {
        std::string address;
        std::string port;
        std::string doc_root;
        std::string cert_file;
        std::string key_file;
        std::string dh_file;
        std::string threads;
        std::string ssl;
        std::filesystem::path tmp_dir;
    };
}

#endif
