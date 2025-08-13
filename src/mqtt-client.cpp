#include <iostream>
#include <optional>
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
#include "transition.hpp"
#include "sequence.hpp"
#include "log_util.hpp"

using json = nlohmann::json;

// Global variables for signal handling
static bool running = true;
static display::Display* global_display = nullptr;
static struct mosquitto* mosq = nullptr;
static std::unique_ptr<sequence::SequenceManager> sequence_manager;

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
};

static void signal_handler(int signal) {
    LOG("Received signal " << signal << ", shutting down gracefully...");
    running = false;
    if (mosq) {
        mosquitto_disconnect(mosq);
    }
    if (global_display) {
        global_display->stop();
    }
}

static void process_add_sequence(const json& message) {
    try {
        if (!message.contains("state") || !message.contains("time")) {
            LOG("addSequence requires 'state' and 'time' fields");
            return;
        }
        
        double ttl = message.contains("ttl") ? message["ttl"].get<double>() : 0.0;
        double time = message["time"].get<double>();
        json state_json = message["state"];
        std::string sequence_id = message.contains("sequence_id") ? message["sequence_id"].get<std::string>() : "";

        // Parse JSON to DisplayState
        sequence::DisplayState state = sequence::parseDisplayStateFromJSON(state_json);
        
        if (sequence_manager) {
            sequence_manager->addSequenceState(state, time, ttl, sequence_id);
            DEBUG_LOG("Added state to sequence with time=" << time << "s, ttl=" << ttl << "s, sequence_id='" << sequence_id << "'");
        }
        
    } catch (const std::exception& e) {
        LOG("Error processing addSequence: " << e.what());
    }
}

static void process_set_sequence(const json& message) {
    try {
        if (!message.is_array()) {
            LOG("setSequence requires an array of sequence states");
            return;
        }
        
        std::vector<sequence::SequenceState> sequence_states;
        
        for (const auto& item : message) {
            if (!item.contains("state") || !item.contains("time")) {
                LOG("Each sequence item requires 'state' and 'time' fields");
                continue;
            }
            
            double ttl = item.contains("ttl") ? item["ttl"].get<double>() : 0.0;
            double time = item["time"].get<double>();
            json state_json = item["state"];
            std::string sequence_id = item.contains("sequence_id") ? item["sequence_id"].get<std::string>() : "";
            
            // Parse JSON to DisplayState
            sequence::DisplayState state = sequence::parseDisplayStateFromJSON(state_json);
            
            // Note: created_at will be set by SequenceManager
            sequence_states.push_back({state, time, ttl, std::chrono::steady_clock::now(), sequence_id});
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
            sequence_manager->clearSequence();
            DEBUG_LOG("Cleared all sequences");
        }
        
    } catch (const std::exception& e) {
        LOG("Error processing clearSequence: " << e.what());
    }
}

static void on_message(struct mosquitto* /*mosq*/, void* /*userdata*/, const struct mosquitto_message* message) {
    if (!message->payload) return;
    
    std::string topic(message->topic);
    std::string payload(static_cast<char*>(message->payload), static_cast<size_t>(message->payloadlen));
    
    DEBUG_LOG("Received MQTT message on topic: " << topic << " with payload: " << payload);
    
    try {
        json message_json = json::parse(payload);
        
        if (topic == "display/addSequence") {
            process_add_sequence(message_json);
        } else if (topic == "display/setSequence") {
            process_set_sequence(message_json);
        } else if (topic == "display/clearSequence") {
            process_clear_sequence(message_json);
        } else if (topic == "display/quit") {
            DEBUG_LOG("Received quit message");
            running = false;
        } else {
            DEBUG_LOG("Unknown topic: " << topic);
        }
    } catch (const json::parse_error& e) {
        LOG("JSON parse error: " << e.what());
        LOG("Payload: " << payload);
    }
}

