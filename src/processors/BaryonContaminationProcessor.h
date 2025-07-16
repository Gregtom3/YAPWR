#pragma once

#include "ModuleProcessor.h"
namespace fs = std::filesystem;
#include <filesystem>
#include <yaml-cpp/yaml.h> 

class BaryonContaminationProcessor : public ModuleProcessor {
public:
    std::string name() const override;
    Result process(const std::string& moduleOutDir,
                   const Config& cfg) override;
protected:
    bool useMcPeriod() const override {
        return true;
    }

private:
    struct ContaminationData {
        int total_entries;
        std::map<int,int> trueparentpid_1;
        std::map<int,int> trueparentpid_2;
        std::map<int,int> trueparentparentpid_1;
        std::map<int,int> trueparentparentpid_2;
    };

    ContaminationData loadData(const std::string& moduleOutDir) const;
};
