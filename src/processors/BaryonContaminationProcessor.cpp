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

Result BaryonContaminationProcessor::process(const std::string& moduleOutDir, const Config& cfg) {
    // get the correct path (data --> MC) automatically:
    std::filesystem::path dir = effectiveOutDir(moduleOutDir);
    LOG_INFO("Using module-out directory: " + dir.string());

    Result r;
    r.moduleName = name();

    // 1) parse YAML
    auto d = loadData(dir);

    // 2) debug‑print counts
    LOG_DEBUG("total_entries = " + std::to_string(d.total_entries));
    auto dump = [&](auto& m, const char* label) {
        LOG_DEBUG(std::string(label) + ":");
        for (auto const& [pid, count] : m)
            LOG_DEBUG("  " + std::to_string(pid) + " → " + std::to_string(count));
    };
    dump(d.trueparentpid_1, "trueparentpid_1");
    dump(d.trueparentpid_2, "trueparentpid_2");
    dump(d.trueparentparentpid_1, "trueparentparentpid_1");
    dump(d.trueparentparentpid_2, "trueparentparentpid_2");

    // 3) (example) flatten into scalars
    r.scalars["total_entries"] = d.total_entries;
    for (auto const& [pid, count] : d.trueparentpid_1)
        r.scalars["trueparentpid1_" + std::to_string(pid)] = count;
    for (auto const& [pid, count] : d.trueparentpid_2)
        r.scalars["trueparentpid2_" + std::to_string(pid)] = count;
    for (auto const& [pid, count] : d.trueparentparentpid_1)
        r.scalars["trueparentparentpid1_" + std::to_string(pid)] = count;
    for (auto const& [pid, count] : d.trueparentparentpid_2)
        r.scalars["trueparentparentpid2_" + std::to_string(pid)] = count;

    r.print();
    return r;
}

BaryonContaminationProcessor::ContaminationData BaryonContaminationProcessor::loadData(const std::string& moduleOutDir) const {

    fs::path yamlPath = fs::path(moduleOutDir) / "baryonContamination.yaml";
    YAML::Node root = YAML::LoadFile(yamlPath.string());

    ContaminationData d;
    d.total_entries = root["total_entries"].as<int>();

    auto parseMap = [&](char const* key, auto& target) {
        for (const auto& kv : root[key]) {
            int pid = std::stoi(kv.first.as<std::string>());
            int count = kv.second.as<int>();
            target[pid] = count;
        }
    };

    parseMap("trueparentpid_1", d.trueparentpid_1);
    parseMap("trueparentpid_2", d.trueparentpid_2);
    parseMap("trueparentparentpid_1", d.trueparentparentpid_1);
    parseMap("trueparentparentpid_2", d.trueparentparentpid_2);

    return d;
}
