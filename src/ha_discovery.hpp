#pragma once

#include "display.hpp"
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
    ~HADiscoveryManager();
    
    void close(struct mosquitto* mosq);
    void on_connect(struct mosquitto* mosq) const;
    bool on_message(struct mosquitto* mosq, std::string& topic, std::string& payload, std::function<void()>& clearDisplay) const;
    void publishDeviceState(
        struct mosquitto* mosq,
        const std::string& text = "",
        const std::string& time_format = "",
        int brightness = DEFAULT_BRIGHTNESS
    ) const;
    
private:
    HAConfig config_;
    std::unique_ptr<timer::Timer> lifeline_timer_;
    bool running;

    // Discovery operations
    void publishDeviceDiscovery(struct mosquitto* mosq) const;
    void publishSensorDiscovery(struct mosquitto* mosq) const;
    void publishAvailability(struct mosquitto* mosq, bool online) const;
    
    // Topic helpers
    std::string getAvailabilityTopic() const;
    std::string getStateTopic() const;
    std::string getCommandTopic() const;
    std::string getLWTTopic() const;
    std::string getLWTPayload() const;
    
    // Command processing
    bool isCommandTopic(const std::string& topic) const;
    bool handleCommand(std::string& payload, std::function<void()>& clearDisplay) const;

};

} // namespace ha_discovery
