#pragma once

#include "ModuleProcessor.h"
#include "Constants.h"
#include <filesystem>

class BinMigrationProcessor : public ModuleProcessor {
public:
  std::string name() const override;
  Result process(const std::string& moduleOutDir,
                 const Config& cfg) override;
};