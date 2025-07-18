#pragma once
#include "Error.h"

class BinMigrationError : public Error {
public:
    std::string errorName() const override { return "binMigration"; }

    double getError(const Result& r,
                    const std::string& region,
                    int pwTerm) override;
};