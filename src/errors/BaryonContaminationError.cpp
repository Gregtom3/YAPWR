#include "BaryonContaminationError.h"
#include "Logger.h"

double BaryonContaminationError::getError(const Result& r,
                                          const std::string& region,
                                          int pwTerm)
{

    const std::string key = region + ".b_" + std::to_string(pwTerm) + "_relerr";

    auto it = r.scalars.find(key);
    if (it != r.scalars.end()) {
        return it->second;                 // use the value saved by the processor
    }

    // Fallback: default to 3% relative error
    constexpr double kFallback = 0.03;
    LOG_WARN("BaryonContaminationError: missing '" << key
              << "' in Result → defaulting to " << kFallback);
    return kFallback;
}