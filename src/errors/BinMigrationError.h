#pragma once
#include "Error.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <TMatrixD.h>

class BinMigrationError : public Error {
public:
    BinMigrationError(const Config&                                   cfg,
                      const std::map<std::string,Config>&             cfgMap,
                      const std::vector<std::string>&                 sortedCfgNames,
                      const std::unordered_map<std::string,double>&   asymValue,
                      const std::unordered_map<std::string,const Result*>& binMig);

    std::string errorName() const override { return "binMigration"; }

    /// Relative error for one region / PW‑term
    double getRelativeError(const Result&      r,
                            const std::string& region,
                            int                pwTerm);

    // Build M with rows = reconstructed bins, columns = true (generated) bins.
    // This orientation satisfies  A_rec = M * A_true  ⇒  A_true = M^{-1} * A_rec
    TMatrixD getMigrationMatrix_RecoRows_TrueCols() const;

private:
    const Config&                                   cfg_;
    const std::map<std::string,Config>&             configMap_;
    const std::vector<std::string>&                 sortedCfgNames_;
    const std::unordered_map<std::string,double>&   asymValue_;
    const std::unordered_map<std::string,const Result*>& binMig_;
    void logMigrationMatrix_() const;
};
