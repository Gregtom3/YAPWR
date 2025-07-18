#pragma once
#include "Error.h"
#include "Config.h"
#include <string>
#include <vector>

class NormalizationError : public Error {
public:
    explicit NormalizationError(Config& cfg);

    std::string errorName() const override { return "normalization"; }

    /// Fetch a specific component (returns 0 if missing)
    double getRelativeError(const Result& r,
                            const std::string& type);

    /// Total (quadrature) relative error – satisfies Error base
    double getRelativeError(const Result& r,
                            const std::string& /*region*/,
                            int /*pwTerm*/) override;

    /// Let callers know the list of component keys
    static const std::vector<std::string>& components();

private:
    Config cfg_;
};
