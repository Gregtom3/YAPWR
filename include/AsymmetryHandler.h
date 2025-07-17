#pragma once

#include <map>
#include <string>
#include "Result.h"
#include "AsymmetryProcessor.h"
#include "KinematicBinsProcessor.h"
#include "Config.h"

/// Handle asymmetryPW results across all configs
class AsymmetryHandler {
public:
    explicit AsymmetryHandler(const std::map<std::string, std::map<std::string, Result>>& allResults,
                              const std::map<std::string, Config>& configMap);

    void collectRawAsymmetryData(const std::string& region,
                                 int termIndex,
                                 const std::string& binPrefix) const;
private:
    AsymmetryProcessor asymProc_;
    const std::map<std::string, Config> configMap_;
    const std::map<std::string, std::map<std::string, Result>> allResults_;     
};