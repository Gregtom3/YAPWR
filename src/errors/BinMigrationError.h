#pragma once
#include "Error.h"

class BinMigrationError : public Error {
public:

    explicit BinMigrationError(Config &cfg);
    std::string errorName() const override { return "binMigration"; }

    double getError(const Result& r,
                    const std::string& region,
                    int pwTerm) override;

private:
    Config cfg_;
};