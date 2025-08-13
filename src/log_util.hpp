#ifndef log_util_hpp
#define log_util_hpp

#include <iostream>
#include <fstream>
#include <memory>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef DEBUG
#define DEBUG_LOG_ACTIVE 1
#else
#define DEBUG_LOG_ACTIVE 0
#endif

namespace debug {

class Logger {
public:
    // Header-only static variables - shared across ALL compilation units
    static inline std::unique_ptr<std::ofstream> log_file;
    static inline std::mutex log_mutex;
    static inline bool use_file_logging = false;

    static void enableFileLogging(const std::string& filename) {
        std::cerr << "Enabling file logging to " << filename << std::endl;
        std::lock_guard<std::mutex> lock(log_mutex);
        log_file = std::make_unique<std::ofstream>(filename, std::ios::app);
        if (log_file && log_file->is_open()) {
            use_file_logging = true;
        } else {
            std::cout << "ERROR: Failed to open log file " << filename << std::endl;
            use_file_logging = false;
        }
    }
    
    static void disableFileLogging() {
        std::lock_guard<std::mutex> lock(log_mutex);
        if (log_file) {
            log_file->close();
            log_file.reset();
        }
        use_file_logging = false;
    }

private:
    // Unified internal logging function
    static void writeMessage(const std::string& message, bool use_stdout_fallback = false) {
        std::lock_guard<std::mutex> lock(log_mutex);
        
        // Get timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::ostringstream timestamp;
        timestamp << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        timestamp << '.' << std::setfill('0') << std::setw(3) << ms.count();
        
        // Write to appropriate destination
        if (use_file_logging && log_file && log_file->is_open()) {
            *log_file << timestamp.str() << ": " << message << std::endl;
            log_file->flush();
        } else {
            if (use_stdout_fallback) {
                std::cout << timestamp.str() << ": " << message << std::endl;
            } else {
                std::cerr << timestamp.str() << ": " << message << std::endl;
            }
        }
    }

public:
    // Debug message function (conditional on DEBUG_LOG_ACTIVE)
    static void writeDebugMessage(const std::string& message) {
        if (!DEBUG_LOG_ACTIVE) return;
        writeMessage("DEBUG: " + message, false);
    }

    // Regular log message function (always active)
    static void writeLogMessage(const std::string& message) {
        writeMessage(message, true);
    }
};

} // namespace debug

// Simplified macro that creates a stringstream and calls a single function
#define DEBUG_LOG(msg) \
    do { \
        if (DEBUG_LOG_ACTIVE) { \
            std::ostringstream debug_stream; \
            debug_stream << msg; \
            debug::Logger::writeDebugMessage(debug_stream.str()); \
        } \
    } while (0)

// Regular logging macro (always active)
#define LOG(msg) \
    do { \
        std::ostringstream log_stream; \
        log_stream << msg; \
        debug::Logger::writeLogMessage(log_stream.str()); \
    } while (0)

#endif