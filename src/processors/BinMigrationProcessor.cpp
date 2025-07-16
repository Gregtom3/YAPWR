#include "BinMigrationProcessor.h"
#include "ModuleProcessorFactory.h"

std::string BinMigrationProcessor::name() const {
    return "binMigration";
}

Result BinMigrationProcessor::process(const std::string& moduleOutDir, const Config& cfg) {
    // get the correct path (data --> MC) automatically:
    std::filesystem::path dir = effectiveOutDir(moduleOutDir);
    LOG_INFO("Using module-out directory: " + dir.string());

    Result r;
    r.moduleName = name();
    // TODO: open files in moduleOutDir, read data into r.scalars, etc.
    return r;
}

// 3) Self‑registration so the factory knows about you
namespace {
std::unique_ptr<ModuleProcessor> make() {
    return std::make_unique<BinMigrationProcessor>();
}
const bool registered = []() {
    ModuleProcessorFactory::instance().registerProcessor("binMigration", make);
    return true;
}();
} // namespace