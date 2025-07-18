#pragma once

#include "AsymmetryProcessor.h"
#include "BaryonContaminationError.h"
#include "BinMigrationError.h"
#include "Config.h"
#include "KinematicBinsProcessor.h"
#include "NormalizationError.h"
#include "ParticleMisidentificationError.h"
#include "Result.h"
#include <map>
#include <string>

/// Handle asymmetryPW results across all configs
class AsymmetryHandler {
public:
    explicit AsymmetryHandler(const std::map<std::string, std::map<std::string, Result>>& allResults,
                              const std::map<std::string, Config>& configMap);

    void reportAsymmetry(const std::string& region, int termIndex, const std::string& binPrefix) const;

    void collectSystematics(const std::string& region, int termIndex, const std::string& binPrefix) const;

protected:
    void createSortedConfigNames();

private:
    void initializeAsymmetryMaps(const std::string& region = "background",
                                 int termIndex = 0) const;   // defaults
    std::pair<std::string, double> getBinInfo(const std::string& cfgName, const std::string& binPrefix) const;
    AsymmetryProcessor asymProc_;
    std::vector<std::string> sortedCfgNames_;
    const std::map<std::string, Config> configMap_;
    const std::map<std::string, std::map<std::string, Result>> allResults_;
    mutable std::unordered_map<std::string,double> asymValue_;
    mutable std::unordered_map<std::string,double> asymStatErr_;
    mutable std::unordered_map<std::string,double> asymSysErr_;
};