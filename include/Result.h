#pragma once
#include "Logger.h"
#include <map>
#include <string>

/// Generic holder for module outputs (errors, histograms, tables…)
struct Result {
    std::string moduleName;
    std::map<std::string, double> scalars; // e.g. {"asymmetry": 0.0123}

    void print(bool force = false) const {
        if (force == true) {
            LOG_PRINT("=== Result for module: " + moduleName + " ===");
            for (const auto& [key, val] : scalars) {
                LOG_PRINT("  " + key + " = " + std::to_string(val));
            }
        } else {
            LOG_DEBUG("=== Result for module: " + moduleName + " ===");
            for (const auto& [key, val] : scalars) {
                LOG_DEBUG("  " + key + " = " + std::to_string(val));
            }
        }
    }
};