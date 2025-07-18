#pragma once
#include "Error.h"

class NormalizationError : public Error {
public:
    std::string errorName() const override { return "normalization"; }

    double getError(const Result& r,
                    const std::string& region,
                    int pwTerm) override;
};