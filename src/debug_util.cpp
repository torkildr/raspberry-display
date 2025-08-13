#include "debug_util.hpp"

namespace debug {

// Static member definitions
std::unique_ptr<std::ofstream> Logger::log_file;
std::mutex Logger::log_mutex;
bool Logger::use_file_logging = false;

} // namespace debug
