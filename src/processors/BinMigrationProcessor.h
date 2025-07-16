#pragma once

#include "ModuleProcessor.h"

class BinMigrationProcessor : public ModuleProcessor {
public:
  std::string name() const override;
  Result process(const std::string& moduleOutDir,
                 const Config& cfg) override;
};