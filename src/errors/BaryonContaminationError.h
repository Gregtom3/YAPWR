#pragma once
#include "Error.h"

class BaryonContaminationError : public Error {
public:
    std::string errorName() const override { return "baryonContamination"; }

    /// Return the relative error (e.g. 0.03 means 3 %).
    double getError(const Result& r,
                    const std::string& region,
                    int pwTerm) override;
};