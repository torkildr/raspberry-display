#include <iostream>
#include <string>
#include <signal.h>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <mosquitto.h>
#include <nlohmann/json.hpp>

#include "display_impl.hpp"
#include "debug_util.hpp"

using json = nlohmann::json;

// Global variables for signal handling
static bool running = true;
static display::Display* global_display = nullptr;
static struct mosquitto* mosq = nullptr;

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

static void process_display_state(display::Display* disp, const json& state) {
    DEBUG_LOG("Processing display state: " << state.dump());
    
    try {
        // Handle display content
        if (state.contains("clear") && state["clear"].get<bool>()) {
            disp->show("");
        }
        else if (state.contains("text")) {
            std::string text = state["text"];
            
            if (state.contains("show_time") && state["show_time"].get<bool>()) {
                std::string time_format = state.contains("time_format") ? 
                    state["time_format"].get<std::string>() : "";
                disp->showTime(time_format, text);
            } else {
                disp->show(text);
            }
        }
        else if (state.contains("show_time") && state["show_time"].get<bool>()) {
            std::string time_format = state.contains("time_format") ? 
                state["time_format"].get<std::string>() : "";
            disp->showTime(time_format);
        }
        
        // Handle brightness
        if (state.contains("brightness")) {
            int brightness = state["brightness"];
            if (brightness >= 0 && brightness <= 15) {
                disp->setBrightness(brightness);
            }
        }
        
        // Handle alignment
        if (state.contains("alignment")) {
            std::string alignment = state["alignment"];
            if (alignment == "center" || alignment == "centre") {
                disp->setAlignment(display::Alignment::CENTER);
                if (!state.contains("scroll")) {
                    disp->setScrolling(display::Scrolling::DISABLED);
                }
            } else if (alignment == "left") {
                disp->setAlignment(display::Alignment::LEFT);
            }
        }
        
        // Handle scrolling
        if (state.contains("scroll")) {
            std::string scroll = state["scroll"];
            if (scroll == "enabled") {
                disp->setScrolling(display::Scrolling::ENABLED);
            } else if (scroll == "disabled") {
                disp->setScrolling(display::Scrolling::DISABLED);
            } else if (scroll == "reset") {
                // Reset scroll offset - this is handled internally by setScrolling
                disp->setScrolling(display::Scrolling::ENABLED);
            }
        }
        
        // Handle quit
        if (state.contains("quit") && state["quit"].get<bool>()) {
            running = false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing display state: " << e.what() << std::endl;
    }
}

static void on_message(struct mosquitto* /*mosq*/, void* userdata, const struct mosquitto_message* message) {
    display::Display* disp = static_cast<display::Display*>(userdata);
    
    if (!message->payload) return;
    
    std::string topic(message->topic);
    std::string payload(static_cast<char*>(message->payload), static_cast<size_t>(message->payloadlen));
    
    DEBUG_LOG("Received MQTT message on topic: " << topic << " with payload: " << payload);
    
    try {
        // Try to parse as JSON first
        json state = json::parse(payload);
        process_display_state(disp, state);
    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        std::cerr << "Payload: " << payload << std::endl;
    }
}

static void on_connect(struct mosquitto* mosq, void* /*userdata*/, int result) {
    if (result == 0) {
        std::cout << "Connected to MQTT broker successfully" << std::endl;
        
        // Subscribe to topics
        mosquitto_subscribe(mosq, nullptr, "display/state", 0);
        mosquitto_subscribe(mosq, nullptr, "display", 0);  // Alternative topic
        
        std::cout << "Subscribed to display topics" << std::endl;
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
    
    // Initialize display with empty callbacks
    auto preUpdate = []() {};
    auto postUpdate = []() {};
    display::DisplayImpl disp(preUpdate, postUpdate);
    global_display = &disp;
    disp.start();
    
    // Initialize mosquitto library
    mosquitto_lib_init();
    
    // Create mosquitto client
    mosq = mosquitto_new(client_id.c_str(), true, &disp);
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
    
    // Main loop
    while (running) {
        int loop_result = mosquitto_loop(mosq, 100, 1);
        if (loop_result != MOSQ_ERR_SUCCESS) {
            std::cerr << "MQTT loop error: " << mosquitto_strerror(loop_result) << std::endl;
            if (loop_result == MOSQ_ERR_CONN_LOST) {
                std::cout << "Connection lost, attempting to reconnect..." << std::endl;
                mosquitto_reconnect(mosq);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Cleanup
    std::cout << "Shutting down..." << std::endl;
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    
    return 0;
}
