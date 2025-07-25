#pragma once

#include "ModuleProcessor.h"
#include <filesystem>
#include <yaml-cpp/yaml.h>
namespace fs = std::filesystem;

class ParticleMisidentificationProcessor : public ModuleProcessor {
public:
    std::string name() const override;

    Result process(const std::string& moduleOutDir, const Config& cfg) override {
        // swap in MC‑period if needed (inherited)
        std::filesystem::path dir = effectiveOutDir(moduleOutDir, cfg);
        LOG_INFO("Using module-out directory: " + dir.string());
        return loadData(dir);
    }
    void plotSummary(const std::string& moduleOutDir, const Config& cfg) const override;
protected:
    bool useMcPeriod() const override {
        return true;
    }

private:
    Result loadData(const std::filesystem::path& dir) const;
};
