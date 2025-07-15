#include "Synthesizer.h"
#include "ModuleProcessorFactory.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

Synthesizer::Synthesizer(const std::string& pd,
                         const std::string& pp,
                         const std::string& rp)
 : projectDir_(pd), pionPair_(pp), runPeriod_(rp)
{}

void Synthesizer::discoverConfigs() {
  fs::path base = fs::path(projectDir_) / pionPair_ / runPeriod_;
  for (auto& d : fs::directory_iterator(base)) {
    if (d.is_directory() && d.path().extension()==".yaml") continue;
    auto cfg = Config::loadFromFile((d.path() / (d.path().filename().string()+".yaml")).string());
    configs_.push_back(cfg);
  }
}

void Synthesizer::runAll() {
  for (auto& cfg : configs_) {
    for (auto& mod : moduleNames_) {
      fs::path modPath = fs::path(projectDir_)
        / cfg.name / pionPair_ / runPeriod_
        / ("module-out___" + mod);
      auto proc = ModuleProcessorFactory::instance().create(mod);
      auto res  = proc->process(modPath.string(), cfg);
      allResults_[cfg.name].push_back(res);
    }
  }
}

void Synthesizer::synthesizeFinal() {
  // e.g. loop allResults_, compute errors,
  // use TCanvas / TGraph to make plots,
  // write out .tex tables.
}