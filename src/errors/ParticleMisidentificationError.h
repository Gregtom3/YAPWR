#pragma once
#include <vector>
#include <string>
#include "Error.h"
#include "Config.h"

struct PMEntry {
    std::string prefix;   // "truepid_1" , "truepid_21", ...
    int         pid;      // 211, -321, …
    double      count;    // associated scalar value
};

class ParticleMisidentificationError : public Error {
public:
    explicit ParticleMisidentificationError(Config& cfg);

    std::string errorName() const override { return "particleMisidentification"; }

    double getRelativeError(const Result& r,
                            const std::string& region,
                            int pwTerm) override;

private:
    std::vector<PMEntry> parseEntries(const Result& r) const;
    static double getTotalEntries(const Result& r);

    Config cfg_;
};