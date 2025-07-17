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

    bool supportsConfig(const Config& cfg) const override {
        // only run on these two pion‑pairs
        auto pp = cfg.getPionPair();
        return (pp == "piplus_pi0" || pp == "piminus_pi0");
    }

};