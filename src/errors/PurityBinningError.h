#pragma once
#include "Error.h"
#include "Config.h"

class PurityBinningError : public Error {
public:
    PurityBinningError(const Config& cfg, const double asymValue);

    std::string errorName() const override { return "purityBinning"; }


    double getRelativeError(const Result&      r,
                            const std::string& region,
                            int                pwTerm) override;

private:
    const Config& cfg_; 
    const double asymValue_;
    static inline std::string DEFAULT_BKG_REGION="sideband_M2_0.2_M2_0.4___";
};
