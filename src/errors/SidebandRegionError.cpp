#include "SidebandRegionError.h"
#include "Logger.h"
#include <iomanip>

SidebandRegionError::SidebandRegionError(const Config& cfg)
  : cfg_(cfg)
{
}

double SidebandRegionError::getRelativeError(const Result&      r,
                                             const std::string& region,
                                             int                pwTerm)
{

    //r.print(Logger::FORCE);

    // Only process pi0
    if(!cfg_.contains_pi0()){
        LOG_INFO("Skipping SidebandRegionError for non-Pi0 dihadron");
        return 0.00;
    }

    const std::string termTag = ".b_" + std::to_string(pwTerm);

    std::vector<std::pair<std::string,double>> matches;

    for (const auto& [key, val] : r.scalars) {
        if (key.find(region) == std::string::npos)      continue;
        if (key.find(termTag)  == std::string::npos)      continue;
        if (key.find("_err")   != std::string::npos)      continue;

        matches.emplace_back(key, val);
    }

    // log the discovered values
    if (matches.empty()) {
        LOG_WARN("SidebandRegionError: no side‑band keys found for b_"
                 << pwTerm);
    } else {
        for (const auto& [k, v] : matches) {
            LOG_WARN("SidebandRegionError: " << k << " = "
                     << std::setprecision(6) << v);
        }
    }

    // ── placeholder until proper calculation is added ─────────────
    constexpr double kFallback = 0.04;
    return kFallback;
}