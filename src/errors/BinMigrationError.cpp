#include "BinMigrationError.h"
#include "Logger.h"

BinMigrationError::BinMigrationError(Config& cfg)
    : cfg_(cfg) {}

double BinMigrationError::getRelativeError(const Result& r,
                                           const std::string& region,
                                           int pwTerm)
{
    const std::string key = region + ".b_" + std::to_string(pwTerm) + "_relerr";
    auto it = r.scalars.find(key);
    if (it != r.scalars.end()) return it->second;

    constexpr double kFallback = 0.03;
    LOG_WARN("BinMigrationError: missing '" << key
              << "' → defaulting to " << kFallback);
    return kFallback;
}