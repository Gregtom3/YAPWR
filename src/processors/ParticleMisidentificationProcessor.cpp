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

Result ParticleMisidentificationProcessor::process(const std::string& moduleOutDir, const Config& cfg) {
    // get the correct path (data --> MC) automatically:
    std::filesystem::path dir = effectiveOutDir(moduleOutDir);
    LOG_INFO("Using module-out directory: " + dir.string());

    Result r;
    r.moduleName = name();

    // 1) Load structured data
    auto d = loadData(dir);

    // 2) Debug‑print everything in one go
    LOG_DEBUG("total_entries = " + std::to_string(d.total_entries));

    // table of label → pointer to each map in MisIDData
    const std::vector<std::pair<std::string, const std::map<int, int>*>> tables = {
        {"truepid_e", &d.truepid_e},   {"truepid_1", &d.truepid_1},   {"truepid_2", &d.truepid_2},  {"truepid_11", &d.truepid_11},
        {"truepid_12", &d.truepid_12}, {"truepid_21", &d.truepid_21}, {"truepid_22", &d.truepid_22}};

    for (auto const& [label, mp] : tables) {
        LOG_DEBUG(label + ":");
        for (auto const& [pid, count] : *mp) {
            LOG_DEBUG("  " + std::to_string(pid) + " → " + std::to_string(count));
        }
    }

    // 3) Flatten into scalars
    r.scalars["total_entries"] = d.total_entries;
    for (auto const& [label, mp] : tables) {
        for (auto const& [pid, count] : *mp) {
            r.scalars[label + "_" + std::to_string(pid)] = count;
        }
    }

    // 4) Print final Result
    r.print();
    return r;
}

ParticleMisidentificationProcessor::MisIDData ParticleMisidentificationProcessor::loadData(const std::string& moduleOutDir) const {

    fs::path yamlPath = fs::path(moduleOutDir) / "particleMisidentification.yaml";
    YAML::Node root = YAML::LoadFile(yamlPath.string());

    MisIDData d;
    d.total_entries = root["total_entries"].as<int>();

    auto parseMap = [&](char const* key, auto& target) {
        for (const auto& kv : root[key]) {
            int pid = std::stoi(kv.first.as<std::string>());
            int count = kv.second.as<int>();
            target[pid] = count;
        }
    };

    parseMap("truepid_e", d.truepid_e);
    parseMap("truepid_1", d.truepid_1);
    parseMap("truepid_2", d.truepid_2);
    parseMap("truepid_11", d.truepid_11);
    parseMap("truepid_12", d.truepid_12);
    parseMap("truepid_21", d.truepid_21);
    parseMap("truepid_22", d.truepid_22);
    return d;
}
