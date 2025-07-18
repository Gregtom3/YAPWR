#pragma once
#include <iostream>
#include <stdexcept>

namespace Logger {

enum class Level { Error = 0, Warn = 1, Info = 2, Debug = 3 };
constexpr bool FORCE = true;
inline std::string makeBar(size_t length, char fill = '=') {
    return std::string(length, fill);
}
constexpr char RESET_COLOR[] = "\x1b[0m";
constexpr char RED_COLOR[] = "\x1b[31m";
constexpr char GREEN_COLOR[] = "\x1b[32m";
constexpr char YELLOW_COLOR[] = "\x1b[33m";
constexpr char BLUE_COLOR[] = "\x1b[34m";

// default at Info
inline Level& currentLevel() {
    static Level lvl = Level::Info;
    return lvl;
}

inline void setLevel(Level lvl) {
    currentLevel() = lvl;
}
} // namespace Logger

#define LOG_PRINT(msg)                                                                           \
    do {                                                                                         \
        if (true)                                                                                \
            std::cout << Logger::BLUE_COLOR << "[PRINT] " << Logger::RESET_COLOR << msg << "\n"; \
    } while (0)

#define LOG_DEBUG(msg)                                                                           \
    do {                                                                                         \
        if (Logger::currentLevel() >= Logger::Level::Debug)                                      \
            std::cout << Logger::BLUE_COLOR << "[DEBUG] " << Logger::RESET_COLOR << msg << "\n"; \
    } while (0)

#define LOG_INFO(msg)                                                                            \
    do {                                                                                         \
        if (Logger::currentLevel() >= Logger::Level::Info)                                       \
            std::cout << Logger::GREEN_COLOR << "[INFO] " << Logger::RESET_COLOR << msg << "\n"; \
    } while (0)

#define LOG_WARN(msg)                                                                             \
    do {                                                                                          \
        if (Logger::currentLevel() >= Logger::Level::Warn)                                        \
            std::cerr << Logger::YELLOW_COLOR << "[WARN] " << Logger::RESET_COLOR << msg << "\n"; \
    } while (0)

#define LOG_ERROR(msg)                                                                          \
    do {                                                                                        \
        if (Logger::currentLevel() >= Logger::Level::Error)                                     \
            std::cerr << Logger::RED_COLOR << "[ERROR] " << msg << Logger::RESET_COLOR << "\n"; \
    } while (0)

#define LOG_FATAL(msg)                                                                      \
    do { /* always fire */                                                                  \
        std::cerr << Logger::RED_COLOR << "[FATAL] " << msg << Logger::RESET_COLOR << "\n"; \
        throw std::runtime_error(std::string("[FATAL] ") + msg);                            \
    } while (0)
