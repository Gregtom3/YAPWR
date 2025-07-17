#include "AsymmetryProcessor.h"
#include "ModuleProcessorFactory.h"
#include <filesystem>
#include <yaml-cpp/yaml.h>

namespace {
std::unique_ptr<ModuleProcessor> make() {
    return std::make_unique<AsymmetryProcessor>();
}
const bool registered = []() {
    ModuleProcessorFactory::instance().registerProcessor("asymmetryPW", make);
    return true;
}();
} // namespace

Result AsymmetryProcessor::loadData(const std::filesystem::path& dir) const {
    Result r;
    r.moduleName = name();

    auto yamlPath = dir / "asymmetry_results.yaml";
    if (!std::filesystem::exists(yamlPath)) {
        LOG_ERROR("YAML not found: " + yamlPath.string());
        return r;
    }

    YAML::Node doc = YAML::LoadFile(yamlPath.string());

    // Loop over each region block
    for (const auto& node : doc["results"]) {
        std::string region = node["region"].as<std::string>();
        int entries = node["entries"].as<int>();

        // 1) record entries
        r.scalars[region + ".entries"] = entries;
        LOG_DEBUG("Region " + region + " entries = " + std::to_string(entries));

        // 2) detect fit_failed and skip parameters if true
        if (auto ff = node["fit_failed"]; ff && ff.as<bool>()) {
            LOG_WARN("Fit failed for region: " + region);
            continue;
        }

        // 3) record all other keys
        for (const auto& kv : node) {
            const std::string key = kv.first.as<std::string>();
            if (key == "region" || key == "entries" || key == "fit_failed")
                continue;

            double value = kv.second.as<double>();
            std::string scalarName = region + "." + key;
            r.scalars[scalarName] = value;
            LOG_DEBUG("  " + scalarName + " = " + std::to_string(value));
        }
    }

    // 4) final dump
    r.print();
    return r;
}