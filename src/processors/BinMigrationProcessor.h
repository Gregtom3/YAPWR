#pragma once

#include "Constants.h"
#include "ModuleProcessor.h"
#include <filesystem>

namespace fs = std::filesystem;

/// Processor for “binMigration” that automatically swaps to MC and
/// parses binMigration.yaml into a Result
class BinMigrationProcessor : public ModuleProcessor {
public:
    std::string name() const override {
        return "binMigration";
    }

    /// Build the effective dir (data vs MC), then parse YAML into a Result
    Result process(const std::string& moduleOutDir, const Config& cfg) override {
        // 1) Compute the right directory (data or MC)
        fs::path dir = effectiveOutDir(moduleOutDir, cfg);
        LOG_INFO("Using module-out directory: " + dir.string());

        // 2) Parse YAML + flatten into a Result
        return loadData(dir);
    }

protected:
    bool useMcPeriod() const override {
        return true;
    }

private:
    /// Load & parse binMigration.yaml under `dir` directly into a Result
    Result loadData(const fs::path& dir) const;
};