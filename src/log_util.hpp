#ifndef log_util_hpp
#define log_util_hpp

#include <iostream>
#include <fstream>
#include <memory>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <string>

namespace debug {

enum class LogLevel {
    DEBUG_LEVEL = 0,
    INFO_LEVEL = 1,
    WARN_LEVEL = 2,
    ERROR_LEVEL = 3,
    OFF_LEVEL = 4
};

class Logger {
public:
    // Header-only static variables - shared across ALL compilation units
    static inline std::unique_ptr<std::ofstream> log_file;
    static inline std::mutex log_mutex;
    static inline bool use_file_logging = false;
    
    static LogLevel getCurrentLogLevel() {
        static LogLevel cached_level = LogLevel::OFF_LEVEL;
        static bool initialized = false;
        
        if (!initialized) {
            const char* env_level = std::getenv("RASPBERRY_DISPLAY_LOG_LEVEL");
            if (env_level) {
                std::string level_str(env_level);
                if (level_str == "DEBUG" || level_str == "debug") {
                    cached_level = LogLevel::DEBUG_LEVEL;
                } else if (level_str == "INFO" || level_str == "info") {
                    cached_level = LogLevel::INFO_LEVEL;
                } else if (level_str == "WARN" || level_str == "warn") {
                    cached_level = LogLevel::WARN_LEVEL;
                } else if (level_str == "ERROR" || level_str == "error") {
                    cached_level = LogLevel::ERROR_LEVEL;
                } else if (level_str == "OFF" || level_str == "off") {
                    cached_level = LogLevel::OFF_LEVEL;
                }
            } else {
                cached_level = LogLevel::INFO_LEVEL;
            }
            initialized = true;
        }
        
        return cached_level;
    }
    
    static bool shouldLog(LogLevel level) {
        return level >= getCurrentLogLevel();
    }

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
    static void writeMessage(const std::string& message, std::ostream& out_stream) {
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
            out_stream << timestamp.str() << ": " << message << std::endl;
        }
    }

public:
    // Debug message function (conditional on log level)
    static void writeDebugMessage(const std::string& message) {
        if (!shouldLog(LogLevel::DEBUG_LEVEL)) return;
        writeMessage("DEBUG: " + message, std::cerr);
    }

    // Info message function 
    static void writeInfoMessage(const std::string& message) {
        if (!shouldLog(LogLevel::INFO_LEVEL)) return;
        writeMessage("INFO: " + message, std::cout);
    }

    // Warning message function
    static void writeWarnMessage(const std::string& message) {
        if (!shouldLog(LogLevel::WARN_LEVEL)) return;
        writeMessage("WARN: " + message, std::cerr);
    }

    // Error message function
    static void writeErrorMessage(const std::string& message) {
        if (!shouldLog(LogLevel::ERROR_LEVEL)) return;
        writeMessage("ERROR: " + message, std::cerr);
    }

    // Regular log message function (maps to INFO level)
    static void writeLogMessage(const std::string& message) {
        writeInfoMessage(message);
    }
};

} // namespace debug

// Base logging macro - eliminates repetition
#define LOG_BASE(level, write_func, msg) \
    do { \
        if (debug::Logger::shouldLog(debug::LogLevel::level)) { \
            std::ostringstream log_stream; \
            log_stream << msg; \
            debug::Logger::write_func(log_stream.str()); \
        } \
    } while (0)

// Specific logging macros using the base
#define DEBUG_LOG(msg) LOG_BASE(DEBUG_LEVEL, writeDebugMessage, msg)
#define INFO_LOG(msg)  LOG_BASE(INFO_LEVEL, writeInfoMessage, msg)
#define WARN_LOG(msg)  LOG_BASE(WARN_LEVEL, writeWarnMessage, msg)
#define ERROR_LOG(msg) LOG_BASE(ERROR_LEVEL, writeErrorMessage, msg)

// Regular logging macro (maps to INFO level, backward compatible)
#define LOG(msg) INFO_LOG(msg)

#endif