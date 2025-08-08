#include <iostream>
#include <string>
#include <sstream>
#include <signal.h>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <nlohmann/json.hpp>

#include <mqtt_client_cpp.hpp>

#include "display_impl.hpp"
#include "debug_util.hpp"

using json = nlohmann::json;

// Global variables for signal handling
static bool running = true;
static display::Display* global_display = nullptr;
static std::shared_ptr<MQTT_NS::callable_overlay<MQTT_NS::sync_client<MQTT_NS::tcp_endpoint<boost::asio::ip::tcp::socket, boost::asio::io_context::strand>>>> mqtt_client;

static void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    running = false;
    if (global_display) {
        global_display->stop();
    }
    if (mqtt_client) {
        mqtt_client->disconnect();
    }
}

static void process_display_state(display::Display* disp, const json& state) {
    DEBUG_LOG("Processing display state: " << state.dump());
    
    try {
        // Handle display content
        if (state.contains("clear") && state["clear"].get<bool>()) {
            // Clear display
            disp->show("");
        }
        else if (state.contains("text")) {
            // Text display (with optional time)
            std::string text = state["text"];
            
            if (state.contains("show_time") && state["show_time"].get<bool>()) {
                // Show text with time
                std::string time_format = state.contains("time_format") ? 
                    state["time_format"].get<std::string>() : "%H:%M";
                disp->showTime(time_format, text);
            } else {
                // Show text only
                disp->show(text);
            }
        }
        else if (state.contains("show_time") && state["show_time"].get<bool>()) {
            // Time-only display (no text field present)
            std::string format = state.contains("time_format") ? 
                state["time_format"].get<std::string>() : "%H:%M:%S";
            disp->showTime(format);
        }
        
        // Handle scrolling
        if (state.contains("scroll")) {
            std::string scroll_mode = state["scroll"];
            if (scroll_mode == "enabled" || scroll_mode == "left" || scroll_mode == "auto") {
                disp->setScrolling(display::Scrolling::ENABLED);
            }
            else if (scroll_mode == "disabled" || scroll_mode == "none" || scroll_mode == "off") {
                disp->setScrolling(display::Scrolling::DISABLED);
            }
            else if (scroll_mode == "reset") {
                disp->setScrolling(display::Scrolling::RESET);
            }
        }
        
        // Handle brightness
        if (state.contains("brightness")) {
            int brightness = state["brightness"].get<int>();
            if (brightness >= 0 && brightness <= 15) {
                disp->setBrightness(brightness);
            } else {
                std::cerr << "Brightness must be between 0 and 15, got: " << brightness << std::endl;
            }
        }
        
        // Handle quit command
        if (state.contains("quit") && state["quit"].get<bool>()) {
            running = false;
            disp->stop();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing display state: " << e.what() << std::endl;
        std::cerr << "State was: " << state.dump() << std::endl;
    }
}

static void process_mqtt_message(display::Display* disp, const std::string& topic, const std::string& payload) {
    DEBUG_LOG("Received MQTT message on topic '" << topic << "': " << payload);
    
    // New state-based protocol: only accept JSON on the main display topic
    if (topic.find("/state") != std::string::npos || topic.find("/display") != std::string::npos) {
        try {
            json state = json::parse(payload);
            process_display_state(disp, state);
        } catch (const json::parse_error& e) {
            std::cerr << "Invalid JSON payload on topic " << topic << ": " << e.what() << std::endl;
            std::cerr << "Payload was: " << payload << std::endl;
        }
    } else {
        std::cerr << "Unsupported topic: " << topic << ". Please use '/state' or '/display' topic with JSON payload." << std::endl;
        std::cerr << "Example: mosquitto_pub -t 'display/state' -m '{\"text\":\"Hello World\", \"brightness\":10}'" << std::endl;
    }
}

int main(int argc, char** argv) {
    std::string mqtt_host;
    std::string mqtt_port;
    std::string client_id;
    std::string topic_prefix;
    
    // Support both command line arguments and environment variables
    // Environment variables provide fallback for systemd service
    if (argc >= 3) {
        // Command line arguments take precedence
        mqtt_host = argv[1];
        mqtt_port = argv[2];
        client_id = (argc > 3) ? argv[3] : "raspberry-display-mqtt";
        topic_prefix = (argc > 4) ? argv[4] : "display";
    } else {
        // Fall back to environment variables (for systemd service)
        const char* env_host = std::getenv("MQTT_HOST");
        const char* env_port = std::getenv("MQTT_PORT");
        const char* env_client_id = std::getenv("CLIENT_ID");
        const char* env_topic_prefix = std::getenv("TOPIC_PREFIX");
        
        if (!env_host || !env_port) {
            std::cout << "Usage: " << argv[0] << " <mqtt_host> <mqtt_port> [client_id] [topic_prefix]" << std::endl;
            std::cout << "Or set environment variables: MQTT_HOST, MQTT_PORT, CLIENT_ID, TOPIC_PREFIX" << std::endl;
            std::cout << "Example: " << argv[0] << " localhost 1883 raspberry-display display" << std::endl;
            return -1;
        }
        
        mqtt_host = env_host;
        mqtt_port = env_port;
        client_id = env_client_id ? env_client_id : "raspberry-display-mqtt";
        topic_prefix = env_topic_prefix ? env_topic_prefix : "display";
    }

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize display
    auto preUpdate = []() {};
    auto postUpdate = []() {};
    
    auto disp = std::make_unique<display::DisplayImpl>(preUpdate, postUpdate);
    global_display = disp.get();
    
    disp->start();
    std::cout << "Display initialized and started" << std::endl;

    // Setup MQTT
    MQTT_NS::setup_log();
    boost::asio::io_context ioc;
    
    try {
        // Create MQTT client
        mqtt_client = MQTT_NS::make_sync_client(ioc, mqtt_host, mqtt_port);
        
        // Setup client properties
        mqtt_client->set_client_id(client_id);
        mqtt_client->set_clean_session(true);
        
        std::cout << "Connecting to MQTT broker at " << mqtt_host << ":" << mqtt_port 
                  << " with client ID: " << client_id << std::endl;
        
        // Setup connection handler
        mqtt_client->set_connack_handler(
            [&topic_prefix](bool session_present, MQTT_NS::connect_return_code return_code) {
                std::cout << "MQTT connected. Session present: " << std::boolalpha << session_present << std::endl;
                std::cout << "Connection return code: " << MQTT_NS::connect_return_code_to_str(return_code) << std::endl;
                
                if (return_code == MQTT_NS::connect_return_code::accepted) {
                    // Subscribe to state-based topic
                    std::string state_topic = topic_prefix + "/state";
                    mqtt_client->subscribe(state_topic, MQTT_NS::qos::at_least_once);
                    std::cout << "Subscribed to state topic: " << state_topic << std::endl;
                    
                    // Also subscribe to generic display topic for compatibility
                    std::string display_topic = topic_prefix + "/display";
                    mqtt_client->subscribe(display_topic, MQTT_NS::qos::at_least_once);
                    std::cout << "Subscribed to display topic: " << display_topic << std::endl;
                }
                return true;
            }
        );
        
        // Setup message handler
        using packet_id_t = typename std::remove_reference_t<decltype(*mqtt_client)>::packet_id_t;
        mqtt_client->set_publish_handler(
            [disp = disp.get()](
                MQTT_NS::optional<packet_id_t> /* packet_id */,
                MQTT_NS::publish_options /* pubopts */,
                MQTT_NS::buffer topic_name,
                MQTT_NS::buffer contents
            ) {
                std::string topic = topic_name.to_string();
                std::string payload = contents.to_string();
                
                process_mqtt_message(disp, topic, payload);
                return true;
            }
        );
        
        // Setup disconnect handler
        mqtt_client->set_close_handler([]() {
            std::cout << "MQTT connection closed" << std::endl;
        });
        
        // Setup error handler
        mqtt_client->set_error_handler([](MQTT_NS::error_code ec) {
            std::cerr << "MQTT error: " << ec.message() << std::endl;
        });
        
        // Connect to MQTT broker
        mqtt_client->connect();
        
        std::cout << "MQTT client started. Using state-based protocol on topic: " << topic_prefix << "/state" << std::endl;
        std::cout << "Send JSON state messages to control the display. Press Ctrl+C to quit." << std::endl;
        std::cout << "Example: mosquitto_pub -t '" << topic_prefix << "/state' -m '{\"text\":\"Hello\", \"brightness\":10}'" << std::endl;
        
        // Run the IO context in a separate thread
        std::thread mqtt_thread([&ioc]() {
            ioc.run();
        });
        
        // Main loop - keep the application running
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Cleanup
        std::cout << "Shutting down..." << std::endl;
        
        if (mqtt_client) {
            mqtt_client->disconnect();
        }
        
        ioc.stop();
        if (mqtt_thread.joinable()) {
            mqtt_thread.join();
        }
        
        if (disp) {
            disp->stop();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    std::cout << "MQTT client stopped" << std::endl;
    return 0;
}

