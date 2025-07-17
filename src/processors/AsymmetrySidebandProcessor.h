#pragma once

#include "AsymmetryProcessor.h"
#include <filesystem>

/// Handle multiple sidebands under module‑out___asymmetry_sideband*
class AsymmetrySidebandProcessor : public AsymmetryProcessor {
public:
    // Factory key
    std::string name() const override {
        return "asymmetry_sideband";
    }

    // Override to scan wildcard subdirs instead of a single dir
    Result process(const std::string& outDir, const Config& cfg) override;
};