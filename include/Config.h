#pragma once
#include "ConfigFile.h"
#include "Logger.h"
#include <map>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
/// Parsed per‑bin configuration, loaded from YAML
class Config {
public:
    Config(const std::string& yamlPath);
    std::string name;
    void setName(const std::string& _name) {
        name = _name;
    }

private:
    void setNameFromYamlPath(const std::string& yamlPath);
    ConfigFile getConfigFile() {
        return _cfgFile;
    }
    ConfigFile _cfgFile;
};