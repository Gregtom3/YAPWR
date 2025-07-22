// src/purityBinning.C
#include <TCanvas.h>
#include <TF1.h>
#include <TFile.h>
#include <TFitResultPtr.h>
#include <TH1F.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <utility>
#include <vector>

namespace {

struct BinInfo {
    std::vector<double> rEdges;    // size M+1
    std::vector<double> purity;    // size M
    std::vector<double> purityErr; // size M
};

/// — Propagate uncertainty of a ratio p = s/t
inline double ratioErr(double s, double ds, double t, double dt) {
    return (t > 0) ? std::sqrt(std::pow(ds / t, 2) + std::pow(s * dt / (t * t), 2)) : 0.0;
}

} // anonymous namespace

//==============================================================================
//  Helper for one grid --------------------------------------------------------
//==============================================================================
static void doGrid(const std::vector<double>& phi_h, const std::vector<double>& phi_R1, const std::vector<double>& m2, int N, int M,
                   const char* outputDir, const char* pairName,
                   std::vector<double>& hEdgesOut, // N+1
                   std::vector<BinInfo>& binTable) // N elements
{
    const size_t n = phi_h.size();

    // ---------------------------------------------------------------------------
    // φₕ edges (equal occupancy)
    // ---------------------------------------------------------------------------
    hEdgesOut.resize(N + 1);
    std::vector<double> tmp = phi_h;
    std::sort(tmp.begin(), tmp.end());
    for (int i = 0; i <= N; ++i) {
        hEdgesOut[i] = tmp[std::min(n - 1, size_t(i * n / N))];
    }

    // prepare canvas
    TCanvas c(Form("c_%s_%dx%d", pairName, N, M), Form("Purity %s %dx%d", pairName, N, M), M * 500, N * 500);
    c.Divide(M, N, 0, 0);
    gStyle->SetOptStat(0);

    // storage
    binTable.resize(N);

    // ---------------------------------------------------------------------------
    // loop φₕ–bins
    // ---------------------------------------------------------------------------
    for (int i = 0; i < N; ++i) {
        double hlo = hEdgesOut[i], hhi = hEdgesOut[i + 1];

        // gather (φ_R1,M2) pairs for this slice
        std::vector<std::pair<double, double>> slice;
        slice.reserve(n);
        for (size_t k = 0; k < n; ++k)
            if (phi_h[k] >= hlo && phi_h[k] < hhi)
                slice.emplace_back(phi_R1[k], m2[k]);

        size_t nn = slice.size();
        if (!nn)
            continue;

        // sort by φ_R1 to get quantiles
        std::sort(slice.begin(), slice.end(), [](auto& a, auto& b) {
            return a.first < b.first;
        });

        BinInfo info;
        info.rEdges.resize(M + 1);
        info.purity.resize(M);
        info.purityErr.resize(M);

        for (int j = 0; j <= M; ++j)
            info.rEdges[j] = slice[std::min(nn - 1, size_t(j * nn / M))].first;

        // loop φ_R1 sub-bins
        for (int j = 0; j < M; ++j) {
            // -------------------------------- histogram -------------------------
            TH1F* h = new TH1F(Form("h_%d_%d_%d_%d", N, M, i, j), "", 100, 0.06, 0.4);
            h->SetDirectory(nullptr);
            for (auto& p : slice)
                if (p.first >= info.rEdges[j] && p.first < info.rEdges[j + 1])
                    h->Fill(p.second);
            h->Sumw2();
            double area = h->Integral();
            if (area > 0)
                h->Scale(1.0 / area);

            // -------------------------------- fit ------------------------------
            TF1* fit = new TF1("fit", "gaus(0)+pol4(3)", 0.06, 0.4);
            fit->SetParameters(0.005, 0.15, 0.01);
            fit->SetParLimits(0, 0.001, 1);
            fit->SetParLimits(1, 0.1, 0.2);
            fit->SetParLimits(2, 0.001, 0.1);
            TFitResultPtr r = h->Fit(fit, "QRN"); // quiet range, no draw

            TF1* sig = new TF1("sig", "gaus", 0.06, 0.4);
            sig->SetParameters(fit->GetParameter(0), fit->GetParameter(1), fit->GetParameter(2));

            // integrals + errors
            double tot_int = fit->Integral(0.106, 0.166);
            double tot_err = fit->IntegralError(0.106, 0.166);
            double sig_int = sig->Integral(0.106, 0.166);

            double sig_err = std::sqrt(std::pow(std::sqrt(2 * M_PI) * sig->GetParameter(2) * sig->GetParError(0), 2) +
                                       std::pow(sig->GetParameter(0) * std::sqrt(2 * M_PI) * sig->GetParError(2), 2));

            double purity = tot_int > 0 ? sig_int / tot_int : 0;
            double purity_err = ratioErr(sig_int, sig_err, tot_int, tot_err);

            info.purity[j] = purity;
            info.purityErr[j] = purity_err;

            // -------------------------------- draw -----------------------------
            int pad = i * M + j + 1;
            c.cd(pad);
            h->SetLineColor(kBlack);
            h->SetLineWidth(2);
            fit->SetLineColor(kRed);
            fit->SetLineWidth(2);
            sig->SetLineColor(kBlue);
            sig->SetLineWidth(2);
            h->Draw("hist");
            fit->Draw("same");
            sig->Draw("same");

            TLatex L;
            L.SetNDC();
            L.SetTextSize(0.05);
            L.DrawLatex(0.5, 0.9, Form("%.3f<#phi_{h}<%.3f", hlo, hhi));
            L.DrawLatex(0.5, 0.85, Form("%.3f<#phi_{R}<%.3f", info.rEdges[j], info.rEdges[j + 1]));
            L.DrawLatex(0.5, 0.8, Form("#mu=%.3f#pm%.3f", fit->GetParameter(1), fit->GetParError(1)));
            L.DrawLatex(0.5, 0.75, Form("#sigma=%.3f#pm%.3f", fit->GetParameter(2), fit->GetParError(2)));
            L.DrawLatex(0.5, 0.7, Form("#chi^{2}/ndf=%.2f", (fit->GetNDF() ? fit->GetChisquare() / fit->GetNDF() : 0)));
            L.SetTextColor(kBlue);
            L.DrawLatex(0.5, 0.65, Form("purity=%.3f#pm%.3f", purity, purity_err));
            L.SetTextColor(kBlack);
            L.DrawLatex(0.5, 0.6, Form("Events = %.0f", h->GetEntries()));
        } // j loop

        binTable[i] = std::move(info);
    } // i loop

    // save canvas
    gSystem->mkdir(outputDir, true);
    c.SaveAs(Form("%s/purity_%s_%dx%d.png", outputDir, pairName, N, M));
}

