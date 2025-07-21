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
void AsymmetryHandler::createSortedConfigNames() {
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

void AsymmetryHandler::initializeAsymmetryMaps(const std::string& region, int termIndex) const {
    for (const std::string& cfgName : sortedCfgNames_) {
        const auto& modules = allResults_.at(cfgName);
        // --- asymmetry value & stat ---------------------------------
        auto aIt = modules.find(asymProc_.name());
        if (aIt == modules.end())
            continue;
        const Result& aRes = aIt->second;

        double A = asymProc_.getParameterValue(aRes, region, termIndex);
        double sStat = asymProc_.getParameterError(aRes, region, termIndex);
        double sSys = 0.0;
        asymValue_[cfgName] = A;
        asymStatErr_[cfgName] = sStat;
        asymSysErr_[cfgName] = sSys;
    }
}

std::pair<std::string, double> AsymmetryHandler::getBinInfo(const std::string& cfgName, const std::string& binPrefix) const {
    const auto& modules = allResults_.at(cfgName);
    Config thisConfig = configMap_.at(cfgName);
    const std::string binField = thisConfig.getBinVariable();

    double binVal = std::numeric_limits<double>::quiet_NaN();
    if (auto kIt = modules.find("kinematicBins"); kIt != modules.end()) {
        binVal = KinematicBinsProcessor::getBinScalar(kIt->second, binPrefix, binField);
    } else {
        LOG_WARN("No kinematicBins result for config " << cfgName);
    }
    return std::make_pair(binField, binVal);
}

void AsymmetryHandler::reportAsymmetry(const std::string& region, int termIndex, const std::string& binPrefix) const {

    // First initialize the Asymmetry map, logging all asymmetry values/statistical errors
    initializeAsymmetryMaps(region, termIndex);

    // Determination of the systematic errors
    // Loop over each kinematic bin
    for (const std::string& cfgName : sortedCfgNames_) {
        Config thisConfig = configMap_.at(cfgName);
        const auto& modules = allResults_.at(cfgName);
        const auto& binInfo = getBinInfo(cfgName, binPrefix);
        const std::string binField = binInfo.first;
        const double binVal = binInfo.second;
        //------------------------------------------------------------
        // 1)  Fetch the asymmetry value and its statistical error
        //------------------------------------------------------------
        const double A = asymValue_.at(cfgName);
        const double sStat = asymStatErr_.at(cfgName);
        //------------------------------------------------------------
        // 2)  Grab each systematic contribution (relative errors)
        //------------------------------------------------------------
        double rBinMig = 0.0, rBary = 0.0, rMisID = 0.0;
        BaryonContaminationError bcErr(thisConfig);
        BinMigrationError bmErr(thisConfig, configMap_, sortedCfgNames_, asymValue_);
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
