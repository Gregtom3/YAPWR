#include <vector>
#include <tuple>
#include "BaryonContaminationError.h"
#include "Logger.h"

BaryonContaminationError::BaryonContaminationError(Config& cfg)
    : cfg_(cfg) {}

double BaryonContaminationError::getError(const Result& r,
                                          const std::string& region,
                                          int pwTerm)
{
    r.print(Logger::FORCE);
    const std::string key = region + ".b_" + std::to_string(pwTerm) + "_relerr";

    auto it = r.scalars.find(key);
    if (it != r.scalars.end()) {
        return it->second;                 // use the value saved by the processor
    }

    // Fallback: default to 3% relative error
    constexpr double kFallback = 0.03;
    LOG_WARN("BaryonContaminationError: missing '" << key
              << "' in Result → defaulting to " << kFallback);

    auto entries = parseBaryonContamination(r);
    
    for (const auto& e : entries) {
        LOG_ERROR(e.prefix << "  pid=" << e.pid << "  count=" << e.count);
    }
    return kFallback;
}


std::vector<BCEntry> BaryonContaminationError::parseBaryonContamination(const Result& r)
{
    std::vector<BCEntry> out;

    for (const auto& [key, val] : r.scalars) {
        if (   key.rfind("trueparentpid_"        , 0) == 0
            || key.rfind("trueparentparentpid_", 0) == 0) {

            const std::size_t pos = key.rfind('_');      // last underscore
            if (pos == std::string::npos) continue;      // malformed key?

            const std::string prefix = key.substr(0, pos);       // up to last ‘_’
            const int         pid    = std::stoi(key.substr(pos + 1));

            out.push_back({ prefix, pid, val });
        }
    }
    return out;
}