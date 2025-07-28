#pragma once

#include "AsymmetryProcessor.h"
#include "BaryonContaminationError.h"
#include "BinMigrationError.h"
#include "Config.h"
#include "Constants.h"
#include "KinematicBinsProcessor.h"
#include "NormalizationError.h"
#include "ParticleMisidentificationError.h"
#include "PurityBinningError.h"
#include "Result.h"
#include "SidebandRegionError.h"
#include <TDecompLU.h>
#include <TMatrixD.h>
#include <TVectorD.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
namespace fs = std::filesystem;

/// Handle asymmetryPW results across all configs
class AsymmetryHandler {

    struct Record {
        std::string cfgName;    // e.g. “piplus_pi0_rga_fall2018”
        std::string pionPair;   // Config::getPionPair()
        std::string runVersion; // Config::getRunVersion()
        std::string binVar;     // e.g. “x”, “Mh”, …
        int L;                  // partial wave L
        int M;                  // partial wave M
        int TWIST;
        std::string modulationLatex;
        std::string region;
        double binVal = NAN; // numeric value of that variable
        double A = NAN;      // asymmetry
        double sStat = NAN;  // statistical error
        double sSys = NAN;   // total systematic error

        // systematic breakdown (relative ↔ absolute)
        double rBinMig = 0., aBinMig = 0.;
        double rBary = 0., aBary = 0.;
        double rMisID = 0., aMisID = 0.;
        double rSreg = 0., aSreg = 0.;
        double rPbin = 0., aPbin = 0.;
        std::map<std::string, double> rNorm; // polarisation, target‑density, …
        std::map<std::string, double> aNorm;
    };

public:
    explicit AsymmetryHandler(const std::map<std::string, std::map<std::string, Result>>& allResults,
                              const std::map<std::string, Config>& configMap);

    void reportAsymmetry(const std::string& region, int termIndex, const std::string& binPrefix) const;

    void collectSystematics(const std::string& region, int termIndex, const std::string& binPrefix) const;
    void dumpYaml(const std::string& outPath, bool append) const;

    void setMutateBinMigration(bool v) {
        mutateBinMigration_ = v;
    }

protected:
    void createSortedConfigNames();

private:
    void initializeAsymmetryMaps(const std::string& region = "background",
                                 int termIndex = 0) const; // defaults
    std::pair<std::string, double> getBinInfo(const std::string& cfgName, const std::string& binPrefix) const;
    void unfoldAsymmetryViaBinMigration_(const std::unordered_map<std::string, const Result*>& allBinMig) const;
    AsymmetryProcessor asymProc_;
    std::vector<std::string> sortedCfgNames_;
    const std::map<std::string, Config> configMap_;
    const std::map<std::string, std::map<std::string, Result>> allResults_;
    mutable std::unordered_map<std::string, double> asymValue_;
    mutable std::unordered_map<std::string, double> asymStatErr_;
    mutable std::unordered_map<std::string, double> asymSysErr_;
    mutable std::vector<Record> records_;

    bool mutateBinMigration_ = false;
};