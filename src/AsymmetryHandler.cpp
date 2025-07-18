#include "AsymmetryHandler.h"
#include "Logger.h"

AsymmetryHandler::AsymmetryHandler(const std::map<std::string, std::map<std::string, Result>>& allResults,
                                   const std::map<std::string, Config>& configMap)
    : asymProc_()
    , allResults_(allResults)
    , configMap_(configMap) {

    createSortedConfigNames();
}

// Creates the "sortedCfgNames_" vector
void AsymmetryHandler::createSortedConfigNames() const {
    std::vector<std::pair<double, std::string>> tmp;
    for (const auto& [cfgName, modules] : allResults_) {
        Config thisConfig = configMap_.at(cfgName);
        const std::string binField = thisConfig.getBinVariable();

        double binVal = std::numeric_limits<double>::quiet_NaN();
        if (auto kIt = modules.find("kinematicBins"); kIt != modules.end()) {
            binVal = KinematicBinsProcessor::getBinScalar(kIt->second, "full", binField);
        } else {
            LOG_WARN("No kinematicBins result for config " << cfgName);
        }

        tmp.emplace_back(binVal, cfgName);
    }
    // sort by the bin variable
    std::sort(tmp.begin(), tmp.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    // keep only the ordered names
    for (const auto& [bin, name] : tmp)
        sortedCfgNames_.push_back(name);
}

void AsymmetryHandler::reportAsymmetry(const std::string& region, int termIndex, const std::string& binPrefix) const {
    for (const std::string& cfgName : sortedCfgNames_) {
        const auto& modules = allResults_.at(cfgName);
        std::cout << cfgName << std::endl;
        //------------------------------------------------------------
        // 0) Which kinematic field?  (e.g. "x", "Q2", ...)
        //------------------------------------------------------------
        Config thisConfig = configMap_.at(cfgName);
        const std::string binField = thisConfig.getBinVariable();

        double binVal = std::numeric_limits<double>::quiet_NaN();
        if (auto kIt = modules.find("kinematicBins"); kIt != modules.end()) {
            binVal = KinematicBinsProcessor::getBinScalar(kIt->second, binPrefix, binField);
        } else {
            LOG_WARN("No kinematicBins result for config " << cfgName);
        }
        //------------------------------------------------------------
        // 1)  Fetch the asymmetry value and its statistical error
        //------------------------------------------------------------
        auto aIt = modules.find(asymProc_.name());
        if (aIt == modules.end()) {
            LOG_WARN("No asymmetryPW result for config " << cfgName);
            continue;
        }
        const Result& aRes = aIt->second;
        const double A = asymProc_.getParameterValue(aRes, region, termIndex);
        const double sStat = asymProc_.getParameterError(aRes, region, termIndex);

        //------------------------------------------------------------
        // 2)  Grab each systematic contribution (relative errors)
        //------------------------------------------------------------
        double rBinMig = 0.0, rBary = 0.0, rMisID = 0.0;
        BaryonContaminationError bcErr(thisConfig);
        BinMigrationError bmErr(thisConfig);
        ParticleMisidentificationError pmErr(thisConfig);
        NormalizationError normErr(thisConfig);

        if (auto it = modules.find("binMigration"); it != modules.end())
            rBinMig = bmErr.getRelativeError(it->second, region, termIndex);

        if (auto it = modules.find("baryonContamination"); it != modules.end())
            rBary = bcErr.getRelativeError(it->second, region, termIndex);

        if (auto it = modules.find("particleMisidentification"); it != modules.end())
            rMisID = pmErr.getRelativeError(it->second, region, termIndex);

        // ----- Normalization pieces -----
        std::map<std::string, double> rNorm;
        double sNormTotal = 0.0;
        if (auto it = modules.find("normalization"); it != modules.end()) {
            for (const auto& comp : NormalizationError::components()) {
                double rel = normErr.getRelativeError(it->second, comp);
                rNorm[comp] = rel;
                sNormTotal += rel * rel;
            }
            sNormTotal = std::sqrt(sNormTotal);
        }

        // ---------- 3) absolute + total -----------------------------
        const double aAbs = std::abs(A);
        const double sBinMig = aAbs * rBinMig;
        const double sBary = aAbs * rBary;
        const double sMisID = aAbs * rMisID;
        const double sNormAbs = aAbs * sNormTotal;

        const double sSys = std::sqrt(sBinMig * sBinMig + sBary * sBary + sMisID * sMisID + sNormAbs * sNormAbs);

        // ---------- 4) print summary --------------------------------
        LOG_INFO("[" << cfgName << "] " << region << ".b_" << termIndex << " = " << A << "  +/-stat " << sStat << "  +/-sys  " << sSys
                     << "  |  " << binPrefix << "___" << binField << " = " << binVal);

        LOG_INFO("    binMigration        : rel " << rBinMig << ", abs " << sBinMig);
        LOG_INFO("    baryonContamination : rel " << rBary << ", abs " << sBary);
        LOG_INFO("    particleMisID       : rel " << rMisID << ", abs " << sMisID);

        // normalization breakdown
        for (const auto& [comp, rel] : rNorm) {
            const double absErr = aAbs * rel;
            LOG_INFO("    normalization(" << comp << ") : rel " << rel << ", abs " << absErr);
        }
    }
}
