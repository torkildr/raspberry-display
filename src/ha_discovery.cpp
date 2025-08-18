#include "ha_discovery.hpp"
#include "log_util.hpp"
#include "utf8_converter.hpp"
#include <nlohmann/json.hpp>
#include <mosquitto.h>
#include <chrono>

using json = nlohmann::json;

namespace ha_discovery {

HADiscoveryManager::HADiscoveryManager(const HAConfig& config) : config_(config) {
    DEBUG_LOG("HA Discover Manager");
    DEBUG_LOG("  Device ID: " << config_.device_id);
    DEBUG_LOG("  Topic Prefix: " << config_.topic_prefix);
    DEBUG_LOG("  HA Discovery Prefix: " << config_.ha_discovery_prefix);
}

void HADiscoveryManager::publishDeviceDiscovery(struct mosquitto* mosq) const {
    // This method is now deprecated - use publishSensorDiscovery instead
    publishSensorDiscovery(mosq);
}

void HADiscoveryManager::publishSensorDiscovery(struct mosquitto* mosq) const {
    // Publish individual sensor configurations as per HA documentation
    
    // 1. Display Text Sensor
    {
        json config = createTextSensorConfig();
        std::string topic = config_.ha_discovery_prefix + "/sensor/" + config_.device_id + "_text/config";
        std::string payload = config.dump();
        
        int result = mosquitto_publish(mosq, nullptr, topic.c_str(),
                                     static_cast<int>(payload.length()), payload.c_str(), 1, true);
        
        if (result == MOSQ_ERR_SUCCESS) {
            DEBUG_LOG("Published text sensor discovery");
        } else {
            WARN_LOG("Failed to publish text sensor discovery: " << mosquitto_strerror(result));
        }
    }
    
    // 2. Display Brightness Sensor
    {
        json config = createBrightnessSensorConfig();
        std::string topic = config_.ha_discovery_prefix + "/sensor/" + config_.device_id + "_brightness/config";
        std::string payload = config.dump();
        
        int result = mosquitto_publish(mosq, nullptr, topic.c_str(),
                                     static_cast<int>(payload.length()), payload.c_str(), 1, true);
        
        if (result == MOSQ_ERR_SUCCESS) {
            DEBUG_LOG("Published brightness sensor discovery");
        } else {
            WARN_LOG("Failed to publish brightness sensor discovery: " << mosquitto_strerror(result));
        }
    }
    
    // 3. Clear Display Button
    {
        json config = createClearButtonConfig();
        std::string topic = config_.ha_discovery_prefix + "/button/" + config_.device_id + "_clear/config";
        std::string payload = config.dump();
        
        int result = mosquitto_publish(mosq, nullptr, topic.c_str(),
                                     static_cast<int>(payload.length()), payload.c_str(), 1, true);
        
        if (result == MOSQ_ERR_SUCCESS) {
            DEBUG_LOG("Published clear button discovery");
        } else {
            WARN_LOG("Failed to publish clear button discovery: " << mosquitto_strerror(result));
        }
    }
    
    LOG("Published Home Assistant sensor discovery configurations");
}

void HADiscoveryManager::publishAvailability(struct mosquitto* mosq, bool online) const {
    std::string availability_topic = getAvailabilityTopic();
    std::string payload = online ? "online" : "offline";
    
    int result = mosquitto_publish(mosq, nullptr, availability_topic.c_str(),
                                 static_cast<int>(payload.length()), payload.c_str(), 1, true);
    
    if (result == MOSQ_ERR_SUCCESS) {
        DEBUG_LOG("Published availability: " << payload);
    } else {
        WARN_LOG("Failed to publish availability: " << mosquitto_strerror(result));
    }
}

void HADiscoveryManager::publishDeviceState(struct mosquitto* mosq, const std::string& text, const std::string& time_format, int brightness) const {
    // Create display content based on what's actually being shown
    std::string display_content;
    if (!text.empty() && !time_format.empty()) {
        display_content = text + " (" + time_format + ")";
    } else if (!text.empty()) {
        display_content = text;
    } else if (!time_format.empty()) {
        display_content = "Time: " + time_format;
    } else {
        display_content = "<empty>";
    }
    
    // Convert Latin1 display content to UTF-8 for Home Assistant
    std::string utf8_display_content;
    try {
        utf8_display_content = utf8::toUtf8(display_content);
    } catch (const std::exception& e) {
        ERROR_LOG("Failed to convert display content to UTF-8: " << e.what());
        utf8_display_content = display_content; // Use original as fallback
    } catch (...) {
        ERROR_LOG("Unknown error converting display content to UTF-8");
        utf8_display_content = display_content; // Use original as fallback
    }
    
    json state_payload;
    try {
        state_payload = {
            {"text", utf8_display_content},
            {"brightness", round((brightness / 15.0) * 100.0)},
            {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()}
        };
    } catch (const std::exception& e) {
        ERROR_LOG("Failed to create JSON payload: " << e.what());
        return;
    } catch (...) {
        ERROR_LOG("Unknown error creating JSON payload");
        return;
    }
    
    std::string state_topic = getStateTopic();
    std::string payload_str;
    try {
        payload_str = state_payload.dump();
    } catch (const std::exception& e) {
        ERROR_LOG("Failed to serialize JSON payload: " << e.what());
        return;
    } catch (...) {
        ERROR_LOG("Unknown error serializing JSON payload");
        return;
    }
    
    int result = mosquitto_publish(mosq, nullptr, state_topic.c_str(),
                                 static_cast<int>(payload_str.length()), payload_str.c_str(), 0, false);
    
    if (result == MOSQ_ERR_SUCCESS) {
        DEBUG_LOG("Published device state");
    } else {
        WARN_LOG("Failed to publish device state: " << mosquitto_strerror(result));
    }
}

std::string HADiscoveryManager::getAvailabilityTopic() const {
    return config_.topic_prefix + "/availability";
}

std::string HADiscoveryManager::getStateTopic() const {
    return config_.topic_prefix + "/state";
}

std::string HADiscoveryManager::getCommandTopic() const {
    return config_.topic_prefix + "/command";
}

std::string HADiscoveryManager::getLWTTopic() const {
    return getAvailabilityTopic();
}

std::string HADiscoveryManager::getLWTPayload() const {
    return "offline";
}

bool HADiscoveryManager::isCommandTopic(const std::string& topic) const {
    return topic.find("/command") != std::string::npos;
}

std::string HADiscoveryManager::processCommand(const std::string& payload) const {
    try {
        json message_json = json::parse(payload);
        if (message_json.contains("action")) {
            std::string action = message_json["action"];
            if (action == "clear") {
                DEBUG_LOG("Processed clear command from Home Assistant");
                return "clear";
            }
        }
    } catch (const json::parse_error& e) {
        WARN_LOG("JSON parse error in HA command: " << e.what());
    }
    return "";
}

json HADiscoveryManager::createDeviceInfo() const {
    return {
        {"ids", {config_.device_id}},
        {"name", "Led Display " + config_.device_id},
        {"mf", "Raspberry Pi Foundation"},
        {"mdl", "Raspberry Pi Display"},
        {"sw", "1.0"},
        {"sn", config_.device_id},
        {"hw", "1.0"}
    };
}

json HADiscoveryManager::createTextSensorConfig() const {
    return {
        {"name", "Display Text"},
        {"unique_id", config_.device_id + "_text"},
        {"state_topic", getStateTopic()},
        {"availability_topic", getAvailabilityTopic()},
        {"value_template", "{{ value_json.text }}"},
        {"icon", "mdi:monitor"},
        {"device", createDeviceInfo()},
        {"origin", {
            {"name", "raspberry-display"},
            {"sw", "1.0"},
            {"url", "https://github.com/torkildr/raspberry-display"}
        }}
    };
}

json HADiscoveryManager::createBrightnessSensorConfig() const {
    return {
        {"name", "Display Brightness"},
        {"unique_id", config_.device_id + "_brightness"},
        {"state_topic", getStateTopic()},
        {"availability_topic", getAvailabilityTopic()},
        {"value_template", "{{ value_json.brightness }}"},
        {"unit_of_measurement", "%"},
        {"icon", "mdi:brightness-6"},
        {"device", createDeviceInfo()},
        {"origin", {
            {"name", "raspberry-display"},
            {"sw", "1.0"},
            {"url", "https://github.com/torkildr/raspberry-display"}
        }}
    };
}

json HADiscoveryManager::createClearButtonConfig() const {
    return {
        {"name", "Clear Display"},
        {"unique_id", config_.device_id + "_clear"},
        {"command_topic", getCommandTopic()},
        {"availability_topic", getAvailabilityTopic()},
        {"payload_press", "{\"action\": \"clear\"}"},
        {"icon", "mdi:monitor-off"},
        {"device", createDeviceInfo()},
        {"origin", {
            {"name", "raspberry-display"},
            {"sw", "1.0"},
            {"url", "https://github.com/torkildr/raspberry-display"}
        }}
    };
}

} // namespace ha_discovery
