#include <cstring>
#include <iostream>
#include <string>
#include <signal.h>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <mosquitto.h>
#include <nlohmann/json.hpp>
#ifdef __linux__
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#include "display_impl.hpp"
#include "sequence.hpp"
#include "log_util.hpp"
#include "ha_discovery.hpp"
#include "pong.hpp"

using json = nlohmann::json;

// Global variables for signal handling
static bool running = true;
static display::Display* global_display = nullptr;
static struct mosquitto* mosq = nullptr;
static std::unique_ptr<sequence::SequenceManager> sequence_manager;
static std::unique_ptr<ha_discovery::HADiscoveryManager> ha_manager;

static struct display_state {
    std::string text;
    std::string time_format;
    int brightness;
} last_state;

#ifdef __linux__
// Systemd notification helper function
static void systemd_notify(const char* message) {
    const char* socket_path = std::getenv("NOTIFY_SOCKET");
    if (!socket_path) {
        return; // Not running under systemd or notifications not enabled
    }
    
    int fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    
    if (socket_path[0] == '@') {
        // Abstract socket
        strncpy(addr.sun_path + 1, socket_path + 1, sizeof(addr.sun_path) - 2);
    } else {
        strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    }
    
    sendto(fd, message, strlen(message), 0, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    close(fd);
}
#endif

// Connection state tracking
static bool mqtt_connected = false;
static int reconnect_delay = 1; // Start with 1 second
static const int max_reconnect_delay = 30; // Max 30 seconds
static auto last_connection_attempt = std::chrono::steady_clock::now();

// Configuration structure
struct MqttConfig {
    std::string host;
    int port = 1883;
    std::string client_id = "raspberry-display";
    std::string topic_prefix = "display";
    std::string username;
    std::string password;
    bool ha_reporting = false;
};

static void signal_handler(int signal) {
    LOG("Received signal " << signal << ", shutting down gracefully...");
    running = false;
    sequence_manager->stop();
}

static void process_add_sequence(const json& message) {
    try {
        if (!message.contains("state") || !message.contains("time") || (!message.contains("id") && !message.contains("sequence_id"))) {
            LOG("adding to sequence requires 'id', 'state' and 'time' fields");
            return;
        }
        
        double ttl = message.contains("ttl") ? message["ttl"].get<double>() : 0.0;
        double time = message["time"].get<double>();
        json state_json = message["state"];
        std::string sequence_id = message.contains("id")
            ? message["id"].get<std::string>()
            : (message.contains("sequence_id") ? message["sequence_id"].get<std::string>() : ""); // fall back to sequence_id for backwards compatability (for now)

        // Parse JSON to DisplayState
        sequence::DisplayState state = sequence::parseDisplayStateFromJSON(state_json);
        
        if (sequence_manager) {
            sequence_manager->addSequenceState(sequence_id, state, time, ttl);
            DEBUG_LOG("Added state to sequence with time=" << time << "s, ttl=" << ttl << "s, sequence_id='" << sequence_id << "'");
        }
        
    } catch (const std::exception& e) {
        LOG("Error processing addSequence: " << e.what());
    }
}

static void process_set_sequence(const json& message) {
    try {
        if (!message.is_array()) {
            LOG("setting sequence requires an array of sequence states");
            return;
        }
        
        std::vector<sequence::SequenceState> sequence_states;
        
        for (const auto& item : message) {
            if (!item.contains("state") || !item.contains("time") || (!item.contains("id") && !item.contains("sequence_id"))) {
                LOG("Each sequence item requires 'id', 'state' and 'time' fields");
                continue;
            }
            
            double ttl = item.contains("ttl") ? item["ttl"].get<double>() : 0.0;
            double time = item["time"].get<double>();
            json state_json = item["state"];
            std::string sequence_id = item.contains("id")
                ? item["id"].get<std::string>()
                : (item.contains("sequence_id") ? item["sequence_id"].get<std::string>() : ""); // fall back to sequence_id for backwards compatability (for now)
            
            // Parse JSON to DisplayState
            sequence::DisplayState state = sequence::parseDisplayStateFromJSON(state_json);
            
            sequence_states.push_back({
                sequence_id,
                std::chrono::steady_clock::now(),
                time,
                ttl,
                state,
            });
        }
        
        if (sequence_manager) {
            sequence_manager->setSequence(sequence_states);
        }
        
    } catch (const std::exception& e) {
        LOG("Error processing setSequence: " << e.what());
    }
}

static void process_clear_sequence(const json& message) {
    try {
        if (!sequence_manager) {
            return;
        }
        
        // Check if clearing all or specific sequence_id
        if (message.contains("sequence_id")) {
            std::string sequence_id = message["sequence_id"].get<std::string>();
            sequence_manager->clearSequenceById(sequence_id);
            DEBUG_LOG("Cleared sequence with id: '" << sequence_id << "'");
        } else {
            // Clear all sequences
            sequence_manager->clearSequence(true);
            DEBUG_LOG("Cleared all sequences");
        }
        
    } catch (const std::exception& e) {
        LOG("Error processing clearSequence: " << e.what());
    }
}

static void process_pong(const json& message) {
    try {
        if (!sequence_manager) {
            return;
        }
        
        if (message.contains("command")) {
            std::string command = message["command"].get<std::string>();
            
            if (command == "toggle") {
                // Toggle pong on/off
                sequence_manager->togglePongGame();
                DEBUG_LOG("Toggled pong game via MQTT");
            } else if (command == "control") {
                // Control paddle: {"command": "control", "direction": "up"/"down"/"stop"}
                if (message.contains("direction")) {
                    std::string direction = message["direction"].get<std::string>();
                    int control = 0;
                    if (direction == "up") {
                        control = -1;
                    } else if (direction == "down") {
                        control = 1;
                    }
                    sequence_manager->setPongPlayerControl(control);
                    DEBUG_LOG("Pong paddle control: " << direction);
                }
            }
        }
        
    } catch (const std::exception& e) {
        LOG("Error processing pong command: " << e.what());
    }
}

static void on_message(struct mosquitto* /*mosq*/, void* /*userdata*/, const struct mosquitto_message* message) {
    if (!message->payload) return;
    
    std::string topic(message->topic);
    std::string payload(static_cast<char*>(message->payload), static_cast<size_t>(message->payloadlen));
    
    DEBUG_LOG("Received MQTT message on topic: " << topic << " with payload: " << payload);
    
    std::function<void()> clearDisplay = [&]() {
        process_clear_sequence({});
    };
    // Check if HA manager handles this message
    bool ha_handled = ha_manager && ha_manager->on_message(mosq, topic, payload, clearDisplay);
    
    // If HA manager returned false, check if it's a pong command that needs processing
    if (ha_manager && !ha_handled) {
        try {
            json message_json = json::parse(payload);
            if (message_json.contains("action") && message_json["action"] == "pong") {
                process_pong(message_json);
                DEBUG_LOG("Processed pong command from HA topic: " << topic);
                return; // Exit early after processing pong
            }
        } catch (const json::parse_error& e) {
            // Ignore parse errors for HA commands
        }
    }
    
    // If HA fully handled the message, return early
    if (ha_handled) {
        return;
    }

    try {
        json message_json = json::parse(payload);

        if (topic == "display/add") {
            process_add_sequence(message_json);
        } else if (topic == "display/set") {
            process_set_sequence(message_json);
        } else if (topic == "display/clear") {
            process_clear_sequence(message_json);
        } else if (topic == "display/pong") {
            process_pong(message_json);
        } else if (topic == "display/quit") {
            DEBUG_LOG("Received quit message");
            running = false;
        } else {
            DEBUG_LOG("Unknown topic: " << topic);
        }
    } catch (const json::parse_error& e) {
        WARN_LOG("JSON parse error: " << e.what());
        DEBUG_LOG("Payload: " << payload);
    }
}

static void on_connect(struct mosquitto* mosq, void* userdata, int result) {
    if (result == 0) {
        LOG("Connected to MQTT broker successfully");
        mqtt_connected = true;
        reconnect_delay = 1; // Reset backoff on successful connection
        
        // Get config from userdata
        MqttConfig* config = static_cast<MqttConfig*>(userdata);
        std::string prefix = config->topic_prefix;
        
        // Subscribe to original display control topics
        mosquitto_subscribe(mosq, nullptr, (prefix + "/add").c_str(), 0);
        mosquitto_subscribe(mosq, nullptr, (prefix + "/set").c_str(), 0);
        mosquitto_subscribe(mosq, nullptr, (prefix + "/clear").c_str(), 0);
        mosquitto_subscribe(mosq, nullptr, (prefix + "/pong").c_str(), 0);
        mosquitto_subscribe(mosq, nullptr, (prefix + "/quit").c_str(), 0);
        LOG("Subscribed to " << prefix << " topics (add, set, clear, pong, quit)");
        
        // Publish Home Assistant discovery and availability
        if (ha_manager) {
            ha_manager->on_connect(mosq);
        }
        
    } else {
        WARN_LOG("Failed to connect to MQTT broker: " << mosquitto_connack_string(result));
        mqtt_connected = false;
    }
}

static void on_disconnect(struct mosquitto* /*mosq*/, void* /*userdata*/, int result) {
    mqtt_connected = false;
    if (result != 0) {
        WARN_LOG("Unexpected disconnection from MQTT broker");
    } else {
        WARN_LOG("Disconnected from MQTT broker");
    }
}

static void print_usage(const char* prog_name) {
    LOG("Usage: " << prog_name << " [host] [port]");
    LOG("");
    LOG("Configuration priority:");
    LOG("  1. Command line arguments (host, port)");
    LOG("  2. Environment variables");
    LOG("");
    LOG("Environment Variables:");
    LOG("  MQTT_HOST        - MQTT broker hostname/IP (required if not in args)");
    LOG("  MQTT_PORT        - MQTT broker port (default: 1883)");
    LOG("  MQTT_USERNAME    - MQTT username (optional)");
    LOG("  MQTT_PASSWORD    - MQTT password (optional)");
    LOG("  MQTT_CLIENT_ID   - MQTT client ID (default: raspberry-display)");
    LOG("  MQTT_TOPIC_PREFIX- Topic prefix (default: display)");
    LOG("  HA_REPORTING     - Enable Home Assistant reporting (true|false) (default: false)");
    LOG("");
    LOG("Examples:");
    LOG("  " << prog_name << " localhost 1883");
    LOG("  MQTT_HOST=broker.example.com " << prog_name);
    LOG("  MQTT_HOST=localhost MQTT_USERNAME=user MQTT_PASSWORD=pass " << prog_name);
}

static MqttConfig parse_config(int argc, char** argv) {
    MqttConfig config;
    
    // Get environment variables
    const char* env_host = std::getenv("MQTT_HOST");
    const char* env_port = std::getenv("MQTT_PORT");
    const char* env_username = std::getenv("MQTT_USERNAME");
    const char* env_password = std::getenv("MQTT_PASSWORD");
    const char* env_client_id = std::getenv("MQTT_CLIENT_ID");
    const char* env_topic_prefix = std::getenv("MQTT_TOPIC_PREFIX");
    const char* env_ha_reporting = std::getenv("HA_REPORTING");
    
    // Apply environment variables
    if (env_host) config.host = env_host;
    if (env_port) config.port = std::stoi(env_port);
    if (env_username) config.username = env_username;
    if (env_password) config.password = env_password;
    if (env_client_id) config.client_id = env_client_id;
    if (env_topic_prefix) config.topic_prefix = env_topic_prefix;

    // convert env "true" or "false" to bolean
    if (env_ha_reporting) config.ha_reporting = strcmp(env_ha_reporting, "true") == 0;

    // Command line arguments override environment variables
    if (argc >= 2) {
        config.host = argv[1];
    }
    if (argc >= 3) {
        config.port = std::stoi(argv[2]);
    }
    
    return config;
}

static bool attempt_reconnect(struct mosquitto* mosq, const MqttConfig& config) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_connection_attempt).count();
    
    if (elapsed >= reconnect_delay) {
        DEBUG_LOG("Attempting to reconnect to MQTT broker (" << config.host << ":" << config.port << ")...");
        int result = mosquitto_reconnect(mosq);
        last_connection_attempt = now;
        
        if (result == MOSQ_ERR_SUCCESS) {
            return true;
        }
        
        // Exponential backoff: double delay up to max
        reconnect_delay = std::min(reconnect_delay * 2, max_reconnect_delay);
        DEBUG_LOG("Reconnection failed, next attempt in " << reconnect_delay << " seconds");
    }
    
    return false;
}

