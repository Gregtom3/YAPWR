#include "AsymmetryProcessor.h"
#include <yaml-cpp/yaml.h>
#include <TFile.h>
#include <TTree.h>

Result AsymmetryProcessor::process(const std::string& outDir, const Config& cfg) {
  Result r; r.moduleName = name();
  // e.g. open outDir+"/asymmetry_results.yaml", parse, fill r.scalars...
  return r;
}

// Register:
namespace {
  std::unique_ptr<ModuleProcessor> make() { return std::make_unique<AsymmetryProcessor>(); }
  const bool registered = [](){
    ModuleProcessorFactory::instance().registerProcessor("asymmetryPW", make);
    return true;
  }();
}