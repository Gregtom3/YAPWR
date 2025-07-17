#pragma once

#include "ModuleProcessor.h"
#include "Utility.h"
#include <filesystem>
#include <vector>

/// Processor to load and report kinematic bin definitions
class KinematicBinsProcessor : public ModuleProcessor {
public:
    std::string name() const override {
        return "kinematicBins";
    }

    Result process(const std::string& moduleOutDir, const Config& cfg) override;

private:
    /// Load full.csv from dir and return a Result with every field
    Result loadData(const std::filesystem::path& dir) const;
};
