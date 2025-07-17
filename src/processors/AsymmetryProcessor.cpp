#include "AsymmetryProcessor.h"
#include "ModuleProcessorFactory.h"
#include <filesystem>
#include <limits>
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
        }
    }

    return r;
}

double AsymmetryProcessor::getParameter(const Result& r, const std::string& region, int termIndex,
                                        AsymmetryProcessor::PARAMETER_TYPE ptype) const {

    std::string key = region + "." + "b_" + std::to_string(termIndex);
    if (ptype == AsymmetryProcessor::PARAMETER_TYPE::ERROR) {
        key += "_err";
    }

    auto it = r.scalars.find(key);
    if (it == r.scalars.end()) {
        LOG_ERROR("AsymmetryProcessor: parameter not found: " + key);
        return std::numeric_limits<double>::quiet_NaN();
    }
    return it->second;
}

double AsymmetryProcessor::getParameterValue(const Result& r, const std::string& region, int termIndex) const {
    return getParameter(r, region, termIndex, AsymmetryProcessor::PARAMETER_TYPE::VALUE);
}

double AsymmetryProcessor::getParameterError(const Result& r, const std::string& region, int termIndex) const {
    return getParameter(r, region, termIndex, AsymmetryProcessor::PARAMETER_TYPE::ERROR);
}