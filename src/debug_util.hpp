#ifndef debug_util_hpp
#define debug_util_hpp

#ifdef DEBUG
#define DEBUG_LOG_ACTIVE 1
#else
#define DEBUG_LOG_ACTIVE 0
#endif

#define DEBUG_LOG(...) \
    if (DEBUG_LOG_ACTIVE) { \
        std::cerr << __VA_ARGS__ << std::endl; \
    } while (0)

#endif