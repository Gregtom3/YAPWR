#include "AsymmetryHandler.h"
#include "Logger.h"

AsymmetryHandler::AsymmetryHandler(const std::map<std::string, std::map<std::string, Result>>& allResults,
                                   const std::map<std::string, Config>& configMap)
    : asymProc_()
    , allResults_(allResults)
    , configMap_(configMap) {}

void AsymmetryHandler::reportAsymmetry(const std::string& region,
                                       int               termIndex,
                                       const std::string& binPrefix) const
{
    for (const auto& [cfgName, modules] : allResults_) {
        //------------------------------------------------------------
        // 0) Which kinematic field?  (e.g. "x", "Q2", ...)
        //------------------------------------------------------------
        const std::string binField = configMap_.at(cfgName).getBinVariable();

        double binVal = std::numeric_limits<double>::quiet_NaN();
        if (auto kIt = modules.find("kinematicBins"); kIt != modules.end()) {
            binVal = KinematicBinsProcessor::getBinScalar(
                         kIt->second, binPrefix, binField);
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
        const double  A    = asymProc_.getParameterValue (aRes, region, termIndex);
        const double  sStat= asymProc_.getParameterError (aRes, region, termIndex);

        //------------------------------------------------------------
        // 2)  Grab each systematic contribution (relative errors)
        //------------------------------------------------------------
        double rBinMig = 0.0, rBary = 0.0, rMisID = 0.0, rNorm = 0.0;
        BaryonContaminationError bcErr;
        BinMigrationError bmErr;
        ParticleMisidentificationError pmErr;
        NormalizationError normErr;

        if (auto it = modules.find("binMigration"); it != modules.end())
            rBinMig = bmErr.getError(it->second, region, termIndex);

        if (auto it = modules.find("baryonContamination"); it != modules.end())
            rBary   = bcErr.getError(it->second, region, termIndex);

        if (auto it = modules.find("particleMisidentification"); it != modules.end())
            rMisID  = pmErr.getError(it->second, region, termIndex);

        if (auto it = modules.find("normalization"); it != modules.end())
            rNorm   = normErr.getError(it->second, region, termIndex);

        //------------------------------------------------------------
        // 3)  Convert to absolute errors & quadrature sum
        //------------------------------------------------------------
        const double aAbs   = std::abs(A);
        const double sBinMig= aAbs * rBinMig;
        const double sBary  = aAbs * rBary;
        const double sMisID = aAbs * rMisID;
        const double sNorm  = aAbs * rNorm;

        const double sSys = std::sqrt(  sBinMig*sBinMig
                                      + sBary  *sBary
                                      + sMisID *sMisID
                                      + sNorm  *sNorm );

        //------------------------------------------------------------
        // 4)  Print summary and breakdown
        //------------------------------------------------------------
        LOG_INFO("[" << cfgName << "] "
                 << region << ".b_" << termIndex
                 << " = " << A
                 << "  +/-stat " << sStat
                 << "  +/-sys  " << sSys
                 << "  |  " << binPrefix << "___" << binField
                 << " = " << binVal);

        LOG_INFO("    binMigration        : rel " << rBinMig << ", abs " << sBinMig);
        LOG_INFO("    baryonContamination : rel " << rBary   << ", abs " << sBary);
        LOG_INFO("    particleMisID       : rel " << rMisID  << ", abs " << sMisID);
        LOG_INFO("    normalization       : rel " << rNorm   << ", abs " << sNorm);
    }
}

