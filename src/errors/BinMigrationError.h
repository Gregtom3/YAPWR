#pragma once
#include "Error.h"
#include <unordered_map>
#include <vector>
#include <string>

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

private:
    const Config&                                   cfg_;
    const std::map<std::string,Config>&             configMap_;
    const std::vector<std::string>&                 sortedCfgNames_;
    const std::unordered_map<std::string,double>&   asymValue_;
    const std::unordered_map<std::string,const Result*>& binMig_;
};
