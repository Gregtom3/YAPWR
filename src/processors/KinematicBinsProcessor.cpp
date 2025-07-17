#include "KinematicBinsProcessor.h"
#include "ModuleProcessorFactory.h"

#include <filesystem>
#include <fstream>
#include <sstream>

// Self‑registration
namespace {
std::unique_ptr<ModuleProcessor> make() {
    return std::make_unique<KinematicBinsProcessor>();
}
const bool registered = []() {
    ModuleProcessorFactory::instance().registerProcessor("kinematicBins", make);
    return true;
}();
} // namespace

Result KinematicBinsProcessor::process(const std::string& moduleOutDir, const Config& cfg) {
    // Apply MC‑period swap if enabled in base class
    std::filesystem::path dir = effectiveOutDir(moduleOutDir);
    LOG_INFO("Using module‑out directory: " + dir.string());
    return loadData(dir);
}

Result KinematicBinsProcessor::loadData(const std::filesystem::path& dir) const {
    Result r;
    r.moduleName = name();

    auto csvPath = dir / "full.csv";
    std::ifstream in(csvPath.string());
    if (!in) {
        LOG_ERROR("Could not open CSV: " + csvPath.string());
        return r;
    }

    std::string headerLine, dataLine;
    if (!std::getline(in, headerLine) || !std::getline(in, dataLine)) {
        LOG_ERROR("CSV does not have two lines (header + data): " + csvPath.string());
        return r;
    }

    auto headers = Utility::split(headerLine, ',');
    auto values = Utility::split(dataLine, ',');

    if (headers.size() != values.size()) {
        LOG_WARN("Header/data column count mismatch: " + std::to_string(headers.size()) + " vs " + std::to_string(values.size()));
    }

    size_t n = std::min(headers.size(), values.size());
    for (size_t i = 0; i < n; ++i) {
        std::string key = Utility::trim(headers[i]);
        std::string sval = Utility::trim(values[i]);
        try {
            double v = std::stod(sval);
            r.scalars[key] = v;
            LOG_DEBUG(key + " = " + std::to_string(v));
        } catch (...) {
            LOG_WARN("Failed to convert \"" + sval + "\" to double for column " + key);
        }
    }

    r.print();
    return r;
}
