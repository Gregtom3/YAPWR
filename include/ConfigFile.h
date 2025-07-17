#pragma once
#include "Logger.h"
#include <map>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

struct ConfigFile {
    std::map<std::string, std::vector<std::string>> cutsByPair;
    std::string binVariable;
    static ConfigFile loadFromFile(const std::string& yamlPath);
    void print() const;
};
