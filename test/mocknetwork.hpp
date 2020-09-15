#include "gmock/gmock.h"
#include <vector>
#include <string>

class network
{
    public:
        bool connect_network(const string ssid, const string passphrase);
        vector<string> get_available_networks();
        vector<string> run_command(const string command);
        bool test_connection();
        bool wifi_adapter_up();
};

class mocknetwork : public network {
    public:
        MOCK_METHOD(vector<string>, run_command, (), (override));
}