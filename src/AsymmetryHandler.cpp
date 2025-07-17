#include "AsymmetryHandler.h"
#include "Logger.h"

AsymmetryHandler::AsymmetryHandler(const std::map<std::string, std::map<std::string, Result>>& allResults,
                                   const std::map<std::string, Config>& configMap)
    : asymProc_()
    , allResults_(allResults)
    , configMap_(configMap) {}

void AsymmetryHandler::collectRawAsymmetryData(const std::string& region, int termIndex, const std::string& binPrefix) const {
    for (const auto& [cfgName, modules] : allResults_) {
        std::string binField = configMap_.at(cfgName).getBinVariable();
        // 1) Asymmetry coefficient
        auto aIt = modules.find(asymProc_.name());
        if (aIt == modules.end()) {
            LOG_WARN("No asymmetryPW result for config " << cfgName);
            continue;
        }
        const Result& aRes = aIt->second;
        double asymVal = asymProc_.getParameterValue(aRes, region, termIndex);

        // 2) Kinematic‐bins value
        auto kIt = modules.find("kinematicBins");
        double binVal = std::numeric_limits<double>::quiet_NaN();
        if (kIt != modules.end()) {
            const Result& kRes = kIt->second;
            binVal = KinematicBinsProcessor::getBinScalar(kRes, binPrefix, binField);
        } else {
            LOG_WARN("No kinematicBins result for config " << cfgName);
        }

        // 3) Debug print both
        LOG_WARN("[" << cfgName << "] " << region << ".b_" << termIndex << " = " << asymVal << "   |   " << binPrefix << "___"
                     << binField << " = " << binVal);
    }
}