int main(int argc, char** argv) {
    MqttConfig config = parse_config(argc, argv);
    
    // Validate configuration
    if (config.host.empty()) {
        ERROR_LOG("Error: MQTT host not specified");
        LOG("");
        print_usage(argv[0]);
        return 1;
    }
    
    LOG("MQTT Configuration:");
    LOG("  Host: " << config.host);
    LOG("  Port: " << config.port);
    LOG("  Client ID: " << config.client_id);
    LOG("  Topic Prefix: " << config.topic_prefix);
    LOG("  HA Reporting: " << ((config.ha_reporting) ? "Enabled" : "Disabled"));
    if (!config.username.empty()) {
        LOG("  Username: " << config.username);
        LOG("  Password: [provided]");
    }
    
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize display with empty callbacks and transfer ownership to SequenceManager
    auto preUpdate = []() {};
    auto postUpdate = []() {};
    
    // Display state callback for MQTT publishing
    auto displayStateCallback = [](const std::string& text, const std::string& time_format, int brightness) {
        if (ha_manager && mqtt_connected) {
            last_state.text = text;
            last_state.time_format = time_format;
            last_state.brightness = brightness;
            ha_manager->publishDeviceState(mosq, text, time_format, brightness);
        }
    };

    auto scrollCompleteCallback = []() {
        DEBUG_LOG("Scroll complete");
        sequence_manager->onScrollComplete();
    };
    
    auto display = std::make_unique<display::DisplayImpl>(preUpdate, postUpdate, displayStateCallback, scrollCompleteCallback);
    global_display = display.get(); // Keep pointer for signal handling
    
    // Initialize mosquitto library
    mosquitto_lib_init();
    
    // Create mosquitto client with config as userdata
    mosq = mosquitto_new(config.client_id.c_str(), true, &config);
    if (!mosq) {
        ERROR_LOG("Failed to create mosquitto client");
        return 1;
    }
    
    // Set authentication if provided
    if (!config.username.empty()) {
        int auth_result = mosquitto_username_pw_set(mosq, config.username.c_str(), 
                                                   config.password.empty() ? nullptr : config.password.c_str());
        if (auth_result != MOSQ_ERR_SUCCESS) {
            ERROR_LOG("Failed to set MQTT authentication: " << mosquitto_strerror(auth_result));
            mosquitto_destroy(mosq);
            mosquitto_lib_cleanup();
            return 1;
        }
    }
    
    if (config.ha_reporting) {
        // Use separate HA device ID if provided, otherwise fall back to client_id
        std::string ha_device_id = config.client_id;
        const char* env_ha_device_id = std::getenv("HA_DEVICE_ID");
        if (env_ha_device_id && strlen(env_ha_device_id) > 0) {
            ha_device_id = env_ha_device_id;
        }
        
        ha_discovery::HAConfig ha_config = {
            .device_id = ha_device_id,
            .topic_prefix = config.topic_prefix,
        };
        ha_manager = std::make_unique<ha_discovery::HADiscoveryManager>(ha_config);
    }

    // Set callbacks
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_message_callback_set(mosq, on_message);
    
    // Initial connection attempt
    LOG("Connecting to MQTT broker at " << config.host << ":" << config.port);
    int result = mosquitto_connect(mosq, config.host.c_str(), config.port, 60);
    if (result != MOSQ_ERR_SUCCESS) {
        ERROR_LOG("Initial connection failed: " << mosquitto_strerror(result));
        ERROR_LOG("Will continue trying to connect...");
    }

    // State callback is now handled by Display class directly
    sequence_manager = std::make_unique<sequence::SequenceManager>(std::move(display));
    
    // Notify systemd that we're ready
#ifdef __linux__
    systemd_notify("READY=1");
    DEBUG_LOG("Notified systemd that service is ready");
#endif
    
    // Main loop with exponential backoff reconnection
    auto last_watchdog = std::chrono::steady_clock::now();

    while (running) {
        int loop_result = mosquitto_loop(mosq, 100, 1);
        
        if (loop_result != MOSQ_ERR_SUCCESS) {
            if (mqtt_connected) {
                ERROR_LOG("MQTT loop error: " << mosquitto_strerror(loop_result));
                mqtt_connected = false;
                reconnect_delay = 1;
                last_connection_attempt = std::chrono::steady_clock::now();
            }
            
            // Try to reconnect with exponential backoff
            if (attempt_reconnect(mosq, config)) {
                LOG("Successfully reconnected to MQTT broker");
            }
        }
        
        // Send systemd watchdog notification every 15 seconds (half of WatchdogSec=30)
        // Also publish device state updates every 30 seconds
#ifdef __linux__
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_watchdog).count() >= 15) {
            systemd_notify("WATCHDOG=1");
            last_watchdog = now;
        }
#endif
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
        
    // Cleanup
    LOG("Shutting down...");
    
    // Publish offline availability before disconnecting
    if (ha_manager && mqtt_connected) {
        ha_manager->close(mosq);
    }
    
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    
    return 0;
}
