#pragma once

#include <string>
#include <mosquitto.h>
#include <nlohmann/json.hpp>

namespace ha_discovery {

// Configuration structure for Home Assistant integration
struct HAConfig {
    std::string device_id;
    std::string topic_prefix;
    std::string ha_discovery_prefix = "homeassistant";
};

// Home Assistant Discovery Manager
class HADiscoveryManager {
public:
    explicit HADiscoveryManager(const HAConfig& config);
    
    // Discovery operations
    void publishDeviceDiscovery(struct mosquitto* mosq) const;
    void publishSensorDiscovery(struct mosquitto* mosq) const;
    void publishAvailability(struct mosquitto* mosq, bool online) const;
    void publishDeviceState(struct mosquitto* mosq, const std::string& text = "", const std::string& time_format = "", int brightness = 15) const;
    
    // Topic helpers
    std::string getAvailabilityTopic() const;
    std::string getStateTopic() const;
    std::string getCommandTopic() const;
    std::string getLWTTopic() const;
    std::string getLWTPayload() const;
    
    // Command processing
    bool isCommandTopic(const std::string& topic) const;
    std::string processCommand(const std::string& payload) const;
    
private:
    HAConfig config_;
    
    // Helper methods (removed - now using single device discovery)
};

} // namespace ha_discovery
