// src/purityBinning.C
#include <TFile.h>
#include <TTree.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>

//----------------------------------------------------------------------
// Helper: two‐level, equal‐occupancy binning
//----------------------------------------------------------------------
//   phi_h:   full vector of φₕ
//   phi_R1:  full vector of φ_R₁ (same length)
//   N:       number of φₕ bins
//   M:       number of φ_R₁ bins inside each φₕ slice
static void binEqualOcc2(const std::vector<double>& phi_h,
                         const std::vector<double>& phi_R1,
                         int N, int M)
{
    size_t n = phi_h.size();
    if (n == 0 || phi_R1.size() != n) {
        std::cerr << "[binEqualOcc2] ERROR: vectors must be same non-zero length\n";
        return;
    }

    // 1) Compute φₕ edges by quantiles
    std::vector<double> h_sorted = phi_h;
    std::sort(h_sorted.begin(), h_sorted.end());
    std::vector<double> h_edges(N+1);
    for (int i = 0; i <= N; ++i) {
        size_t idx = std::min(n - 1, size_t(i * n / N));
        h_edges[i] = h_sorted[idx];
    }

    // Print φₕ edges
    std::cout << "\n=== φₕ edges (equal‐occupancy):\n  ";
    for (double e : h_edges) std::cout << std::fixed << std::setprecision(3) << e << " ";
    std::cout << "\n";

    // 2) Loop over each φₕ bin
    for (int i = 0; i < N; ++i) {
        double h_low  = h_edges[i];
        double h_high = h_edges[i+1];

        // collect φ_R₁ for events in this φₕ bin
        std::vector<double> rvals;
        for (size_t k = 0; k < n; ++k) {
            if (phi_h[k] >= h_low && phi_h[k] < h_high) {
                rvals.push_back(phi_R1[k]);
            }
        }
        size_t rn = rvals.size();

        std::cout << "\nφₕ‐bin ["<< i <<"] = [" 
                  << h_low << ", " << h_high 
                  << ")  →  " << rn 
                  << " events\n";

        if (rn == 0) continue;

        // 3) Compute φ_R₁ edges by quantiles
        std::sort(rvals.begin(), rvals.end());
        std::vector<double> r_edges(M+1);
        for (int j = 0; j <= M; ++j) {
            size_t idx = std::min(rn - 1, size_t(j * rn / M));
            r_edges[j] = rvals[idx];
        }

        // Print φ_R₁ edges
        std::cout << "  φ_R₁ edges:\n    ";
        for (double e : r_edges) std::cout << std::fixed << std::setprecision(3) << e << " ";
        std::cout << "\n";

        // 4) Count occupancy in each φ_R₁ sub‐bin
        std::cout << "  counts per φ_R₁ sub‐bin:\n";
        for (int j = 0; j < M; ++j) {
            size_t cnt = std::count_if(rvals.begin(), rvals.end(),
                [&](double rv){
                    return rv >= r_edges[j] && rv < r_edges[j+1];
                });
            std::cout << "    ["<< i <<"]["<< j <<"] = " << cnt << "\n";
        }
    }
}

//----------------------------------------------------------------------
// Main macro
//----------------------------------------------------------------------
// Reads only φₕ and φ_R₁ from the tree, then does two‐level equal‐
// occupancy binning with N×M bins.
void purityBinning(const char* inputPath,
                   const char* treeName,
                   const char* pairName,
                   int N = 8,
                   int M = 10)
{
    // 1) Open
    TFile* f = TFile::Open(inputPath, "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "[purityBinning] ERROR: cannot open " << inputPath << "\n";
        return;
    }

    // 2) Get tree
    TTree* t = dynamic_cast<TTree*>(f->Get(treeName));
    if (!t) {
        std::cerr << "[purityBinning] ERROR: tree '"<< treeName 
                  << "' not found in " << inputPath << "\n";
        f->Close();
        return;
    }

    // 3) Entry count
    Long64_t nEntries = t->GetEntries();
    std::cout << "[purityBinning] Pair [" << pairName 
              << "] has " << nEntries 
              << " entries in '" << treeName << "'\n";

    // 4) Enable only the two needed branches
    t->SetBranchStatus("*", 0);
    double phi_h_val = 0, phi_R1_val = 0;
    t->SetBranchStatus("phi_h",  1); t->SetBranchAddress("phi_h",  &phi_h_val);
    t->SetBranchStatus("phi_R1", 1); t->SetBranchAddress("phi_R1", &phi_R1_val);

    // 5) Read into vectors
    std::vector<double> phH, phR;
    phH.reserve(nEntries);
    phR.reserve(nEntries);
    for (Long64_t i = 0; i < nEntries; ++i) {
        t->GetEntry(i);
        phH.push_back(phi_h_val);
        phR.push_back(phi_R1_val);
    }

    f->Close();

    // 6) Perform two‐level equal‐occupancy binning
    binEqualOcc2(phH, phR, N, M);
}
