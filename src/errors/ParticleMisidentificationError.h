#pragma once
#include "Error.h"

class ParticleMisidentificationError : public Error {
public:

    explicit ParticleMisidentificationError(Config &cfg);
    std::string errorName() const override { return "particleMisidentification"; }

    double getRelativeError(const Result& r,
                            const std::string& region,
                            int pwTerm) override;

private:
    Config cfg_;
};