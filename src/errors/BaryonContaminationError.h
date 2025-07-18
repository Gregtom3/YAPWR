#pragma once
#include <string>
#include "Error.h"

struct BCEntry {
    std::string prefix;   // "trueparentpid_1" …
    int         pid;      // 3122, -999, …
    double      count;    // the value in Result::scalars
};


class BaryonContaminationError : public Error {
public:
    explicit BaryonContaminationError(Config &cfg);
    std::string errorName() const override { return "baryonContamination"; }

    /// Return the relative error (e.g. 0.03 means 3 %).
    double getError(const Result& r,
                    const std::string& region,
                    int pwTerm) override;

private:
    std::vector<BCEntry> parseBaryonContamination(const Result& r);
    Config cfg_;
};