#include "SidebandRegionError.h"
#include "Logger.h"

SidebandRegionError::SidebandRegionError(const Config& cfg)
  : cfg_(cfg)
{
}

double SidebandRegionError::getRelativeError(const Result&      /*r*/,
                                             const std::string& /*region*/,
                                             int                /*pwTerm*/)
{

    // Only process pi0
    if(!cfg_.contains_pi0()){
        LOG_INFO("Skipping SidebandRegionError for non-Pi0 dihadron");
        return 0.00;
    }

    
    
    constexpr double kFallback = 0.04;

    LOG_WARN(errorName()
             << ": side‑band error not yet implemented — returning "
             << kFallback);
    return kFallback;
}
