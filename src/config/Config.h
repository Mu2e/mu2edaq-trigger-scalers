#pragma once

#include <string>
#include <vector>
#include <stdexcept>

struct AlarmConfig {
    bool enabled{false};
    double min_rate{-1.0};  // Hz; negative = disabled
    double max_rate{-1.0};  // Hz; negative = disabled
};

struct TriggerConfig {
    int id{0};              // Category index matching TriggerMessage::category
    std::string name;
    double target_rate{-1.0};  // Expected rate in Hz (informational, shown in tooltip)
    bool visible{true};
    AlarmConfig alarm;
};

struct BankConfig {
    std::string name{"Trigger Scalers"};
    std::vector<TriggerConfig> triggers;
};

struct UdpConfig {
    std::string bind_address{"0.0.0.0"};
    int port{5555};
};

struct ZmqConfig {
    std::string endpoint{"tcp://localhost:5556"};
    std::string socket_type{"sub"};  // "sub" (PUB/SUB) or "pull" (PUSH/PULL)
};

struct TransportConfig {
    std::string protocol{"udp"};  // "udp" or "zeromq"
    UdpConfig udp;
    ZmqConfig zeromq;
};

struct DisplayConfig {
    std::string size{"medium"};       // small | medium | large | huge
    int update_interval_ms{1000};     // Rate calculation and display refresh interval
    std::string window_title{"Mu2e Trigger Scalers"};
    int columns{4};                   // Scalar widgets per row within each bank
    std::string color_scheme{"Amber"};
};

struct AppConfig {
    DisplayConfig display;
    TransportConfig transport;
    std::vector<BankConfig> banks;
};

AppConfig loadConfig(const std::string& filename);
void      saveConfig(const AppConfig& config, const std::string& filename);
