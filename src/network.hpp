/* 
CXX-WiFi - Linux Network Manager (WPA) WiFi Connection Library for C++
Copyright (C) 2020 - Dana Garcia
Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0
See accompanying file LICENSE
*/
#include <string>
#include <vector>

namespace cxxwifi
{
    class network
    {
        public:
            bool connect_network(const std::string ssid, const std::string passphrase);
            std::vector<std::string> get_available_networks();
            std::vector<std::string> run_command(const std::string command);
            bool test_connection();
            bool wifi_adapter_up();
    };
}