#include <vector>
#include <tuple>
#include "BaryonContaminationError.h"
#include "Logger.h"

BaryonContaminationError::BaryonContaminationError(Config& cfg)
    : cfg_(cfg) {}

double BaryonContaminationError::getRelativeError(const Result& r,
                                                  const std::string& region,
                                                  int pwTerm)
{
    const bool isPi0 = cfg_.contains_pi0();
    auto entries = parseBaryonContamination(r);
    const int total_entries = getTotalEntries(r);

    if (total_entries == 0) {
        LOG_WARN(errorName() << ": total_entries == 0 → returning 0");
        return 0.0;
    }

    int totalBaryonParents = 0;
    
    for (const auto& e : entries) {
        std::string prefix = e.prefix;
        // Pi0 never appears in the first slot so ignore
        if(prefix=="trueparentparentpid_1"){
            continue;
        }
        // Only allow certain baryonParents to contribute depending on pi0
        // skip the following
        if(isPi0&&prefix=="trueparentpid_2"){
            continue;
        }
        if(!isPi0&&prefix=="trueparentparentpid_2"){
            continue;
        }

        
        int parentpid      = e.pid;
        int count          = e.count;

        // skip proton
        if (parentpid == 2212){
            continue;
        }
        
        // If parent is a baryon...
        auto it = Constants::particlePalette().find(parentpid);
        if (it != Constants::particlePalette().end()) {
            // Add to the total
            if(it->second.isBaryon)
                totalBaryonParents+=count;
        }
    }

    // Calculate relative uncertainty
    double ratio    = totalBaryonParents*1.0/total_entries;
    double relError = (ratio)/(1-ratio);
    return relError;
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