#pragma once
#include <memory>
#include <string>
#include <map>
#include "ModuleProcessor.h"

/// Registry of available ModuleProcessor implementations
class ModuleProcessorFactory {
public:
  using Creator = std::unique_ptr<ModuleProcessor>(*)();

  static ModuleProcessorFactory& instance();
  void registerProcessor(const std::string& name, Creator c);
  std::unique_ptr<ModuleProcessor> create(const std::string& name) const;

private:
  std::map<std::string, Creator> creators_;
};