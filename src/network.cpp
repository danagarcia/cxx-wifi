/* 
CXX-WiFi - Linux Network Manager (WPA) WiFi Connection Library for C++
Copyright (C) 2020 - Dana Garcia
Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0
See accompanying file LICENSE
*/

#include <iostream>
#include "network.hpp"
#include "includes\pstreams\pstream.h"
#include <string>
#include <thread>
#include <vector>

using namespace cxxwifi;
using namespace std;

bool network::connect_network(const std::string ssid, const std::string passphrase)
{
    // Configure network config block for config file
    std::string network_config_command = "echo '\\nnetwork={\\n ssid=\"" + ssid + "\"\\n scan_ssid=1\\n psk=\"" + passphrase + "\"\\n key_mgmt=WPA-PSK\\n}' >> /etc/wpa_supplicant/wpa_supplicant.conf";
    // Write network config block to config file
    try
    {
        std::vector<std::string> network_config_result = this->run_command(network_config_command);
    }
    catch (runtime_error& e)
    {
        throw runtime_error("Unable to set network config block in /etc/wpa_supplicant/wpa_supplicant.conf");
    }
    // Disable wlan0
    try
    {
        std::vector<std::string> network_disable_result = this->run_command("sudo ifconfig wlan0 down");
    }
    catch (runtime_error& e)
    {
        throw runtime_error("Unable to disable wlan0 adapter");
    }
    // Wait for adapter to show as down
    bool network_adapter_is_up = this->wifi_adapter_up();
    while (network_adapter_is_up)
    {
        // Sleep for 1/4 second
        this_thread::sleep_for(chrono::milliseconds(250));
        // Recheck for adapter up status
        network_adapter_is_up = this->wifi_adapter_up();
    }
    // Enable wlan0
    try
    {
        std::vector<std::string> network_enable_result = this->run_command("sudo ifconfig wlan0 up");
    }
    catch (runtime_error& e)
    {
        throw runtime_error("Unable to enable wlan0 adapter");
    }
    // Wait for adapter to show as up
    network_adapter_is_up = this->wifi_adapter_up();
    while(!network_adapter_is_up)
    {
        // Sleep for 1/4 second
        this_thread::sleep_for(chrono::milliseconds(250));
        // Recheck for adapter up status
        network_adapter_is_up = this->wifi_adapter_up();
    }
    // Check for connectivity
    bool connected = false;
    int attempts_left = 10;
    while ( !connected && attempts_left > 0)
    {
        // Ping cloud flare's public DNS server(s)
        bool connection_status = test_connection();
        if ( connection_status )
        {
            // Success connected to true to break out of loop; Could use `break;` here too.
            connected = true;
        }
        else
        {
            // Not connected, reduce attempts_left by 1
            attempts_left--;
        }
        // Sleep for 1 second to give some time between tries
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    // Return result
    if (connected)
    {
        return true;
    }
    else
    {
        return false;
    }
}

std::vector<std::string> network::get_available_networks()
{
    try
    {
        std::vector<std::string> iwlist_result = this->run_command("iwlist wlan0 scanning | grep ESSID | sed -r 's/ESSID:\"+//' | sed -r 's/\"+//'");
    }
    catch(runtime_error& e)
    {
        throw runtime_error("A error occurred while running `iwlist wlan0 scanning`");
    }
    return iwlist_result;
}

std::vector<std::string> network::run_command(const std::string command)
{
    std::string output_line, error_line;
    std::vector<std::string> result, error;
    // Run command
    redi::ipstream process(command, redi::pstreams::pstdout | redi::pstreams::pstderr);
    // Gather output
    while ( std::getline( process.out(), output_line ) )
    {
        result.push_back( output_line );
    }
    // Clear process at end of output or failure
    if ( process.eof() && process.fail() )
    {
        process.clear();
    }
    // Gather error output
    while ( std::getline( process.err(), error_line ) )
    {
        error.push_back( error_line );
    }
    // If errors exist, write errors as C errors, then throw
    if ( !error.empty() )
    {
        std::cerr << "An error occurred while attempting to run: " << command << std::endl;
        for (std::size_t i = 0; i < error.size(); i++)
        {
            std::cerr << error[i] << std::endl;
        }
        throw runtime_error("An error occurred while running the provided command" << command);
    }
    // Return result
    return result;
}

bool network::test_connection()
{
    // Ping cloudflare public DNS with 4 packets
    try
    {
        vector<string> ping_result = this->run_command("ping -c 4 1.1.1.1");
    }
    catch (runtime_error& e)
    {
        cerr << "A error occurred while testing connectivity: " << e.what() << endl();
        return false;
    }
    // Check if result array contains 9 lines (standard for 4 packets)
    if ( ping_result.size() == 9 )
    {
        // Get the ping stats line (line 7)
        std::string ping_stats = ping_result[ping_result.size() - 2];
        // Check if 0 received is present
        if ( ping_stats.find( "0 received" ) == std::string::npos )
        {
            // 0 received not present, meaning some packets were received, test successful, return true
            return true;
        }
        else
        {
            // 0 received was found, meaning all packets were lost, test unsuccessful, return false
            return false;
        }
    }
    else
    {
        // Standard 9 line result not received, test unsuccessful, return false
        return false;
    }
}

bool network::wifi_adapter_up()
{
    // Get wlan0 adapter info
    try
    {
        std::vector<std::string> network_status = this->run_command("ifconfig wlan0");
    }
    catch(runtime_error& e)
    {
        cerr << "A error occurred while trying to run `ifconfig wlan0`: " << e.what() << endl();
    }
    // Determine status using first string
    if(network_status[0].find("<DOWN,") != std::string::npos)
    {
        return false;
    }
    else if (network_status[0].find("<UP,") != std::string::npos)
    {
        return true;
    }
    else
    {
        throw runtime_error("Unable to determine adapter status of wlan0. To debug run `ifconfig` and ensure wlan0 is present.");
    }   
}