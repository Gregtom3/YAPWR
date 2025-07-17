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
    // 1) Compute data vs MC directory
    fs::path dir = effectiveOutDir(moduleOutDir, cfg);
    LOG_INFO("Using module-out directory: " + dir.string());

    // 2) Prepare the Result
    Result r;
    r.moduleName = name();

    // 3) Always load full.csv
    loadCsv(dir / "full.csv", "full", r);

    // 4) If this config contains π0, also load signal.csv and background.csv
    if (cfg.contains_pi0()) {
        loadCsv(dir / "signal.csv", "signal", r);
        loadCsv(dir / "background.csv", "background", r);
    }

    return r;
}

void KinematicBinsProcessor::loadCsv(const fs::path& csvPath, const std::string& prefix, Result& r) const {
    std::ifstream in(csvPath.string());
    if (!in) {
        LOG_WARN("Could not open CSV: " + csvPath.string());
        return;
    }

    std::string headerLine, dataLine;
    if (!std::getline(in, headerLine) || !std::getline(in, dataLine)) {
        LOG_WARN("CSV missing header or data line: " + csvPath.string());
        return;
    }

    // split and trim
    auto headers = Utility::split(headerLine, ',');
    auto values = Utility::split(dataLine, ',');

    if (headers.size() != values.size()) {
        LOG_WARN("Header/data count mismatch in " + csvPath.string() + " (" + std::to_string(headers.size()) + " vs " +
                 std::to_string(values.size()) + ")");
    }

    // for each column
    size_t n = std::min(headers.size(), values.size());
    for (size_t i = 0; i < n; ++i) {
        std::string key = Utility::trim(headers[i]);
        std::string sval = Utility::trim(values[i]);

        // prefix it
        std::string scalarName = prefix + "___" + key;
        try {
            double v = std::stod(sval);
            r.scalars[scalarName] = v;
            LOG_DEBUG(scalarName + " = " + std::to_string(v));
        } catch (...) {
            LOG_WARN("Failed to convert '" + sval + "' to double for column " + scalarName);
        }
    }
}
