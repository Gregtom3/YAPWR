#pragma once
#include "ConfigFile.h"
#include "Constants.h"
#include "Logger.h"
#include <map>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
/// Parsed per‑bin configuration, loaded from YAML
class Config {
public:
    Config(const std::string& yamlPath, const std::string& projectDir, const std::string& pionPair, const std::string& runVersion);

    void setName(const std::string& _name) {
        name = _name;
    }

    std::string getProjectName() const {
        return _projectDir;
    }
    std::string getPionPair() const {
        return _pionPair;
    }
    std::string getRunVersion() const {
        return _runVersion;
    }
    std::string getMCVersion() const {
        return _mcVersion;
    }

    std::string name;

private:
    void setMCVersion();
    void setNameFromYamlPath(const std::string& yamlPath);
    ConfigFile getConfigFile() {
        return _cfgFile;
    }
    ConfigFile _cfgFile;
    std::string _projectDir, _pionPair, _runVersion, _mcVersion;
};