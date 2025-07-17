#include "NormalizationProcessor.h"
#include "ModuleProcessorFactory.h"

#include <iostream>

// Self-registration
namespace {
std::unique_ptr<ModuleProcessor> make() {
    return std::make_unique<NormalizationProcessor>();
}
const bool registered = []() {
    ModuleProcessorFactory::instance().registerProcessor("normalization", make);
    return true;
}();
} // namespace

// Preset error values for each (runVersion, pionPair)
const std::map<std::pair<std::string, std::string>, NormalizationProcessor::ErrorConfig> NormalizationProcessor::errorMap = {
    {{"Fall2018_RGA_inbending", "piplus_pi0"}, FALL2018_RGA_INB_CFG},
    {{"Fall2018_RGA_inbending", "piminus_pi0"}, FALL2018_RGA_INB_CFG},
    {{"Fall2018_RGA_inbending", "piplus_piminus"}, FALL2018_RGA_INB_CFG},
    {{"Fall2018_RGA_inbending", "piplus_piplus"}, FALL2018_RGA_INB_CFG},
    {{"Fall2018_RGA_inbending", "piminus_piminus"}, FALL2018_RGA_INB_CFG},
    {{"Fall2018_RGA_outbending", "piplus_pi0"}, FALL2018_RGA_OUTB_CFG},
    {{"Fall2018_RGA_outbending", "piminus_pi0"}, FALL2018_RGA_OUTB_CFG},
    {{"Fall2018_RGA_outbending", "piplus_piminus"}, FALL2018_RGA_OUTB_CFG},
    {{"Fall2018_RGA_outbending", "piplus_piplus"}, FALL2018_RGA_OUTB_CFG},
    {{"Fall2018_RGA_outbending", "piminus_piminus"}, FALL2018_RGA_OUTB_CFG},
    {{"Spring2019_RGA_inbending", "piplus_pi0"}, SPRING2019_RGA_INB_CFG},
    {{"Spring2019_RGA_inbending", "piminus_pi0"}, SPRING2019_RGA_INB_CFG},
    {{"Spring2019_RGA_inbending", "piplus_piminus"}, SPRING2019_RGA_INB_CFG},
    {{"Spring2019_RGA_inbending", "piplus_piplus"}, SPRING2019_RGA_INB_CFG},
    {{"Spring2019_RGA_inbending", "piminus_piminus"}, SPRING2019_RGA_INB_CFG},
    {{"Fall2018Spring2019_RGA_inbending", "piplus_pi0"}, FALL2018SPRING2019_RGA_INB_CFG},
    {{"Fall2018Spring2019_RGA_inbending", "piminus_pi0"}, FALL2018SPRING2019_RGA_INB_CFG},
    {{"Fall2018Spring2019_RGA_inbending", "piplus_piminus"}, FALL2018SPRING2019_RGA_INB_CFG},
    {{"Fall2018Spring2019_RGA_inbending", "piplus_piplus"}, FALL2018SPRING2019_RGA_INB_CFG},
    {{"Fall2018Spring2019_RGA_inbending", "piminus_piminus"}, FALL2018SPRING2019_RGA_INB_CFG},
};

Result NormalizationProcessor::process(const std::string& moduleOutDir, const Config& cfg) {
    // Normalization errors are independent of MC period
    LOG_INFO("Applying normalization for runVersion=" + cfg.getRunVersion() + ", pionPair=" + cfg.getPionPair());
    fs::path dir = effectiveOutDir(moduleOutDir, cfg);
    return loadData(dir, cfg);
}

NormalizationProcessor::ErrorConfig NormalizationProcessor::lookupErrorConfig(const Config& cfg) const {
    auto key = std::make_pair(cfg.getRunVersion(), cfg.getPionPair());
    auto it = errorMap.find(key);
    if (it != errorMap.end())
        return it->second;
    LOG_WARN("No normalization error config for " + key.first + ", " + key.second);
    return {0.0, 0.0, 0.0};
}

Result NormalizationProcessor::loadData(const fs::path& dir, const Config& cfg) const {
    Result r;
    r.moduleName = name();

    // Retrieve the preset errors
    ErrorConfig e = lookupErrorConfig(cfg);

    // Fill the Result with each relative error
    r.scalars["relative_error_beamPolarization"] = e.beamPolarization;
    r.scalars["relative_error_nonDisElectrons"] = e.nonDisElectrons;
    r.scalars["relative_error_radiativeCorrections"] = e.radiativeCorrections;

    return r;
}
