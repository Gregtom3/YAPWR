#pragma once

#include "ModuleProcessor.h"
#include "Utility.h"
#include <filesystem>
#include <vector>
namespace fs = std::filesystem;
/// Processor to load and report kinematic bin definitions
class KinematicBinsProcessor : public ModuleProcessor {
public:
    std::string name() const override {
        return "kinematicBins";
    }

    Result process(const std::string& moduleOutDir, const Config& cfg) override;

    /// Helper: fetch a scalar by prefix and field name (e.g. prefix="full", field="x")
    static double getBinScalar(const Result& r, const std::string& prefix, const std::string& field);

private:
    void loadCsv(const fs::path& csvPath, const std::string& prefix, Result& r) const;
};
