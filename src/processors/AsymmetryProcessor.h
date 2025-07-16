#pragma once
#include "ModuleProcessor.h"
#include "ModuleProcessorFactory.h"
#include <map>
#include <string>
#include <vector>

class AsymmetryProcessor : public ModuleProcessor {
public:
    std::string name() const override {
        return "asymmetryPW";
    }
    Result process(const std::string& outDir, const Config& cfg) override;

private:
    struct RegionAsym {
        std::string region;
        int entries;
        std::map<std::string, double> params; // e.g. "bkg_b_0" → value
        std::map<std::string, double> errors; // e.g. "bkg_b_0_err" → value
    };
    void printAsymmetryResults(const std::vector<AsymmetryProcessor::RegionAsym>& regs);
    std::vector<RegionAsym> loadData(const std::string& outDir);
};