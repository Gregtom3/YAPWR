#include "ParticleMisidentificationError.h"
#include "Logger.h"
#include <limits>
#include <cmath>

ParticleMisidentificationError::ParticleMisidentificationError(Config& cfg)
  : cfg_(cfg) {}

double ParticleMisidentificationError::getRelativeError(const Result& r,
                                                        const std::string& /*region*/,
                                                        int /*pwTerm*/)
{
    const bool isPi0 = cfg_.contains_pi0();
    const int hadron1 = cfg_.getPid1();
    const int hadron2 = cfg_.getPid2();
    // gather raw truepid_*_* counts
    const auto entries = parseEntries(r);
    const int   Ntot   = static_cast<int>(getTotalEntries(r));

    if (Ntot == 0) {
        LOG_WARN(errorName() << ": total_entries == 0 → returning 0");
        return 0.0;
    }

    int misID = 0;

    for (const auto& e : entries) {
        const std::string& p = e.prefix;
        const int gen_pid    = e.pid;
        // skip pi0‑specific or non‑pi0‑specific prefixes
        if (isPi0 && p == "truepid_2")      continue;
        if (!isPi0 && (p == "truepid_21" || p == "truepid_22")) continue;

        // skip including correctly assigned pids
        if ((p == "truepid_21" || p == "truepid_22")&&gen_pid==22) continue;
        if ((p == "truepid_1" && gen_pid == hadron1) || (p=="truepid_2" && gen_pid == hadron2) || (p=="truepid_e" && gen_pid == 11)) continue;
        misID += static_cast<int>(e.count);
    }

    const double frac = static_cast<double>(misID) / Ntot;
    if (frac >= 1.0) {
        LOG_ERROR(errorName() << ": misID fraction >= 1, returning NaN");
        return std::numeric_limits<double>::quiet_NaN();
    }

    return frac / (1.0 - frac);   // rel error
}

/* ---------- helpers -------------------------------------------------- */

std::vector<PMEntry> ParticleMisidentificationError::parseEntries(const Result& r) const
{
    std::vector<PMEntry> out;

    for (const auto& [key, val] : r.scalars) {
        // keep only truepid_*_* keys – but skip _11 and _12 entirely
        if (key.rfind("truepid_", 0) != 0)        continue;
        if (key.rfind("truepid_11", 0) == 0)      continue;
        if (key.rfind("truepid_12", 0) == 0)      continue;
        std::size_t p1 = key.find('_', 7);   // position after "truepid_"
        std::size_t p2 = key.rfind('_');     // last underscore
        if (p1 == std::string::npos || p2 == p1)  continue;

        std::string prefix = key.substr(0, p2);           // up to last '_'
        int         pid    = std::stoi(key.substr(p2+1));

        out.push_back({ prefix, pid, val });
    }
    return out;
}

double ParticleMisidentificationError::getTotalEntries(const Result& r)
{
    auto it = r.scalars.find("total_entries");
    return (it != r.scalars.end()) ? it->second : 0.0;
}
