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

    virtual double getError(const Result& r, const std::string& region, int pwTerm) = 0;
};
