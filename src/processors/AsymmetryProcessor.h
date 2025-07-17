#pragma once
#include "ModuleProcessor.h"
#include <filesystem>

class AsymmetryProcessor : public ModuleProcessor {
public:
    std::string name() const override {
        return "asymmetryPW";
    }
    Result process(const std::string& outDir, const Config& cfg) override {
        // 1) Swap to MC‑period if needed
        std::filesystem::path dir = effectiveOutDir(outDir, cfg);
        LOG_INFO("Using module-out directory: " + dir.string());

        // 2) Delegate all parsing & flattening to loadData()
        return loadData(dir);
    }

    enum PARAMETER_TYPE { VALUE, ERROR };

    /// Fetch the fitted coefficient “b_{termIndex}” for a given region.
    /// region = "background" or e.g. "signal_purity_12_12"
    double getParameterValue(const Result& r, const std::string& region, int termIndex) const;

    /// Fetch the corresponding error “b_{termIndex}_err”
    double getParameterError(const Result& r, const std::string& region, int termIndex) const;

protected:
    Result loadData(const std::filesystem::path& dir) const;
    double getParameter(const Result& r, const std::string& region, int termIndex, AsymmetryProcessor::PARAMETER_TYPE ptype) const;
};