#include "BinMigrationError.h"
#include "Logger.h"

namespace {
constexpr int K_NEAREST_NEIGHBORS = 3;   // hard limit ±3 bins
/* strip a leading "config_" if present → matches YAML stem keys         */
inline std::string keyStem(std::string name) {
    return name.rfind("config_",0)==0 ? name.substr(7) : std::move(name);
}
}




BinMigrationError::BinMigrationError(const Config& cfg,
                                     const std::map<std::string,Config>&           cfgMap,
                                     const std::vector<std::string>&               sortedCfgNames,
                                     const std::unordered_map<std::string,double>& asymValue,
                                     const std::unordered_map<std::string,const Result*>& binMig)
  : cfg_           (cfg)
  , configMap_     (cfgMap)
  , sortedCfgNames_(sortedCfgNames)
  , asymValue_     (asymValue)
  , binMig_        (binMig)
{}

double BinMigrationError::getRelativeError(const Result&      rSelf,
                                           const std::string& /*region*/,
                                           int                /*pw*/)
{
    rSelf.print(Logger::FORCE);
    /* ----------------------------------------------------------------
     * 0) basic sanity
     * ---------------------------------------------------------------- */
    const auto itNgen = rSelf.scalars.find("entries");
    if (itNgen==rSelf.scalars.end() || itNgen->second<=0) return 0.0;
    const double Ngen_i = itNgen->second;

    const double Ai = asymValue_.at(cfg_.name);
    if (std::abs(Ai)<1e-14) return 0.0;

    /* ----------------------------------------------------------------
     * 1) locate i in ordered list
     * ---------------------------------------------------------------- */
    const int nBins = static_cast<int>(sortedCfgNames_.size());
    const auto itSelf = std::find(sortedCfgNames_.begin(),
                                  sortedCfgNames_.end(),
                                  cfg_.name);
    if (itSelf == sortedCfgNames_.end()) return 0.0;
    const int idx_i  = static_cast<int>(itSelf - sortedCfgNames_.begin());

    /* key suffix used in r.scalars */
    const std::string suffix_i = keyStem(cfg_.name);

    /* ----------------------------------------------------------------
     * 2) accumulate Σ( ± f · A )
     * ---------------------------------------------------------------- */
    double deltaA = 0.0;
    std::ostringstream dbg;
    bool firstTerm = true;
    for (int off = -K_NEAREST_NEIGHBORS; off<=K_NEAREST_NEIGHBORS; ++off) {
        if (off==0) continue;
        const int idx_j = idx_i + off;
        if (idx_j<0 || idx_j>=nBins)      continue;

        const std::string& cfg_j = sortedCfgNames_[idx_j];
        const std::string  suff_j= keyStem(cfg_j);

        /* ------------------------------------------------------------
         * 2a)  f_{i→j}  – from *this* Result
         * ------------------------------------------------------------ */
        double f_ij = 0.0;
        {
            const auto it = rSelf.scalars.find("other___"+suff_j);
            if (it!=rSelf.scalars.end())
                f_ij = it->second / Ngen_i;
        }

        /* ------------------------------------------------------------
         * 2b)  f_{j→i}  – need neighbour Result
         * ------------------------------------------------------------ */
        auto itResJ = binMig_.find(cfg_j);
        if (itResJ == binMig_.end()) continue;                 // no info
        const Result& rJ = *itResJ->second;

        const auto itNgenJ = rJ.scalars.find("entries");
        if (itNgenJ==rJ.scalars.end() || itNgenJ->second<=0) continue;
        const double Ngen_j = itNgenJ->second;

        double f_ji = 0.0;
        {
            const auto it = rJ.scalars.find("other___"+suffix_i);
            if (it!=rJ.scalars.end())
                f_ji = it->second / Ngen_j;
        }

        /* asymmetry in neighbour bin */
        const double Aj = asymValue_.at(cfg_j);

        /* ΔA contribution */
        deltaA += f_ji*Aj - f_ij*Ai;

        if (!firstTerm) dbg << " + ";
        dbg << "(" << f_ji << " * " << Aj << ")" << "-" << "(" << f_ij << " * " << Ai << ")";
        firstTerm = false;
    }

    /* ----------------------------------------------------------------
     * 3) convert to relative |ΔA| / |A|
     * ---------------------------------------------------------------- */
    double relError = std::abs(deltaA) / std::abs(Ai);
    // ------------------------------------------------------------------
    // 4) debug print
    // ------------------------------------------------------------------
    std::ostringstream msg;
    msg << "BinMigration ΔA_" << cfg_.name << " = " << dbg.str()
        << " = " << deltaA << "   (|ΔA|/|A| = " << relError << ')';
    LOG_DEBUG(msg.str());
    
    return relError;
}