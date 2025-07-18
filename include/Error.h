#pragma once

#include "Config.h"
#include "Constants.h"
#include "Logger.h"
#include "Result.h"

// Base class for Error types
class Error {
public:
    virtual ~Error() = default;

    virtual std::string errorName() const = 0;

    virtual double getRelativeError(const Result& r, const std::string& region, int pwTerm) = 0;

protected:
    int getTotalEntries(const Result& r) {
        int total_entries = 0;

        for (const auto& [key, val] : r.scalars) {
            if (key.rfind("total_entries", 0) == 0) {
                total_entries = val;
            }
        }

        if (total_entries == 0) {
            LOG_WARN("Total entries in Result found to be zero!");
        }

        return total_entries;
    }
};
