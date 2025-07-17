#pragma once

#include "ModuleProcessor.h"
namespace fs = std::filesystem;
#include <filesystem>
#include <yaml-cpp/yaml.h>

class BaryonContaminationProcessor : public ModuleProcessor {
public:
    std::string name() const override;
    Result process(const std::string& moduleOutDir, const Config& cfg) override {
        // Auto‑swap to MC if needed
        std::filesystem::path dir = effectiveOutDir(moduleOutDir, cfg);
        LOG_INFO("Using module‑out directory: " + dir.string());
        return loadData(dir);
    }

protected:
    bool useMcPeriod() const override {
        return true;
    }

private:
    Result loadData(const std::filesystem::path& dir) const;
};
