#include "ha_discovery.hpp"
#include "log_util.hpp"
#include "utf8_converter.hpp"
#include "display.hpp"
#include "timer.hpp"
#include <nlohmann/json.hpp>
#include <mosquitto.h>
#include <chrono>

using json = nlohmann::json;
using namespace std::chrono;

namespace ha_discovery {

HADiscoveryManager::HADiscoveryManager(const HAConfig& config) : config_(config) {
    DEBUG_LOG("HA Discover Manager");
    DEBUG_LOG("  Device ID: " << config_.device_id);
    DEBUG_LOG("  Topic Prefix: " << config_.topic_prefix);
    DEBUG_LOG("  HA Discovery Prefix: " << config_.ha_discovery_prefix);

    running = true;
}

HADiscoveryManager::~HADiscoveryManager() {
    if (lifeline_timer_) {
        lifeline_timer_->stop();
    }
}

void HADiscoveryManager::close(struct mosquitto* mosq) {
    if (running) {
        publishAvailability(mosq, false);
        // Give a moment for the message to be sent
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    running = false;
}

void HADiscoveryManager::publishDeviceDiscovery(struct mosquitto* mosq) const {
    // This method is now deprecated - use publishSensorDiscovery instead
    publishSensorDiscovery(mosq);
}

void HADiscoveryManager::on_connect(struct mosquitto* mosq) const {
    mosquitto_subscribe(mosq, nullptr, (getCommandTopic()).c_str(), 0);
    mosquitto_subscribe(mosq, nullptr, "homeassistant/status", 0);
    INFO_LOG("Subscribed to homeassistant topics (homeassistant/status, " << getCommandTopic() << ")");

    publishDeviceDiscovery(mosq);
    publishAvailability(mosq, true);
    publishDeviceState(mosq, "", "", DEFAULT_BRIGHTNESS);

    const_cast<HADiscoveryManager*>(this)->lifeline_timer_ = timer::createTimer(30000ms, [this, mosq]() {
        publishAvailability(mosq, true);
    });

    std::string lwt_topic = getLWTTopic();
    std::string lwt_payload = getLWTPayload();
    int lwt_result = mosquitto_will_set(mosq, lwt_topic.c_str(), 
                                       static_cast<int>(lwt_payload.length()), 
                                       lwt_payload.c_str(), 1, true);
    if (lwt_result != MOSQ_ERR_SUCCESS) {
        WARN_LOG("Failed to set LWT message: " << mosquitto_strerror(lwt_result));
    } else {
        DEBUG_LOG("Set LWT message for availability topic");
    }
}

bool HADiscoveryManager::on_message(struct mosquitto* mosq, std::string& topic, std::string& payload, std::function<void()>& clearDisplay) const {
    if (isCommandTopic(topic)) {
        return handleCommand(payload, clearDisplay);
    } else if (topic == "homeassistant/status") {
        DEBUG_LOG("Received homeassistant/status message: " << payload);
        if (payload == "online") {
            on_connect(mosq);
        }
        return true;
    }
    return false;
}

void HADiscoveryManager::publishSensorDiscovery(struct mosquitto* mosq) const {
    // Create single device-based discovery payload
    json discovery_payload = {
        {"device", {
            {"identifiers", config_.device_id},
            {"name", "Raspberry Display"},
            {"manufacturer", "Raspberry Pi Foundation"},
            {"model", "LED Display"},
            {"sw_version", "1.0"},
            {"serial_number", config_.device_id},
            {"hw_version", "1.0"},
            // TOOD: add this later
            //{"connections", json::array({{"mac", "00:11:22:33:44:55"}})}
        }},
        {"origin", {
            {"name", "raspberry-display"},
            {"sw", "1.0"},
            {"url", "https://github.com/torkildr/raspberry-display"}
        }},
        {"components", {
            {"led_display_" + config_.device_id + "_text", {
                {"platform", "sensor"},
                {"name", "Display Text"},
                {"value_template", "{{ value_json.text }}"},
                {"icon", "mdi:monitor"},
                {"unique_id", config_.device_id + "_text"},
            }},
            {"led_display_" + config_.device_id + "_brightness", {
                {"platform", "sensor"},
                {"name", "Display Brightness"},
                {"value_template", "{{ value_json.brightness }}"},
                {"unit_of_measurement", "%"},
                {"icon", "mdi:brightness-6"},
                {"unique_id", config_.device_id + "_brightness"},
            }},
            {"led_display_" + config_.device_id + "_clear", {
                {"platform", "button"},
                {"name", "Clear Display"},
                {"command_topic", getCommandTopic()},
                {"payload_press", "{\"action\": \"clear\"}"},
                {"icon", "mdi:monitor-off"},
                {"unique_id", config_.device_id + "_clear"},
            }}
        }},
        {"state_topic", getStateTopic()},
        {"availability_topic", getAvailabilityTopic()},
        {"availability_template", "{{ value_json }}"},
        {"qos", 1}
    };
    
    std::string topic = config_.ha_discovery_prefix + "/device/" + config_.device_id + "/config";
    std::string payload = discovery_payload.dump();
    
    int result = mosquitto_publish(mosq, nullptr, topic.c_str(),
                                 static_cast<int>(payload.length()), payload.c_str(), 1, false);
    
    if (result == MOSQ_ERR_SUCCESS) {
        LOG("Published Home Assistant device discovery configuration");
    } else {
        WARN_LOG("Failed to publish device discovery: " << mosquitto_strerror(result));
    }
}

void HADiscoveryManager::publishAvailability(struct mosquitto* mosq, bool online) const {
    std::string availability_topic = getAvailabilityTopic();
    std::string payload = json(online ? "online" : "offline").dump();
    
    int result = mosquitto_publish(mosq, nullptr, availability_topic.c_str(),
                                 static_cast<int>(payload.length()), payload.c_str(), 1, false);
    
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
                                 static_cast<int>(payload_str.length()), payload_str.c_str(), 1, false);
    
    if (result == MOSQ_ERR_SUCCESS) {
        DEBUG_LOG("Published device state");
    } else {
        WARN_LOG("Failed to publish device state: " << mosquitto_strerror(result));
    }
}

std::string HADiscoveryManager::getAvailabilityTopic() const {
    return config_.topic_prefix + "/availability/" + config_.device_id;
}

std::string HADiscoveryManager::getStateTopic() const {
    return config_.topic_prefix + "/state/" + config_.device_id;
}

std::string HADiscoveryManager::getCommandTopic() const {
    return config_.topic_prefix + "/command/" + config_.device_id;
}

std::string HADiscoveryManager::getLWTTopic() const {
    return getAvailabilityTopic();
}

std::string HADiscoveryManager::getLWTPayload() const {
    return json("offline").dump();
}

bool HADiscoveryManager::isCommandTopic(const std::string& topic) const {
    return topic.find("/command/" + config_.device_id) != std::string::npos;
}

bool HADiscoveryManager::handleCommand(std::string& payload, std::function<void()>& clearDisplay) const {
    try {
        json message = json::parse(payload);
        if (message.contains("action")) {
            if (message["action"] == "clear") {
                clearDisplay();
                DEBUG_LOG("Processed clear command from Home Assistant");
            }
            return true;
        }
      } catch (const json::parse_error& e) {
        WARN_LOG("JSON parse error: " << e.what());
        DEBUG_LOG("Payload: " << payload);
    }
    return false;
}

} // namespace ha_discovery
