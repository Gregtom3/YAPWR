#pragma once
#include "Error.h"
#include "Config.h"

class SidebandRegionError : public Error {
public:
    explicit SidebandRegionError(const Config& cfg);

    std::string errorName() const override { return "sidebandRegion"; }

    double getRelativeError(const Result&      r,
                            const std::string& region,
                            int                pwTerm) override;

private:
    const Config& cfg_;     
};
