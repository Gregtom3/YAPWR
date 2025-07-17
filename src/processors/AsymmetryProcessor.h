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

protected:
    Result loadData(const std::filesystem::path& dir) const;
};