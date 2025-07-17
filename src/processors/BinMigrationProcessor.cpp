#include "BinMigrationProcessor.h"
#include "ModuleProcessorFactory.h"

std::string BinMigrationProcessor::name() const {
    return "binMigration";
}

Result BinMigrationProcessor::process(const std::string& moduleOutDir, const Config& cfg) {
    // get the correct path (data --> MC) automatically:
    std::filesystem::path dir = effectiveOutDir(moduleOutDir, cfg);
    LOG_INFO("Using module-out directory: " + dir.string());

    // Load YAML for BinMigration
    auto data = loadData(dir.string());
    LOG_DEBUG("Loaded binMigration.yaml:");
    LOG_DEBUG(" file    = " + data.file);
    LOG_DEBUG(" tree    = " + data.tree);
    LOG_DEBUG(" entries = " + std::to_string(data.entries));
    LOG_DEBUG(" pion_pair = " + data.pion_pair);
    LOG_DEBUG(" primary_config = " + data.primary_config);
    LOG_DEBUG(" primary_passing = " + std::to_string(data.primary_passing));
    LOG_DEBUG(" primary cuts:");
    for (auto& c : data.primary_section.cuts)
        LOG_DEBUG("   - " + c);
    for (auto& oc : data.other_configs) {
        LOG_DEBUG(" other config: " + oc.config);
        LOG_DEBUG("  passing = " + std::to_string(oc.passing));
        for (auto& c : oc.section.cuts)
            LOG_DEBUG("   * " + c);
    }

    // Flatten Results and return
    Result r;
    r.moduleName = name();
    r.scalars["file_entries"] = data.entries;
    std::string primary_base = fs::path(data.primary_config).stem().string();
    r.scalars["primary___" + primary_base] = data.primary_passing;
    for (size_t idx = 0; idx < data.other_configs.size(); ++idx) {
        const auto& oc = data.other_configs[idx];
        std::string base = "other___" + fs::path(oc.config).stem().string();
        r.scalars[base] = oc.passing;
    }
    r.print();
    return r;
}

BinMigrationProcessor::MigrationData BinMigrationProcessor::loadData(const std::string& moduleOutDir) const {
    namespace fs = std::filesystem;
    fs::path yamlPath = fs::path(moduleOutDir) / "binMigration.yaml";
    YAML::Node root = YAML::LoadFile(yamlPath.string());

    MigrationData d;
    d.file = root["file"].as<std::string>();
    d.tree = root["tree"].as<std::string>();
    d.entries = root["entries"].as<int>();
    d.pion_pair = root["pion_pair"].as<std::string>();
    d.primary_config = root["primary_config"].as<std::string>();
    d.primary_cuts_expr = root["primary_cuts_expr"].as<std::string>();
    d.primary_passing = root["primary_passing"].as<int>();

    // parse the cuts under primary_section → <pion_pair> → cuts
    for (const auto& c : root["primary_section"][d.pion_pair]["cuts"]) {
        d.primary_section.cuts.push_back(c.as<std::string>());
    }

    // parse each entry in other_configs
    for (const auto& oc : root["other_configs"]) {
        OtherConfig o;
        o.config = oc["config"].as<std::string>();
        o.transformed_expr = oc["transformed_expr"].as<std::string>();
        o.passing = oc["passing"].as<int>();

        for (const auto& c : oc["section"][d.pion_pair]["cuts"]) {
            o.section.cuts.push_back(c.as<std::string>());
        }
        d.other_configs.push_back(std::move(o));
    }

    return d;
}

// Self registration
namespace {
std::unique_ptr<ModuleProcessor> make() {
    return std::make_unique<BinMigrationProcessor>();
}
const bool registered = []() {
    ModuleProcessorFactory::instance().registerProcessor("binMigration", make);
    return true;
}();
} // namespace