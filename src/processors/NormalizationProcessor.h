#pragma once

#include "Config.h"
#include "ModuleProcessor.h"
#include <filesystem>
#include <map>
#include <utility>

namespace fs = std::filesystem;

/// Processor to apply preset normalization errors per runVersion and pionPair
class NormalizationProcessor : public ModuleProcessor {
public:
    std::string name() const override {
        return "normalization";
    }

    /// Return a Result filled with relative_error_* scalars based on Config
    Result process(const std::string& moduleOutDir, const Config& cfg) override;

protected:
    // Most normalization errors do not depend on MC period
    bool useMcPeriod() const override {
        return false;
    }

private:
    /// Holds the preset error fractions for one config
    struct ErrorConfig {
        double beamPolarization;
        double nonDisElectrons;
        double radiativeCorrections;
    };

    // Named, pre-defined errors
    static constexpr double BEAM_POL_ERR_RGA_INB = 0.031;
    static constexpr double NONDIS_E_ERR_RGA_INB = 0.001;
    static constexpr double RAD_CORR_ERR_RGA_INB = 0.05;

    static constexpr double BEAM_POL_ERR_RGA_OUTB = 0.036;
    static constexpr double NONDIS_E_ERR_RGA_OUTB = 0.001;
    static constexpr double RAD_CORR_ERR_RGA_OUTB = 0.05;

    // Named ErrorConfig instances
    static constexpr ErrorConfig FALL2018_RGA_INB_CFG = {BEAM_POL_ERR_RGA_INB, NONDIS_E_ERR_RGA_INB, RAD_CORR_ERR_RGA_INB};

    static constexpr ErrorConfig FALL2018_RGA_OUTB_CFG = {BEAM_POL_ERR_RGA_OUTB, NONDIS_E_ERR_RGA_OUTB, RAD_CORR_ERR_RGA_OUTB};

    static constexpr ErrorConfig SPRING2019_RGA_INB_CFG = {BEAM_POL_ERR_RGA_INB, NONDIS_E_ERR_RGA_INB, RAD_CORR_ERR_RGA_INB};

    static constexpr ErrorConfig FALL2018SPRING2019_RGA_INB_CFG = {BEAM_POL_ERR_RGA_INB, NONDIS_E_ERR_RGA_INB, RAD_CORR_ERR_RGA_INB};

    /// Map from (runVersion, pionPair) to preset ErrorConfig
    static const std::map<std::pair<std::string, std::string>, ErrorConfig> errorMap;

    /// Lookup the ErrorConfig for this Config, or return zeros with a warning
    ErrorConfig lookupErrorConfig(const Config& cfg) const;

    /// Build the Result using the preset values
    Result loadData(const fs::path& dir, const Config& cfg) const;
};