#include "SidebandRegionError.h"
#include "Logger.h"
#include <numeric>  
#include <cmath>   
#include <iomanip>
SidebandRegionError::SidebandRegionError(const Config& cfg, const double asymValue)
  : cfg_(cfg)
  , asymValue_(asymValue)
{
}

double SidebandRegionError::getRelativeError(const Result&      r,
                                             const std::string& region,
                                             int                pwTerm)
{
    // Only relevant for π0 dihadron configs
    if (!cfg_.contains_pi0()) {
        LOG_INFO("SidebandRegionError: skipping non‑π0 config");
        return 0.0;
    }

    const std::string tag = ".b_" + std::to_string(pwTerm);

    std::vector<double> vals;
    for (const auto& [key, val] : r.scalars) {
        if (key.find(region) == std::string::npos) continue; // e.g. "signal"
        if (key.find(tag)    == std::string::npos) continue; // ".b_n"
        if (key.find("_err") != std::string::npos) continue; // skip errors

        vals.push_back(val);
        LOG_DEBUG("Sideband match : " << key << " = "
                 << std::setprecision(6) << val);
    }

    const std::size_t N = vals.size();
    if (N < 2) {
        LOG_WARN("SidebandRegionError: less than two side‑band values for b_"
                 << pwTerm << " → returning 0");
        return 0.0;
    }

    // mean
    const double mean = std::accumulate(vals.begin(), vals.end(), 0.0) / N;

    // variance
    double ssq = 0.0;
    for (double v : vals) ssq += (v - mean) * (v - mean);
    const double sigma = std::sqrt(ssq / (N - 1));   // unbiased σ

    // relative uncertainty
    const double rel = (asymValue_ != 0.0) ? sigma / std::fabs(asymValue_) : 0.0;

    LOG_DEBUG("SidebandRegionError: mean=" << mean
             << "  sigma=" << sigma
             << "  rel=" << rel);

    return rel; 
}