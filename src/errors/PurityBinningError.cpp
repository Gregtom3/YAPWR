#include "PurityBinningError.h"
#include "Logger.h"
#include <numeric>  
#include <cmath>   
#include <iomanip>
#include <regex>

PurityBinningError::PurityBinningError(const Config& cfg, const double asymValue)
  : cfg_(cfg)
  , asymValue_(asymValue)
{
}

double PurityBinningError::getRelativeError(const Result&      r,
                                            const std::string& region,
                                            int                pwTerm)
{
    const std::string tag = "b_" + std::to_string(pwTerm);     
    const std::regex  tag_re("(^|\\.)" + tag + "($|[^0-9])");  // exact b_n

    std::vector<double> vals;
    for (const auto& [key, val] : r.scalars) {
        if (key.find(DEFAULT_BKG_REGION) == std::string::npos) continue;
        if (key.find("signal") == std::string::npos) continue;
        if (!std::regex_search(key, tag_re)) continue;        // exact b_n
        if (key.find("_err")          != std::string::npos) continue;

        vals.push_back(val);
        LOG_WARN("PurityBinningError match  : " << key << " = "
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

    LOG_DEBUG("PurityBinningError: mean=" << mean
             << "  sigma=" << sigma
             << "  rel=" << rel);

    return rel; 
}
