#include "BaryonContaminationProcessor.h"
#include "ModuleProcessorFactory.h"

// Self register
namespace {
std::unique_ptr<ModuleProcessor> make() {
    return std::make_unique<BaryonContaminationProcessor>();
}
const bool registered = []() {
    ModuleProcessorFactory::instance().registerProcessor("baryonContamination", make);
    return true;
}();
} // namespace

std::string BaryonContaminationProcessor::name() const {
    return "baryonContamination";
}

Result BaryonContaminationProcessor::loadData(const std::filesystem::path& dir) const {
    Result r;
    r.moduleName = name();

    auto yamlPath = dir / "baryonContamination.yaml";
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
    static const std::vector<std::string> sections = {"trueparentpid_1", "trueparentpid_2", "trueparentparentpid_1",
                                                      "trueparentparentpid_2"};

    for (auto const& sec : sections) {
        if (!root[sec])
            continue;
        LOG_DEBUG(sec + ":");
        for (const auto& kv : root[sec]) {
            int pid = std::stoi(kv.first.as<std::string>());
            int count = kv.second.as<int>();
            std::string label = sec + "_" + std::to_string(pid);
            r.scalars[label] = count;
            LOG_DEBUG("  " + label + " = " + std::to_string(count));
        }
    }

    return r;
}
