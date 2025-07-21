#include "AsymmetrySidebandProcessor.h"
#include "ModuleProcessorFactory.h"

#include <filesystem>

namespace {
std::unique_ptr<ModuleProcessor> make() {
    return std::make_unique<AsymmetrySidebandProcessor>();
}
const bool registered = []() {
    ModuleProcessorFactory::instance().registerProcessor("sidebandRegion", make);
    return true;
}();
} // namespace

Result AsymmetrySidebandProcessor::process(const std::string& outDir, const Config& cfg) {
    namespace fs = std::filesystem;

    // 1) get the “base” dir (with MC swap if needed)
    fs::path baseDir = effectiveOutDir(outDir, cfg);
    LOG_INFO("Scanning for sidebands under: " + baseDir.parent_path().string());

    Result combined;
    combined.moduleName = name();

    const std::string prefix = "module-out___asymmetry_sideband";
    auto parent = baseDir.parent_path();

    // 2) for every sibling dir matching our prefix
    for (auto const& entry : fs::directory_iterator(parent)) {
        if (!entry.is_directory())
            continue;
        std::string fn = entry.path().filename().string();
        if (fn.rfind(prefix, 0) != 0)
            continue;

        // sideband name is the part after "module-out___"
        std::string sbName = fn.substr(std::string("module-out___").size());

        // 3) reuse base class’s loadData() on that subdir
        Result r = loadData(entry.path());

        // 4) flatten into combined, prefixing each key
        for (auto const& [key, val] : r.scalars) {
            std::string newKey = sbName + "___" + key;
            combined.scalars[newKey] = val;
        }
    }

    return combined;
}