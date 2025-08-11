#include <iostream>
#include <optional>
#include <string>
#include <signal.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <mosquitto.h>
#include <nlohmann/json.hpp>

#include "display_impl.hpp"
#include "transition.hpp"
#include "sequence.hpp"
#include "debug_util.hpp"

using json = nlohmann::json;

// Global variables for signal handling
static bool running = true;
static display::Display* global_display = nullptr;
static struct mosquitto* mosq = nullptr;
static std::unique_ptr<sequence::SequenceManager> sequence_manager;

static void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    running = false;
    if (global_display) {
        global_display->stop();
    }
    if (mosq) {
        mosquitto_disconnect(mosq);
    }
}

static void process_set(const json& message) {
    if (sequence_manager) {
        sequence_manager->clearSequence();

        sequence::DisplayState state = sequence::parseDisplayStateFromJSON(message);
        sequence_manager->addSequenceState(state, 5, 0.0, "display_set");
    }
}

static void process_add_sequence(const json& message) {
    try {
        if (!message.contains("state") || !message.contains("time")) {
            std::cerr << "addSequence requires 'state' and 'time' fields" << std::endl;
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
        std::cerr << "Error processing addSequence: " << e.what() << std::endl;
    }
}

static void process_set_sequence(const json& message) {
    try {
        if (!message.is_array()) {
            std::cerr << "setSequence requires an array of sequence states" << std::endl;
            return;
        }
        
        std::vector<sequence::SequenceState> sequence_states;
        
        for (const auto& item : message) {
            if (!item.contains("state") || !item.contains("time")) {
                std::cerr << "Each sequence item requires 'state' and 'time' fields" << std::endl;
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
            DEBUG_LOG("Set sequence with " << sequence_states.size() << " states");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing setSequence: " << e.what() << std::endl;
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
        std::cerr << "Error processing clearSequence: " << e.what() << std::endl;
    }
}

static void on_message(struct mosquitto* /*mosq*/, void* /*userdata*/, const struct mosquitto_message* message) {
    if (!message->payload) return;
    
    std::string topic(message->topic);
    std::string payload(static_cast<char*>(message->payload), static_cast<size_t>(message->payloadlen));
    
    DEBUG_LOG("Received MQTT message on topic: " << topic << " with payload: " << payload);
    
    try {
        json message_json = json::parse(payload);
        
        if (topic == "display/set") {
            process_set(message_json);
        } else if (topic == "display/addSequence") {
            process_add_sequence(message_json);
        } else if (topic == "display/setSequence") {
            process_set_sequence(message_json);
        } else if (topic == "display/clearSequence") {
            process_clear_sequence(message_json);
        } else if (topic == "display/quit") {
            running = false;
        } else {
            DEBUG_LOG("Unknown topic: " << topic);
        }
        
    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        std::cerr << "Payload: " << payload << std::endl;
    }
}

static void on_connect(struct mosquitto* mosq, void* /*userdata*/, int result) {
    if (result == 0) {
        std::cout << "Connected to MQTT broker successfully" << std::endl;
        
        // Subscribe to topics
        mosquitto_subscribe(mosq, nullptr, "display/set", 0);
        mosquitto_subscribe(mosq, nullptr, "display/addSequence", 0);
        mosquitto_subscribe(mosq, nullptr, "display/setSequence", 0);
        mosquitto_subscribe(mosq, nullptr, "display/clearSequence", 0);
        mosquitto_subscribe(mosq, nullptr, "display/quit", 0);
        
        std::cout << "Subscribed to display topics (set, addSequence, setSequence, clearSequence, quit)" << std::endl;
    } else {
        std::cerr << "Failed to connect to MQTT broker: " << mosquitto_connack_string(result) << std::endl;
    }
}

static void on_disconnect(struct mosquitto* /*mosq*/, void* /*userdata*/, int result) {
    if (result != 0) {
        std::cerr << "Unexpected disconnection from MQTT broker" << std::endl;
    } else {
        std::cout << "Disconnected from MQTT broker" << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <mqtt_host> <mqtt_port> [client_id] [topic_prefix]" << std::endl;
        return 1;
    }
    
    // Parse command line arguments
    std::string mqtt_host = argv[1];
    int mqtt_port = std::stoi(argv[2]);
    std::string client_id = (argc > 3) ? argv[3] : "raspberry-display";
    std::string topic_prefix = (argc > 4) ? argv[4] : "display";
    
    // Environment variable overrides
    const char* env_host = std::getenv("MQTT_HOST");
    const char* env_port = std::getenv("MQTT_PORT");
    const char* env_client_id = std::getenv("CLIENT_ID");
    const char* env_topic_prefix = std::getenv("TOPIC_PREFIX");
    
    if (env_host) mqtt_host = env_host;
    if (env_port) mqtt_port = std::stoi(env_port);
    if (env_client_id) client_id = env_client_id;
    if (env_topic_prefix) topic_prefix = env_topic_prefix;
    
    std::cout << "MQTT Configuration:" << std::endl;
    std::cout << "  Host: " << mqtt_host << std::endl;
    std::cout << "  Port: " << mqtt_port << std::endl;
    std::cout << "  Client ID: " << client_id << std::endl;
    std::cout << "  Topic Prefix: " << topic_prefix << std::endl;
    
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
    
    // Create mosquitto client (no longer needs display userdata)
    mosq = mosquitto_new(client_id.c_str(), true, nullptr);
    if (!mosq) {
        std::cerr << "Failed to create mosquitto client" << std::endl;
        return 1;
    }
    
    // Set callbacks
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_message_callback_set(mosq, on_message);
    
    // Connect to broker
    std::cout << "Connecting to MQTT broker at " << mqtt_host << ":" << mqtt_port << std::endl;
    int result = mosquitto_connect(mosq, mqtt_host.c_str(), mqtt_port, 60);
    if (result != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to connect to MQTT broker: " << mosquitto_strerror(result) << std::endl;
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }
    
    // Initialize sequence manager with display ownership
    sequence_manager = std::make_unique<sequence::SequenceManager>(std::move(display));
    
    // Main loop
    while (running) {
        int loop_result = mosquitto_loop(mosq, 100, 1);
        if (loop_result != MOSQ_ERR_SUCCESS) {
            std::cerr << "MQTT loop error: " << mosquitto_strerror(loop_result) << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            std::cout << "Attempting to reconnect..." << std::endl;
            mosquitto_reconnect(mosq);
        }
    }
    
    // Cleanup
    std::cout << "Shutting down..." << std::endl;
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    
    return 0;
}
