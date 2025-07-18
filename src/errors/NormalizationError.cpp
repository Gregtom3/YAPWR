#include "NormalizationError.h"
#include "Logger.h"

NormalizationError::NormalizationError(Config& cfg)
    : cfg_(cfg) {}

double NormalizationError::getRelativeError(const Result& r,
                                            const std::string& region,
                                            int pwTerm)
{
    // Try per‑term first …
    std::string key = region + ".b_" + std::to_string(pwTerm) + "_relerr";
    auto it = r.scalars.find(key);

    // … or fall back to a generic scalar "relative_error_normalization"
    if (it == r.scalars.end()) {
        key = "relative_error_normalization";
        it  = r.scalars.find(key);
    }

    if (it != r.scalars.end()) return it->second;

    constexpr double kFallback = 0.03;
    LOG_WARN("NormalizationError: missing '" << key
              << "' → defaulting to " << kFallback);
    return kFallback;
}
