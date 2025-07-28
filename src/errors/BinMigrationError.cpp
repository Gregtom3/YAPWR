#include "BinMigrationError.h"
#include "Logger.h"
#include <TCanvas.h>
#include <TH2D.h>
#include <TStyle.h>
#include <TLatex.h>
#include <TColor.h>
#include <filesystem>
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <TDecompLU.h>
#include <TMatrixD.h>

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

    return M;
}


void BinMigrationError::plotSummary(const std::string& outDir, bool asFraction) const
{
    const int N = static_cast<int>(sortedCfgNames_.size());
    if (N <= 0) return;

    // ---------- labels (use same stem logic as elsewhere) ----------
    std::vector<std::string> labels; labels.reserve(N);
    for (const auto& name : sortedCfgNames_) labels.emplace_back(keyStem(name));

    // ---------- build counts matrix N_{i->j} and fractions f_{i->j} ----------
    // i = true (generated), j = reco (reconstructed)
    std::vector<std::vector<double>> Nij(N, std::vector<double>(N, 0.0));
    std::vector<double> Ngen(N, 0.0);

    for (int i = 0; i < N; ++i) {
        const std::string& cfg_i = sortedCfgNames_[i];
        auto itResI = binMig_.find(cfg_i);
        if (itResI == binMig_.end() || !itResI->second) continue;
        
        const Result& rI = *itResI->second;

        // total generated in bin i
        if (auto itN = rI.scalars.find("entries"); itN != rI.scalars.end() && itN->second > 0.0)
            Ngen[i] = itN->second;
        else
            continue;

        double offSum = 0.0;
        for (int j = 0; j < N; ++j) {
            const std::string suff_j = labels[j];
            if (j == i) continue;

            if (auto it = rI.scalars.find("other___" + suff_j); it != rI.scalars.end()) {
                const double n_ij = it->second;           // already a raw count
                Nij[i][j] = std::max(0.0, n_ij);
                offSum   += std::max(0.0, n_ij);
            }
        }
        // diagonal = "stayed" = N_i - sum_{j!=i} N_{i->j}, clamped
        Nij[i][i] = std::max(0.0, Ngen[i] - offSum);
    }

    // Optional: convert to fractions row-by-row if requested
    const char* title = nullptr;
    if (asFraction) {
        for (int i = 0; i < N; ++i) {
            const double Ni = Ngen[i];
            if (Ni <= 0) continue;
            for (int j = 0; j < N; ++j) Nij[i][j] /= Ni;
        }
        title = "Bin migration fractions f_{i #rightarrow j} (i=true, j=reco)";
    } else {
        title = "Bin migration entries N_{i #rightarrow j} (i=true, j=reco)";
    }

    // ---------- make TH2D (X=true, Y=reco) ----------
    auto hname = std::string("h2_binMigration_") + (asFraction ? "frac" : "counts");
    TH2D* H = new TH2D(hname.c_str(), title, N, 0.5, N + 0.5, N, 0.5, N + 0.5);

    // labels
    std::string binVar = cfg_.getBinVariable(); // ex: "x"
    for (int i = 0; i < N; ++i) { 
        std::string label = binVar + " bin " + std::to_string(i);
        H->GetXaxis()->SetBinLabel(i + 1, label.c_str()); // true
        H->GetYaxis()->SetBinLabel(i + 1, label.c_str()); // reco
    }
    H->GetXaxis()->SetTitle("Generated (true) bin");
    H->GetYaxis()->SetTitle("Reconstructed bin");

    // fill
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            H->SetBinContent(i + 1, j + 1, Nij[i][j]);

    // ---------- draw ----------
    gStyle->SetOptStat(0);
    gStyle->SetNumberContours(64);
    if (asFraction) gStyle->SetPaintTextFormat("0.3f");
    else            gStyle->SetPaintTextFormat("0.0f");

    TCanvas* c = new TCanvas((hname + "_c").c_str(), "", 900, 800);

    c->SetRightMargin(0.12);  // space for Z palette
    c->SetBottomMargin(0.18); // space for long x labels
    c->SetLeftMargin(0.20);   // space for long y labels

    H->GetXaxis()->LabelsOption("v");  // vertical x labels
    H->GetXaxis()->SetLabelSize(0.03);
    H->GetYaxis()->SetLabelSize(0.03);
    H->GetXaxis()->SetTitleOffset(2);
    H->Draw("COLZ TEXT");     // color map + numeric text overlay

    // ---------- save ----------
    const std::string base = outDir + "/" + (asFraction ? "binMigration_matrix_frac" : "binMigration_matrix_counts");
    c->SaveAs((base + ".png").c_str());
    c->SaveAs((base + ".pdf").c_str());
}


