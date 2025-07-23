#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "Config.h"
#include "Result.h"

namespace fs = std::filesystem;

class Synthesizer {
public:
    Synthesizer(const std::string& projectDir, const std::string& pionPair, const std::string& runPeriod);

    // Determine base directory containing yaml config
    fs::path findBaseDirectory();

    /// Discover all <CONFIG_NAME> subdirs under projectDir/pionPair/runPeriod
    void discoverConfigs();

    /// For each config, for each registered module:
    ///   1. construct module‑out path
    ///   2. call ModuleProcessorFactory::create(moduleName)->process(...)
    void runAll();

    std::map<std::string, std::map<std::string, Result>> getResults() {
        return allResults_;
    }
    std::map<std::string, Config> getConfigsMap() {
        return configs_map_;
    }

private:
    std::string projectDir_, pionPair_, runPeriod_;
    std::vector<Config> configs_;
    std::vector<std::string> moduleNames_ = {"asymmetryPW",   "binMigration",   "baryonContamination", "particleMisidentification",
                                             "kinematicBins", "sidebandRegion", "normalization"};
    std::map<std::string, std::map<std::string, Result>> allResults_;
    std::map<std::string, Config> configs_map_;
};