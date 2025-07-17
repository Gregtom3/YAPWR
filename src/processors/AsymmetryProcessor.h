#pragma once
#include "ModuleProcessor.h"
#include <filesystem>

class AsymmetryProcessor : public ModuleProcessor {
public:
    std::string name() const override {
        return "asymmetryPW";
    }
    Result process(const std::string& outDir, const Config& cfg) override;

protected:
    Result loadData(const std::filesystem::path& dir) const;
};