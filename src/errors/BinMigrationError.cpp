#include "BinMigrationError.h"
#include "Logger.h"

BinMigrationError::BinMigrationError(const Config& cfg,
                                     const std::map<std::string,Config>&           cfgMap,
                                     const std::vector<std::string>&               sortedCfgNames,
                                     const std::unordered_map<std::string,double>& asymValue)
  : cfg_           (cfg)
  , configMap_     (cfgMap)
  , sortedCfgNames_(sortedCfgNames)
  , asymValue_     (asymValue)
{}

double BinMigrationError::getRelativeError(const Result&      r,
                                           const std::string& region,
                                           int                pwTerm)
{
    // --- (keep this if you still want the debug print) ------------
    r.print(Logger::FORCE);

    // --- find index of this config in the ordered list ------------
    const std::string& myName = cfg_.name;   // assumes Config has public .name
    auto it = std::find(sortedCfgNames_.begin(),
                        sortedCfgNames_.end(),
                        myName);

    if (it == sortedCfgNames_.end()) {
        LOG_WARN("BinMigrationError: config " << myName
                 << " not found in sorted list");
    } else {
        const ptrdiff_t idx = std::distance(sortedCfgNames_.begin(), it);
        std::string below = (idx > 0)
            ? sortedCfgNames_[idx - 1]
            : "<none>";
        std::string above = (idx + 1 < static_cast<ptrdiff_t>(sortedCfgNames_.size()))
            ? sortedCfgNames_[idx + 1]
            : "<none>";

        LOG_INFO("BinMigrationError: bin \""   << myName
                 << "\"   below: \"" << below
                 << "\"   above: \"" << above << '"');
    }

    // ----------------------------------------------------------------
    constexpr double kFallback = 0.03;   // still the provisional value
    return kFallback;
}
