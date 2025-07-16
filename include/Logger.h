#pragma once
#include <iostream>
#include <stdexcept>

namespace Logger {

enum class Level { Error = 0, Warn = 1, Info = 2, Debug = 3 };

// default at Info
inline Level& currentLevel() {
    static Level lvl = Level::Info;
    return lvl;
}

inline void setLevel(Level lvl) {
    currentLevel() = lvl;
}
} // namespace Logger
#define LOG_DEBUG(msg)                                      \
    do {                                                    \
        if (Logger::currentLevel() >= Logger::Level::Debug) \
            std::cout << "[DEBUG] " << msg << "\n";         \
    } while (0)

#define LOG_INFO(msg)                                      \
    do {                                                   \
        if (Logger::currentLevel() >= Logger::Level::Info) \
            std::cout << "[INFO] " << msg << "\n";         \
    } while (0)

#define LOG_WARN(msg)                                      \
    do {                                                   \
        if (Logger::currentLevel() >= Logger::Level::Warn) \
            std::cerr << "[WARN] " << msg << "\n";         \
    } while (0)

#define LOG_ERROR(msg)                                      \
    do {                                                    \
        if (Logger::currentLevel() >= Logger::Level::Error) \
            std::cerr << "[ERROR] " << msg << "\n";         \
    } while (0)

#define LOG_FATAL(msg)                                           \
    do { /* always fire */                                       \
        std::cerr << "[FATAL] " << msg << "\n";                  \
        throw std::runtime_error(std::string("[FATAL] ") + msg); \
    } while (0)
