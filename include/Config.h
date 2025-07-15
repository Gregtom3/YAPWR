#pragma once
#include <yaml-cpp/yaml.h>

#include <string>

/// Parsed per‑bin configuration, loaded from YAML
struct Config {
  std::string name;   // e.g. "config_x0.1-0.3"
  YAML::Node params;  // arbitrary parameters from YAML

  static Config loadFromFile(const std::string& yamlPath);
};