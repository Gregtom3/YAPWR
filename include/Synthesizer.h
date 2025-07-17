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

    /// After processing, combine all Results to compute global sys/stat errors,
    /// produce plots (via ROOT), export LaTeX tables, etc.
    void synthesizeFinal();

private:
    std::string projectDir_, pionPair_, runPeriod_;
    std::vector<Config> configs_;
    std::vector<std::string> moduleNames_ = {"asymmetryPW", "binMigration", "baryonContamination", "particleMisidentification",
                                             "kinematicBins"};
    std::map<std::string, std::vector<Result>> allResults_;
};