#pragma once

#include "ModuleProcessor.h"
#include <filesystem>
#include <yaml-cpp/yaml.h>
namespace fs = std::filesystem;
/// Processor to evaluate particle misidentification rates for each config
class ParticleMisidentificationProcessor : public ModuleProcessor {
public:
    /// Factory key
    std::string name() const override;

    /// Read module‑out directory, compute misID metrics, fill Result
    Result process(const std::string& moduleOutDir, const Config& cfg) override;

protected:
    bool useMcPeriod() const override {
        return true;
    }

private:
    struct MisIDData {
        int total_entries;
        std::map<int, int> truepid_e;
        std::map<int, int> truepid_1;
        std::map<int, int> truepid_2;
        std::map<int, int> truepid_11;
        std::map<int, int> truepid_12;
        std::map<int, int> truepid_21;
        std::map<int, int> truepid_22;
    };

    MisIDData loadData(const std::string& moduleOutDir) const;
};
