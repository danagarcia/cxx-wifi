#include "mocknetwork.hpp"
#include "gmock\gmock.h"
#include "gtest\gtest.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>
using namespace testing;
using namespace std;
using ::testing::Return;
using ::testing::Throw;

bool network::connect_network(const string ssid, const string passphrase)
{
    // Configure network config block for config file
    string network_config_command = "echo '\\nnetwork={\\n ssid=\"" + ssid + "\"\\n scan_ssid=1\\n psk=\"" + passphrase + "\"\\n key_mgmt=WPA-PSK\\n}' >> /etc/wpa_supplicant/wpa_supplicant.conf";
    // Write network config block to config file
    try
    {
        vector<string> network_config_result = this->run_command(network_config_command);
    }
    catch (runtime_error& e)
    {
        throw runtime_error("Unable to set network config block in /etc/wpa_supplicant/wpa_supplicant.conf");
    }
    // Disable wlan0
    try
    {
        vector<string> network_disable_result = this->run_command("sudo ifconfig wlan0 down");
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
        vector<string> network_enable_result = this->run_command("sudo ifconfig wlan0 up");
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
        this_thread::sleep_for(chrono::seconds(1));
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

vector<string> network::get_available_networks()
{
    try
    {
        vector<string> iwlist_result = this->run_command("iwlist wlan0 scanning | grep ESSID | sed -r 's/ESSID:\"+//' | sed -r 's/\"+//'");
    }
    catch(runtime_error& e)
    {
        throw runtime_error("A error occurred while running `iwlist wlan0 scanning`");
    }
    return iwlist_result;
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
        string ping_stats = ping_result[ping_result.size() - 2];
        // Check if 0 received is present
        if ( ping_stats.find( "0 received" ) == string::npos )
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
        vector<string> network_status = this->run_command("ifconfig wlan0");
    }
    catch(runtime_error& e)
    {
        cerr << "A error occurred while trying to run `ifconfig wlan0`: " << e.what() << endl();
    }
    // Determine status using first string
    if (network_status[0].find("<UP,") != string::npos)
    {
        return true;
    }
    else if (network_status[0].find("<BROADCAST,") != string::npos)
    {
        return false;
    }
    else
    {
        throw runtime_error("Unable to determine adapter status of wlan0. To debug run `ifconfig` and ensure wlan0 is present.");
    }   
}

TEST(ConnectNetworkTests, write_succeeds_with_no_wlan_adapter)
{
    mocknetwork network;
    vector<string> blank_result;
    EXPECT_CALL(network, run_command())
        .Times(2)
        .WillOnce(Return(blank_result))
        .WillOnce(Throw(runtime_error("unable to run"));
    EXPECT_THROW(network.connect_network("wifi","passphrase"), runtime_error);   
}

TEST(ConnectNetworkTests, write_and_wlan_adapter_reset_succeed_with_no_connectivity)
{
    mocknetwork network;
    vector<string> blank_result;
    vector<string> up_adapter_result{
        "wlan0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500",
        "inet 192.168.254.1  netmask 255.255.255.0  broadcast 192.168.254.255",
        "inet6 0000::0000:0000:0000:0000  prefixlen 64  scopeid 0x20<link>",
        "ether 00:00:00:00:00:00  txqueuelen 1000  (Ethernet)",
        "RX packets 0  bytes 0 (0 MiB)",
        "RX errors 0  dropped 0  overruns 0  frame 0",
        "TX packets 0  bytes 0 (0 MiB)",
        "TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0"
    };
    vector<string> down_adapter_result{
        "wlan0: flags=4098<BROADCAST,MULTICAST>  mtu 1500",
        "ether 00:00:00:00:00:00  txqueuelen 1000  (Ethernet)",
        "RX packets 0  bytes 0 (0 MiB)",
        "RX errors 0  dropped 0  overruns 0  frame 0",
        "TX packets 0  bytes 0 (0 MiB)",
        "TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0"
    };
    vector<string> failed_ping_result{
        "PING 1.1.1.1 (1.1.1.1) 56(84) bytes of data.",
        "",
        "--- 1.1.1.1 ping statistics ---",
        "4 packets transmitted, 0 received, 100% packet loss, time 93ms"
    };
    EXPECT_CALL(network, run_command())
        .WillOnce(Return(blank_result))
        .WillOnce(Return(blank_result))
        .WillOnce(Return(down_adapter_result))
        .WillOnce(Return(blank_result))
        .WillOnce(Return(up_adapter_result))
        .WillRepeatedly(Return(failed_ping_result));
    EXPECT_FALSE(network.connect_network("wifi","passphrase"));
}

int _tmain(int argc, _TCHAR* argv[])
{
    ::InitGoogleMock( &amp;argc, arv );
    return RUN_ALL_TESTS();
}