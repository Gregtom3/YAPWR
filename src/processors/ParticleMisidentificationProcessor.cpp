#include "ParticleMisidentificationProcessor.h"
#include "ModuleProcessorFactory.h"

namespace {
std::unique_ptr<ModuleProcessor> make() {
    return std::make_unique<ParticleMisidentificationProcessor>();
}
const bool registered = []() {
    ModuleProcessorFactory::instance().registerProcessor("particleMisidentification", make);
    return true;
}();
} // namespace

std::string ParticleMisidentificationProcessor::name() const {
    return "particleMisidentification";
}

Result ParticleMisidentificationProcessor::loadData(const std::filesystem::path& dir) const {
    Result r;
    r.moduleName = name();

    auto yamlPath = dir / "particleMisidentification.yaml";
    if (!std::filesystem::exists(yamlPath)) {
        LOG_ERROR("YAML not found: " + yamlPath.string());
        return r;
    }

    YAML::Node root = YAML::LoadFile(yamlPath.string());

    // 1) total_entries
    int total = root["total_entries"].as<int>();
    r.scalars["total_entries"] = total;
    LOG_DEBUG("total_entries = " + std::to_string(total));

    // 2) all the pid maps
    static const std::vector<std::string> keys = {"truepid_e",  "truepid_1",  "truepid_2", "truepid_11",
                                                  "truepid_12", "truepid_21", "truepid_22"};

    for (auto const& key : keys) {
        if (!root[key])
            continue; // skip missing sections
        LOG_DEBUG(key + ":");
        for (const auto& kv : root[key]) {
            int pid = std::stoi(kv.first.as<std::string>());
            int count = kv.second.as<int>();
            std::string label = key + "_" + std::to_string(pid);
            r.scalars[label] = count;
            LOG_DEBUG("  " + label + " = " + std::to_string(count));
        }
    }

    // 3) final dump
    r.print();
    return r;
}
