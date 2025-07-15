#pragma once
#include "../ModuleProcessor.h"

class AsymmetryProcessor : public ModuleProcessor {
public:
  std::string name() const override { return "asymmetryPW"; }
  Result process(const std::string& outDir, const Config& cfg) override;
};