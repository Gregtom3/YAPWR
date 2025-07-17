#include "BinMigrationProcessor.h"
#include "ModuleProcessorFactory.h"

#include <filesystem>
#include <yaml-cpp/yaml.h>

namespace {
std::unique_ptr<ModuleProcessor> make() {
    return std::make_unique<BinMigrationProcessor>();
}
const bool registered = []() {
    ModuleProcessorFactory::instance().registerProcessor("binMigration", make);
    return true;
}();
} // namespace

Result BinMigrationProcessor::loadData(const fs::path& dir) const {
    Result r;
    r.moduleName = name();

    // Locate and open the YAML
    fs::path yamlPath = dir / "binMigration.yaml";
    if (!fs::exists(yamlPath)) {
        LOG_ERROR("YAML not found: " + yamlPath.string());
        return r;
    }
    YAML::Node root = YAML::LoadFile(yamlPath.string());

    // 1) entries
    int entries = root["entries"].as<int>();
    r.scalars["entries"] = entries;
    LOG_DEBUG("entries = " + std::to_string(entries));

    // 2) primary passing
    std::string primaryCfg = root["primary_config"].as<std::string>();
    int primaryPass = root["primary_passing"].as<int>();
    std::string primaryKey = "primary___" + fs::path(primaryCfg).stem().string();
    r.scalars[primaryKey] = primaryPass;
    LOG_DEBUG(primaryKey + " = " + std::to_string(primaryPass));

    // 3) other_configs
    for (const auto& oc : root["other_configs"]) {
        std::string cfgPath = oc["config"].as<std::string>();
        int pass = oc["passing"].as<int>();
        std::string key = "other___" + fs::path(cfgPath).stem().string();
        r.scalars[key] = pass;
        LOG_DEBUG(key + " = " + std::to_string(pass));
    }

    // 4) log the map
    r.print();
    return r;
}
