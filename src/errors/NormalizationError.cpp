#include "NormalizationError.h"
#include "Logger.h"
#include <cmath>

NormalizationError::NormalizationError(Config& cfg) : cfg_(cfg) {}

const std::vector<std::string>& NormalizationError::components()
{
    static const std::vector<std::string> list = {
        "beamPolarization",
        "nonDisElectrons",
        "radiativeCorrections"
    };
    return list;
}

double NormalizationError::getRelativeError(const Result& r,
                                            const std::string& type)
{
    std::string key = "relative_error_" + type;
    auto it = r.scalars.find(key);
    if (it != r.scalars.end()) return it->second;

    LOG_WARN("NormalizationError: missing '" << key << "' – using 0");
    return 0.0;
}

double NormalizationError::getRelativeError(const Result& r,
                                            const std::string& /*region*/,
                                            int /*pwTerm*/)
{
    double sumsq = 0.0;
    for (const auto& t : components()) {
        double rel = getRelativeError(r, t);
        sumsq += rel * rel;
    }
    return std::sqrt(sumsq);
}
