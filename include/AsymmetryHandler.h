#pragma once

#include "AsymmetryProcessor.h"
#include "Config.h"
#include "KinematicBinsProcessor.h"
#include "Result.h"
#include "BaryonContaminationError.h"
#include "ParticleMisidentificationError.h"
#include "BinMigrationError.h"
#include "NormalizationError.h"
#include <map>
#include <string>

/// Handle asymmetryPW results across all configs
class AsymmetryHandler {
public:
    explicit AsymmetryHandler(const std::map<std::string, std::map<std::string, Result>>& allResults,
                              const std::map<std::string, Config>& configMap);

    void reportAsymmetry(const std::string& region,
                         int termIndex,
                         const std::string& binPrefix) const;

    void collectSystematics(const std::string& region,
                            int termIndex,
                            const std::string& binPrefix) const;
private:
    AsymmetryProcessor asymProc_;
    const std::map<std::string, Config> configMap_;
    const std::map<std::string, std::map<std::string, Result>> allResults_;
};