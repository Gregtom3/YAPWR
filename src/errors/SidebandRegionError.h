#pragma once
#include "Error.h"
#include "Config.h"

class SidebandRegionError : public Error {
public:
    SidebandRegionError(const Config& cfg, const double asymValue);

    std::string errorName() const override { return "sidebandRegion"; }

    double getRelativeError(const Result&      r,
                            const std::string& region,
                            int                pwTerm) override;

private:
    const Config& cfg_;    
    const double asymValue_;
};
