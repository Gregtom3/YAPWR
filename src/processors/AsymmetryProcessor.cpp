#include "AsymmetryProcessor.h"
#include <filesystem>
#include <yaml-cpp/yaml.h>

Result AsymmetryProcessor::process(const std::string& outDir, const Config& cfg) {
    Result r;
    r.moduleName = name();

    // 1) Parse the YAML into a vector of RegionAsym
    auto regions = loadData(outDir);
    printAsymmetryResults(regions);
    // 2) Flatten into r.scalars, prefixing keys with region name
    for (auto& reg : regions) {
        // store entries as well
        r.scalars[reg.region + ".entries"] = reg.entries;

        // store each parameter and its error
        for (auto& [key, val] : reg.params) {
            r.scalars[reg.region + "." + key] = val;
        }
        for (auto& [key, val] : reg.errors) {
            r.scalars[reg.region + "." + key] = val;
        }
    }

    return r;
}

void AsymmetryProcessor::printAsymmetryResults(const std::vector<AsymmetryProcessor::RegionAsym>& regs) {
    for (const auto& reg : regs) {
        LOG_INFO("=== Region: " + reg.region + " ===");
        LOG_INFO("Entries: " + std::to_string(reg.entries));
        LOG_INFO("Parameters:");
        for (const auto& kv : reg.params) {
            LOG_INFO("  " + kv.first + " = " + std::to_string(kv.second));
        }
        LOG_INFO("Errors:");
        for (const auto& kv : reg.errors) {
            LOG_INFO("  " + kv.first + " = " + std::to_string(kv.second));
        }
        LOG_INFO("");
    }
}

std::vector<AsymmetryProcessor::RegionAsym> AsymmetryProcessor::loadData(const std::string& outDir) {
    namespace fs = std::filesystem;
    fs::path yamlPath = fs::path(outDir) / "asymmetry_results.yaml";
    YAML::Node doc = YAML::LoadFile(yamlPath.string());

    std::vector<RegionAsym> result;
    for (const auto& node : doc["results"]) {
        RegionAsym reg;
        reg.region = node["region"].as<std::string>();
        reg.entries = node["entries"].as<int>();

        // now iterate the map entries
        for (const auto& kv : node) {
            const std::string key = kv.first.as<std::string>();
            if (key == "region" || key == "entries")
                continue;

            double value = kv.second.as<double>();
            if (key.size() > 4 && key.substr(key.size() - 4) == "_err")
                reg.errors[key] = value;
            else
                reg.params[key] = value;
        }

        result.push_back(std::move(reg));
    }
    return result;
}

// Register:
namespace {
std::unique_ptr<ModuleProcessor> make() {
    return std::make_unique<AsymmetryProcessor>();
}
// Immediately load this module
const bool registered = []() {
    ModuleProcessorFactory::instance().registerProcessor("asymmetryPW", make);
    return true;
}();
} // namespace