#include "Config.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <stdexcept>

Config Config::loadFromFile(const std::string& yamlPath) {
  // Load the YAML file
  YAML::Node node = YAML::LoadFile(yamlPath);

  // Basic sanity check
  if (!node) {
    throw std::runtime_error("Failed to load config file: " + yamlPath);
  }

  // Build Config
  Config cfg;
  cfg.params = node;

  // Derive the name from the directory containing the YAML:
  // e.g. ".../config_x0.1-0.3/config_x0.1-0.3.yaml" → "config_x0.1-0.3"
  std::filesystem::path p(yamlPath);
  cfg.name = p.parent_path().filename().string();

  return cfg;
}