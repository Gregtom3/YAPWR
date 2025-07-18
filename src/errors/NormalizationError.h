#pragma once
#include "Error.h"

class NormalizationError : public Error {
public:
    explicit NormalizationError(Config &cfg);
    std::string errorName() const override { return "normalization"; }

    double getRelativeError(const Result& r,
                            const std::string& region,
                            int pwTerm) override;

private:
    Config cfg_;
};