static void on_connect(struct mosquitto* mosq, void* userdata, int result) {
    if (result == 0) {
        LOG("Connected to MQTT broker successfully");
        mqtt_connected = true;
        reconnect_delay = 1; // Reset backoff on successful connection
        
        // Get topic prefix from userdata
        MqttConfig* config = static_cast<MqttConfig*>(userdata);
        std::string prefix = config->topic_prefix;
        
        // Subscribe to topics with configurable prefix
        mosquitto_subscribe(mosq, nullptr, (prefix + "/addSequence").c_str(), 0);
        mosquitto_subscribe(mosq, nullptr, (prefix + "/setSequence").c_str(), 0);
        mosquitto_subscribe(mosq, nullptr, (prefix + "/clearSequence").c_str(), 0);
        mosquitto_subscribe(mosq, nullptr, (prefix + "/quit").c_str(), 0);
        
        LOG("Subscribed to " << prefix << " topics (addSequence, setSequence, clearSequence, quit)");
    } else {
        LOG("Failed to connect to MQTT broker: " << mosquitto_connack_string(result));
        mqtt_connected = false;
    }
}

static void on_disconnect(struct mosquitto* /*mosq*/, void* /*userdata*/, int result) {
    mqtt_connected = false;
    if (result != 0) {
        LOG("Unexpected disconnection from MQTT broker");
    } else {
        LOG("Disconnected from MQTT broker");
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
    
    // Apply environment variables
    if (env_host) config.host = env_host;
    if (env_port) config.port = std::stoi(env_port);
    if (env_username) config.username = env_username;
    if (env_password) config.password = env_password;
    if (env_client_id) config.client_id = env_client_id;
    if (env_topic_prefix) config.topic_prefix = env_topic_prefix;
    
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
        LOG("Error: MQTT host not specified");
        LOG("");
        print_usage(argv[0]);
        return 1;
    }
    
    LOG("MQTT Configuration:");
    LOG("  Host: " << config.host);
    LOG("  Port: " << config.port);
    LOG("  Client ID: " << config.client_id);
    LOG("  Topic Prefix: " << config.topic_prefix);
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
    auto display = std::make_unique<display::DisplayImpl>(preUpdate, postUpdate);
    global_display = display.get(); // Keep pointer for signal handling
    
    // Initialize mosquitto library
    mosquitto_lib_init();
    
    // Create mosquitto client with config as userdata
    mosq = mosquitto_new(config.client_id.c_str(), true, &config);
    if (!mosq) {
        LOG("Failed to create mosquitto client");
        return 1;
    }
    
    // Set authentication if provided
    if (!config.username.empty()) {
        int auth_result = mosquitto_username_pw_set(mosq, config.username.c_str(), 
                                                   config.password.empty() ? nullptr : config.password.c_str());
        if (auth_result != MOSQ_ERR_SUCCESS) {
            LOG("Failed to set MQTT authentication: " << mosquitto_strerror(auth_result));
            mosquitto_destroy(mosq);
            mosquitto_lib_cleanup();
            return 1;
        }
    }
    
    // Set callbacks
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_message_callback_set(mosq, on_message);
    
    // Initial connection attempt
    LOG("Connecting to MQTT broker at " << config.host << ":" << config.port);
    int result = mosquitto_connect(mosq, config.host.c_str(), config.port, 60);
    if (result != MOSQ_ERR_SUCCESS) {
        LOG("Initial connection failed: " << mosquitto_strerror(result));
        LOG("Will continue trying to connect...");
    }
    
    // Initialize sequence manager with display ownership
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
                LOG("MQTT loop error: " << mosquitto_strerror(loop_result));
                mqtt_connected = false;
                reconnect_delay = 1;
                last_connection_attempt = std::chrono::steady_clock::now();
            }
            
            // Try to reconnect with exponential backoff
            if (attempt_reconnect(mosq, config)) {
                DEBUG_LOG("Successfully reconnected to MQTT broker");
            }
        }
        
        // Send systemd watchdog notification every 15 seconds (half of WatchdogSec=30)
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
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    
    return 0;
}
