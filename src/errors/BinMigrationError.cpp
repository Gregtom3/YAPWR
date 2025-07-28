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

    logMigrationMatrix_();
    
    return relError;
}



void BinMigrationError::logMigrationMatrix_() const {
    const int n = static_cast<int>(sortedCfgNames_.size());
    if (n <= 0) return;

    // Build display labels using the same stem logic
    std::vector<std::string> labels; labels.reserve(n);
    int labw = 0;
    for (const auto& name : sortedCfgNames_) {
        std::string s = keyStem(name);
        labw = std::max(labw, static_cast<int>(s.size()));
        labels.push_back(std::move(s));
    }

    // M[i][j] = f_{i -> j}
    std::vector<std::vector<double>> M(n, std::vector<double>(n, 0.0));

    for (int i = 0; i < n; ++i) {
        auto itResI = binMig_.find(sortedCfgNames_[i]);
        if (itResI == binMig_.end() || !itResI->second) continue;

        const Result& rI = *itResI->second;

        const auto itNgen = rI.scalars.find("entries");
        if (itNgen == rI.scalars.end() || itNgen->second <= 0) continue;

        const double Ngen_i = itNgen->second;

        double sumOther = 0.0;
        for (int j = 0; j < n; ++j) {
            if (j == i) continue; // off-diagonals first
            const std::string key = "other___" + labels[j];
            const auto it = rI.scalars.find(key);
            if (it != rI.scalars.end()) {
                const double f_ij = it->second / Ngen_i;
                M[i][j] = f_ij;
                sumOther += f_ij;
            }
        }

        // Diagonal as the complement of the "other" fractions.
        // Clamp to [0, 1] to avoid tiny negative values from rounding.
        const double fii = std::max(0.0, 1.0 - sumOther);
        M[i][i] = std::min(1.0, fii);
    }

    // Pretty print
    std::ostringstream os;
    os.setf(std::ios::fixed);
    os << std::setprecision(4);

    const int nameW = std::max(labw, 8);
    const int cellW = std::max(8, labw);

    os << "Bin-migration matrix f_{i,j} (rows = generated i, columns = reconstructed j)\n";
    os << std::setw(nameW) << "gen\\rec";
    for (int j = 0; j < n; ++j) os << ' ' << std::setw(cellW) << labels[j];
    os << '\n';

    for (int i = 0; i < n; ++i) {
        os << std::setw(nameW) << labels[i];
        double rowSum = 0.0;
        for (int j = 0; j < n; ++j) {
            rowSum += M[i][j];
            os << ' ' << std::setw(cellW) << M[i][j];
        }
        os << "   | row_sum=" << std::setw(6) << std::setprecision(4) << rowSum << '\n';
    }

    LOG_INFO(os.str());
}


TMatrixD BinMigrationError::getMigrationMatrix_RecoRows_TrueCols() const {
    const int N = static_cast<int>(sortedCfgNames_.size());
    TMatrixD M(N, N);  // rows: reconstructed j, cols: true i
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c)
            M(r, c) = 0.0;

    // First build f_{i->j} with rows=true(i), cols=reco(j)
    std::vector<std::vector<double>> F_true_reco(N, std::vector<double>(N, 0.0));

    // Fill off-diagonals from "other___<suffix_j>", then set diagonal as complement
    for (int i = 0; i < N; ++i) {
        const std::string& cfg_i = sortedCfgNames_[i];
        const auto itResI = binMig_.find(cfg_i);
        if (itResI == binMig_.end() || !itResI->second) continue;

        const Result& rI = *itResI->second;
        const auto itNgen = rI.scalars.find("entries");
        if (itNgen == rI.scalars.end() || itNgen->second <= 0) continue;
        const double Ngen_i = itNgen->second;

        const std::string suff_i = keyStem(cfg_i);

        double sumOther = 0.0;
        for (int j = 0; j < N; ++j) {
            const std::string& cfg_j = sortedCfgNames_[j];
            const std::string suff_j = keyStem(cfg_j);
            if (j == i) continue;

            double f_ij = 0.0;
            if (auto it = rI.scalars.find("other___" + suff_j); it != rI.scalars.end()) {
                f_ij = it->second / Ngen_i;
            }
            F_true_reco[i][j] = f_ij;
            sumOther += f_ij;
        }
        // diagonal as complement; clamp to [0,1] for numerical safety
        double fii = std::max(0.0, 1.0 - sumOther);
        F_true_reco[i][i] = std::min(1.0, fii);
    }

    // Transpose into M with rows=reco(j), cols=true(i):
    // A_rec(j) = sum_i M(j,i) * A_true(i)
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            M(j, i) = F_true_reco[i][j];

    M.Print();
    return M;
}