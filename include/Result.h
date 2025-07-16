#pragma once
#include <map>
#include <string>
#include "Logger.h"

/// Generic holder for module outputs (errors, histograms, tables…)
struct Result {
    std::string moduleName;
    std::map<std::string, double> scalars; // e.g. {"asymmetry": 0.0123}

    void print() const {
        LOG_INFO("=== Result for module: " + moduleName + " ===");
        for (const auto& [key, val] : scalars) {
            LOG_INFO("  " + key + " = " + std::to_string(val));
        }
    }
};