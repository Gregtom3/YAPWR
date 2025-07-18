#pragma once
#include "Error.h"

class ParticleMisidentificationError : public Error {
public:
    std::string errorName() const override { return "particleMisidentification"; }

    double getError(const Result& r,
                    const std::string& region,
                    int pwTerm) override;
};