#include "Config.h"
#include <yaml-cpp/yaml.h>

static AlarmConfig parseAlarm(const YAML::Node& node) {
    AlarmConfig a;
    if (!node || !node.IsMap()) return a;
    a.enabled   = node["enabled"].as<bool>(false);
    a.min_rate  = node["min_rate"].as<double>(-1.0);
    a.max_rate  = node["max_rate"].as<double>(-1.0);
    return a;
}

static TriggerConfig parseTrigger(const YAML::Node& node) {
    TriggerConfig t;
    t.id          = node["id"].as<int>();
    t.name        = node["name"].as<std::string>();
    t.target_rate = node["target_rate"].as<double>(-1.0);
    t.visible     = node["visible"].as<bool>(true);
    t.alarm       = parseAlarm(node["alarm"]);
    return t;
}

static BankConfig parseBank(const YAML::Node& node) {
    BankConfig b;
    b.name = node["name"].as<std::string>("Trigger Scalers");
    for (const auto& t : node["triggers"]) {
        b.triggers.push_back(parseTrigger(t));
    }
    return b;
}

AppConfig loadConfig(const std::string& filename) {
    YAML::Node yaml;
    try {
        yaml = YAML::LoadFile(filename);
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to parse config '" + filename + "': " + e.what());
    }

    AppConfig config;

    if (auto d = yaml["display"]; d && d.IsMap()) {
        config.display.size               = d["size"].as<std::string>("medium");
        config.display.update_interval_ms = d["update_interval_ms"].as<int>(1000);
        config.display.window_title       = d["window_title"].as<std::string>("Mu2e Trigger Scalers");
        config.display.columns            = d["columns"].as<int>(4);
    }

    if (auto t = yaml["transport"]; t && t.IsMap()) {
        config.transport.protocol = t["protocol"].as<std::string>("udp");
        if (auto u = t["udp"]; u && u.IsMap()) {
            config.transport.udp.bind_address = u["bind_address"].as<std::string>("0.0.0.0");
            config.transport.udp.port         = u["port"].as<int>(5555);
        }
        if (auto z = t["zeromq"]; z && z.IsMap()) {
            config.transport.zeromq.endpoint    = z["endpoint"].as<std::string>("tcp://localhost:5556");
            config.transport.zeromq.socket_type = z["socket_type"].as<std::string>("sub");
        }
    }

    if (auto banks = yaml["banks"]; banks && banks.IsSequence()) {
        for (const auto& b : banks) {
            config.banks.push_back(parseBank(b));
        }
    }

    if (config.banks.empty()) {
        throw std::runtime_error("Config '" + filename + "' must define at least one bank with triggers");
    }

    return config;
}