//==============================================================================
//  Main macro -----------------------------------------------------------------
//==============================================================================
void purityBinning(const char* inputPath, const char* treeName, const char* pairName, const char* outputDir) {
    // open file in UPDATE so we can add branches
    TFile* f = TFile::Open(inputPath, "UPDATE");
    if (!f || f->IsZombie()) {
        std::cerr << "Cannot open " << inputPath << "\n";
        return;
    }
    TTree* t = (TTree*)f->Get(treeName);
    if (!t) {
        std::cerr << "Tree " << treeName << " not found\n";
        f->Close();
        return;
    }

    // read φₕ, φ_R₁, M2 into vectors
    t->SetBranchStatus("*", 0);
    double ph = 0, pr = 0, m2 = 0;
    t->SetBranchStatus("phi_h", 1);
    t->SetBranchAddress("phi_h", &ph);
    t->SetBranchStatus("phi_R1", 1);
    t->SetBranchAddress("phi_R1", &pr);
    t->SetBranchStatus("M2", 1);
    t->SetBranchAddress("M2", &m2);

    const Long64_t nEnt = t->GetEntries();
    std::vector<double> vph, vpr, vm2;
    vph.reserve(nEnt);
    vpr.reserve(nEnt);
    vm2.reserve(nEnt);
    for (Long64_t i = 0; i < nEnt; ++i) {
        t->GetEntry(i);
        vph.push_back(ph);
        vpr.push_back(pr);
        vm2.push_back(m2);
    }

    t->SetBranchStatus("*", 1);
    // the grids we want
    const std::vector<std::pair<int, int>> grids = {{1, 1}, {3, 3}, {5, 5}, {8, 8}, {12, 12}};

    // for each grid, compute purity table AND add (or overwrite) two branches
    for (auto gm : grids) {
        int N = gm.first, M = gm.second;

        // compute purity tables + draw
        std::vector<double> hEdges;
        std::vector<BinInfo> table;
        doGrid(vph, vpr, vm2, N, M, outputDir, pairName, hEdges, table);

        // ----------------------------------------------------------------------
        // (re)create branches
        // ----------------------------------------------------------------------
        std::string bName = Form("purity_%d_%d", N, M);
        std::string beName = Form("purity_err_%d_%d", N, M);
        double purity_val = 0.0;
        double purity_err = 0.0;

        // if branch exists, delete then recreate
        if (t->GetBranch(bName.c_str()))
            t->SetBranchStatus(bName.c_str(), 0);
        if (t->GetBranch(beName.c_str()))
            t->SetBranchStatus(beName.c_str(), 0);

        TBranch* bp = t->Branch(bName.c_str(), &purity_val, "p/D");
        TBranch* bpe = t->Branch(beName.c_str(), &purity_err, "pe/D");

        // fill event-wise
        for (Long64_t ie = 0; ie < nEnt; ++ie) {
            // find bin indices
            int iH = std::upper_bound(hEdges.begin(), hEdges.end(), vph[ie]) - hEdges.begin() - 1;
            if (iH < 0 || iH >= N) {
                purity_val = -1;
                purity_err = -1;
            } else {
                const auto& rE = table[iH].rEdges;
                int iR = std::upper_bound(rE.begin(), rE.end(), vpr[ie]) - rE.begin() - 1;
                if (iR < 0 || iR >= M) {
                    purity_val = -1;
                    purity_err = -1;
                } else {
                    purity_val = table[iH].purity[iR];
                    purity_err = table[iH].purityErr[iR];
                }
            }
            bp->Fill();
            bpe->Fill();
        }
    } // grids loop
    t->SetBranchStatus("*", 1);
    // write tree back
    t->Write("", TObject::kOverwrite);
    f->Close();
}
