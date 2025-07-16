#include "Synthesizer.h"

#include <iostream>

#include "Logger.h"
#include "ModuleProcessorFactory.h"

Synthesizer::Synthesizer(const std::string& pd, const std::string& pp, const std::string& rp)
    : projectDir_(pd)
    , pionPair_(pp)
    , runPeriod_(rp) {}

fs::path Synthesizer::findBaseDirectory() {
    std::vector<fs::path> candidates = {fs::path(projectDir_), fs::path("out") / projectDir_, fs::path("..") / "out" / projectDir_};
    // Pick the first one that exists
    fs::path base;
    for (auto& p : candidates) {
        if (fs::exists(p) && fs::is_directory(p)) {
            base = p;
            break;
        }
    }

    // If none exist, give a clear error
    if (base.empty()) {
        LOG_ERROR("Synthesizer::discoverConfigs(): none of the candidate base directories exist:\n"
                  "  " +
                  candidates[0].string() +
                  "\n"
                  "  " +
                  candidates[1].string() +
                  "\n"
                  "  " +
                  candidates[2].string());
    }
    return base;
}

void Synthesizer::discoverConfigs() {
    // Determine base directory containing the config file
    fs::path config_base = findBaseDirectory();

    // Scan each subdirectory for its YAML and load it
    for (auto& entry : fs::directory_iterator(config_base)) {
        if (!entry.is_directory())
            continue;

        // Get the folder name, e.g. "config_x0.1-0.3"
        std::string folderName = entry.path().filename().string();

        // If it starts with "config_", remove that prefix
        const std::string prefix = "config_";
        if (folderName.rfind(prefix, 0) == 0) {
            folderName = folderName.substr(prefix.size());
        }

        // Now yamlPath becomes ".../x0.1-0.3.yaml" instead of ".../config_x0.1-0.3.yaml"
        fs::path yamlPath = entry.path() / (folderName + ".yaml");
        if (!fs::exists(yamlPath)) {
            LOG_WARN("No config YAML for " + entry.path().filename().string() + " (looking for " + yamlPath.string() + ")");
            continue;
        }

        configs_.push_back(Config(yamlPath.string()));
    }

    // Ensure we found at least one valid config
    if (configs_.empty()) {
        throw std::runtime_error("Synthesizer::discoverConfigs(): no valid config directories found under " + config_base.string());
    }
    LOG_INFO("Found " << configs_.size() << " configs");
}

void Synthesizer::runAll() {
    for (auto& cfg : configs_) {
        for (auto& mod : moduleNames_) {
            fs::path modPath = "out" / fs::path(projectDir_) / cfg.name / pionPair_ / runPeriod_ / ("module-out___" + mod);
            auto proc = ModuleProcessorFactory::instance().create(mod);
            if (!proc) {
                LOG_WARN("Skipping unregistered processor: " + mod);
                continue;
            }
            auto res = proc->process(modPath.string(), cfg);
            allResults_[cfg.name].push_back(res);
        }
    }
}

void Synthesizer::synthesizeFinal() {
    // e.g. loop allResults_, compute errors,
    // use TCanvas / TGraph to make plots,
    // write out .tex tables.
}