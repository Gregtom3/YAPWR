#include "Config.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

Config::Config(const std::string& yamlPath) {
    setNameFromYamlPath(yamlPath);
    _cfgFile = ConfigFile::loadFromFile(yamlPath);
    _cfgFile.print();
}

void Config::setNameFromYamlPath(const std::string& yamlPath) {
    std::filesystem::path p(yamlPath);
    name = p.parent_path().filename().string();
}