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
    /// Modules that want to swap in MC‑period override this to return true.
    virtual bool useMcPeriod() const { return false; }

    /// Strip off the last two components (runPeriod + module‑out___name),
    /// pick data or MC period, then rebuild the full path.
    std::filesystem::path effectiveOutDir(const std::filesystem::path& moduleOutDir) const {
        auto leaf       = moduleOutDir.filename();                  // "module-out___<mod>"
        auto runDirName = moduleOutDir.parent_path().filename().string(); // e.g. "Fall2018_RGA_inbending"
        auto prefix     = moduleOutDir.parent_path().parent_path(); // everything above runDir

        // decide which period string to use
        std::string period = runDirName;
        if (useMcPeriod()) {
            auto mc = Constants::runToMc.find(runDirName);
            if (mc == Constants::runToMc.end()) {
                LOG_WARN("No MC mapping for runPeriod: " + runDirName);
            } else {
                period = mc->second;
                LOG_DEBUG("Mapped runPeriod '" + runDirName +
                          "' → MC period '" + period + "'");
            }
        }

        return prefix / period / leaf;
    }
};