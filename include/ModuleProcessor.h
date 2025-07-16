#pragma once
#include <filesystem>
#include <map>
#include <string>

#include "Config.h"
#include "Constants.h"
#include "Logger.h"
#include "Result.h"

/// Base class for all analysis‐module processors
/// Each processor reads its own module‐output directory and fills a Result.
class ModuleProcessor {
public:
    virtual ~ModuleProcessor() = default;

    /// Return the name of the module (e.g. "asymmetryPW", "binMigration")
    virtual std::string name() const = 0;

    /// Process one configuration:
    virtual Result process(const std::string& moduleOutDir, const Config& cfg) = 0;

protected:
    /// Grab the runPeriod component from the path:
    /// parent of "module-out___<modName>"
    std::string extractRunPeriod(const std::string& moduleOutDir) const {
        return std::filesystem::path(moduleOutDir)
            .parent_path() // .../Fall2018_RGA_inbending
            .filename()    // "Fall2018_RGA_inbending"
            .string();
    }

    /// Lookup that runPeriod in the Constants::runToMc map
    /// Returns empty string (and logs a warning) if not found.
    std::string mapToMcPeriod(const std::string& moduleOutDir) const {
        auto runPeriod = extractRunPeriod(moduleOutDir);
        auto it = Constants::runToMc.find(runPeriod);
        if (it == Constants::runToMc.end()) {
            LOG_WARN("No MC mapping for runPeriod: " + runPeriod);
            return "";
        }
        auto mcPeriod = it->second;
        LOG_DEBUG("Mapped runPeriod '" + runPeriod + "' --> MC period '" + mcPeriod + "'");
        return mcPeriod;
    }
};