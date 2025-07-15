#pragma once
#include <iostream>
#define LOG_INFO(msg) (std::cout << "[INFO] " << msg << "\n")
#define LOG_WARN(msg) (std::cerr << "[WARN] " << msg << "\n")
#define LOG_ERROR(msg) (std::cerr << "[ERROR] " << msg << "\n")