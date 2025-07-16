#pragma once

#include "Constants.h"
#include "ModuleProcessor.h"
#include <filesystem>

namespace fs = std::filesystem;

class BinMigrationProcessor : public ModuleProcessor {
public:
    std::string name() const override;
    Result process(const std::string& moduleOutDir, const Config& cfg) override;

protected:
    bool useMcPeriod() const override {
        return true;
    }

private:
    struct ConfigCuts {
        std::vector<std::string> cuts;
    };

    struct OtherConfig {
        std::string config;
        ConfigCuts section;
        std::string transformed_expr;
        int passing;
    };

    struct MigrationData {
        std::string file;
        std::string tree;
        int entries;
        std::string pion_pair;
        std::string primary_config;
        ConfigCuts primary_section;
        std::string primary_cuts_expr;
        int primary_passing;
        std::vector<OtherConfig> other_configs;
    };

    /// Load & parse binMigration.yaml under moduleOutDir
    MigrationData loadData(const std::string& moduleOutDir) const;
};