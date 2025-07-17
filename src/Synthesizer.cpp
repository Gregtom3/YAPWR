#include "Synthesizer.h"

#include <iostream>
#include <memory>

#include "AsymmetryProcessor.h"
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

        configs_.push_back(Config(yamlPath.string(), projectDir_, pionPair_, runPeriod_));
    }

    // Ensure we found at least one valid config
    if (configs_.empty()) {
        throw std::runtime_error("Synthesizer::discoverConfigs(): no valid config directories found under " + config_base.string());
    }
    LOG_INFO("Found " << configs_.size() << " configs");
}

void Synthesizer::runAll() {
    for (auto& cfg : configs_) {
        cfg.print();
        for (auto& mod : moduleNames_) {
            LOG_INFO("Parsing config=" + cfg.name + " , module=" + mod);
            fs::path modPath = "out" / fs::path(projectDir_) / cfg.name / pionPair_ / runPeriod_ / ("module-out___" + mod);
            auto proc = ModuleProcessorFactory::instance().create(mod);
            if (!proc) {
                LOG_WARN("Skipping unregistered processor: " + mod);
                continue;
            }

            // skip if the processor says it isn’t applicable
            if (!proc->supportsConfig(cfg)) {
                LOG_INFO("Skipping processor " + mod + " for pionPair=" + cfg.getPionPair());
                continue;
            }

            auto res = proc->process(modPath.string(), cfg);

            // Debuggable print statement
            res.print();

            allResults_[cfg.name][mod] = res;
            LOG_INFO(" --> Added " + cfg.name + " to allResults\n");
        }
    }
}

void Synthesizer::synthesizeFinal() {
    // Create an AsymmetryProcessor to query parameters
    auto procPtr = ModuleProcessorFactory::instance().create("asymmetryPW");
    auto* asymProc = dynamic_cast<AsymmetryProcessor*>(procPtr.get());
    if (!asymProc) {
        LOG_ERROR("AsymmetryProcessor not available for final synthesis");
        return;
    }

    // Loop over each config and log background parameter b_7
    for (auto& cfg : configs_) {
        const std::string& cfgName = cfg.name;
        auto modIt = allResults_.find(cfgName);
        if (modIt == allResults_.end())
            continue;
        auto resIt = modIt->second.find("asymmetryPW");
        if (resIt == modIt->second.end()) {
            LOG_WARN("No asymmetryPW result for config " + cfgName);
            continue;
        }
        const Result& result = resIt->second;
        double b7 = asymProc->getParameterValue(result, "background", 7);
        LOG_WARN("[" + cfgName + "] background.b_7 = " + std::to_string(b7));
    }
}