#include "Config.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

Config::Config(const std::string& yamlPath, const std::string& projectDir, const std::string& pionPair,
               const std::string& runVersion) {
    setNameFromYamlPath(yamlPath);
    _projectDir = projectDir;
    _pionPair = pionPair;
    _runVersion = runVersion;
    setMCVersion();
    _cfgFile = ConfigFile::loadFromFile(yamlPath);
}

void Config::setNameFromYamlPath(const std::string& yamlPath) {
    std::filesystem::path p(yamlPath);
    name = p.parent_path().filename().string();
}

void Config::print() const {
    LOG_INFO(Logger::makeBar(40));
    LOG_INFO("Config Contents: ");
    LOG_INFO(" - Project Directory = " + _projectDir);
    LOG_INFO(" - Pion Pair         = " + _pionPair);
    LOG_INFO(" - runVersion        = " + _runVersion);
    LOG_INFO(" - monteCarloVersion = " + _mcVersion);
    _cfgFile.print();
    LOG_INFO(Logger::makeBar(40));
}

void Config::setMCVersion() {
    auto mc = Constants::runToMc.find(_runVersion);
    if (mc == Constants::runToMc.end()) {
        LOG_WARN("No MC mapping for runPeriod: " + _runVersion);
    } else {
        _mcVersion = mc->second;
        LOG_DEBUG("Mapped runPeriod '" + _runVersion + "' → MC period '" + _mcVersion + "'");
    }
}