void BinMigrationError::saveMigrationDataToYaml(
        const std::string& outDir,
        int pwTerm,
        const std::unordered_map<std::string, double>& unalteredAsymValues) const
{
    namespace fs = std::filesystem;

    const int N = static_cast<int>(sortedCfgNames_.size());
    if (N <= 0) {
        LOG_WARN("saveMigrationDataToYaml: no bins to save");
        return;
    }

    // Ensure output directory exists
    std::error_code ec;
    fs::create_directories(outDir, ec);
    if (ec) {
        LOG_ERROR("saveMigrationDataToYaml: cannot create directory '" << outDir << "': " << ec.message());
        return;
    }

    // 1) Labels in the canonical (sorted) order
    std::vector<std::string> labels; labels.reserve(N);
    for (const auto& name : sortedCfgNames_) labels.emplace_back(keyStem(name));

    // 2) Build migration matrix (rows=reco j, cols=true i)
    TMatrixD M = getMigrationMatrix_RecoRows_TrueCols();

    // 3) Invert (LU)
    Bool_t ok = kFALSE;
    TDecompLU decomp(M);
    TMatrixD Minv = decomp.Invert(ok);

    // 4) Gather asymmetries (unaltered provided; altered from current map)
    //    Emit both as a map (keyed by label) and a vector in bin order.
    std::vector<double> A_raw_vec; A_raw_vec.reserve(N);
    std::vector<double> A_alt_vec; A_alt_vec.reserve(N);

    // Build maps with label keys for readability
    std::map<std::string, double> A_raw_map;
    std::map<std::string, double> A_alt_map;

    for (int k = 0; k < N; ++k) {
        const std::string& cfgName = sortedCfgNames_[k];
        const std::string& lab     = labels[k];

        // unaltered (may be missing → write YAML null)
        auto it_raw = unalteredAsymValues.find(cfgName);
        if (it_raw != unalteredAsymValues.end()) {
            A_raw_vec.push_back(it_raw->second);
            A_raw_map[lab] = it_raw->second;
        } else {
            A_raw_vec.push_back(std::numeric_limits<double>::quiet_NaN());
            // leave out of map on purpose; we’ll emit YAML null below if desired
        }

        // altered/current
        auto it_alt = asymValue_.find(cfgName);
        if (it_alt != asymValue_.end()) {
            A_alt_vec.push_back(it_alt->second);
            A_alt_map[lab] = it_alt->second;
        } else {
            A_alt_vec.push_back(std::numeric_limits<double>::quiet_NaN());
        }
    }

    // 5) Serialize to YAML
    YAML::Emitter out;
    out.SetFloatPrecision(16);  // keep precision for matrices

    out << YAML::BeginMap;

    // Metadata
    out << YAML::Key << "bin_variable" << YAML::Value << cfg_.getBinVariable();
    out << YAML::Key << "matrix_orientation"
        << YAML::Value << "rows = reconstructed (j), cols = true/generated (i); A_rec = M * A_true";

    // Bin order (labels)
    out << YAML::Key << "bin_order" << YAML::Value << YAML::Flow << YAML::BeginSeq;
    for (const auto& lab : labels) out << lab;
    out << YAML::EndSeq;

    // Matrix M
    out << YAML::Key << "binMigrationMatrix" << YAML::Value << YAML::BeginSeq;
    for (int r = 0; r < N; ++r) {
        out << YAML::Flow << YAML::BeginSeq;
        for (int c = 0; c < N; ++c) out << M(r, c);
        out << YAML::EndSeq;
    }
    out << YAML::EndSeq;

    // Inverse
    out << YAML::Key << "inversion_ok" << YAML::Value << static_cast<bool>(ok);
    if (ok) {
        out << YAML::Key << "binMigrationMatrix_inverse" << YAML::Value << YAML::BeginSeq;
        for (int r = 0; r < N; ++r) {
            out << YAML::Flow << YAML::BeginSeq;
            for (int c = 0; c < N; ++c) out << Minv(r, c);
            out << YAML::EndSeq;
        }
        out << YAML::EndSeq;
    }

    // Asymmetries: maps (for readability)
    out << YAML::Key << "unalteredAsymValues" << YAML::Value << YAML::BeginMap;
    for (const auto& lab : labels) {
        auto it = A_raw_map.find(lab);
        out << YAML::Key << lab;
        if (it != A_raw_map.end()) out << YAML::Value << it->second;
        else                       out << YAML::Value << YAML::Null;
    }
    out << YAML::EndMap;

    out << YAML::Key << "alteredAsymValues" << YAML::Value << YAML::BeginMap;
    for (const auto& lab : labels) {
        auto it = A_alt_map.find(lab);
        out << YAML::Key << lab;
        if (it != A_alt_map.end()) out << YAML::Value << it->second;
        else                       out << YAML::Value << YAML::Null;
    }
    out << YAML::EndMap;

    // Also provide vectors in bin order (handy for quick NumPy loads, etc.)
    out << YAML::Key << "unalteredAsymValues_vec" << YAML::Value << YAML::Flow << YAML::BeginSeq;
    for (double v : A_raw_vec) out << v;
    out << YAML::EndSeq;

    out << YAML::Key << "alteredAsymValues_vec" << YAML::Value << YAML::Flow << YAML::BeginSeq;
    for (double v : A_alt_vec) out << v;
    out << YAML::EndSeq;

    out << YAML::EndMap;

    // 6) Write file
    const fs::path outPath = fs::path(outDir) / ("binMigration_data___b_" + std::to_string(pwTerm) + ".yaml");
    std::ofstream fout(outPath);
    if (!fout) {
        LOG_ERROR("saveMigrationDataToYaml: unable to open '" << outPath.string() << "' for writing");
        return;
    }
    fout << out.c_str();
    fout.close();

    LOG_INFO("saveMigrationDataToYaml: wrote " << outPath.string());
}