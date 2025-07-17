// ------------------------------------------------------------------
//  kinematicBins.C   – per-tree CSV with <value> of each numeric
//                      branch plus <P_lm(th)> ( l≤2 ).
//
//  call from Ruby:
//
//   root -l -b -q 'src/kinematicBins.C("file.root","T","piplus_pi0","dir")'
//
// ------------------------------------------------------------------
#include <TBranch.h>
#include <TFile.h>
#include <TLeaf.h>
#include <TMath.h>
#include <TSystem.h>
#include <TTree.h>
#include <TTreeFormula.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// ---------- helper: numeric leaves --------------------------------
static std::vector<TLeaf*> numericLeaves(TTree* t) {
    std::vector<TLeaf*> v;
    auto* L = t->GetListOfLeaves();
    for (int i = 0; i < L->GetEntries(); ++i) {
        auto* leaf = static_cast<TLeaf*>(L->At(i));
        std::string tn = leaf->GetTypeName();
        if (tn == "Double_t" || tn == "Float_t" || tn == "Int_t" || tn == "UInt_t" || tn == "Long64_t")
            v.push_back(leaf);
    }
    return v;
}

// ---------- Legendre P_lm (l≤2) -----------------------------------
static double P_lm(int l, int m, double ct, double st) {
    switch (l) {
    case 0:
        return 1.0;
    case 1:
        return (m == 0) ? ct : st; // ±1 share sin(th)
    case 2: {
        if (m == 0)
            return 0.5 * (3 * ct * ct - 1);
        if (std::abs(m) == 1)
            return std::sin(2 * std::acos(ct)); // 2·th
        if (std::abs(m) == 2)
            return st * st;
    }
    }
    return 0.0;
}

// ---------- average one region to CSV -----------------------------
static void writeCsv(TTree* t, const std::vector<TLeaf*>& leaves, const char* cut, const char* outfile) {
    std::unique_ptr<TTreeFormula> form; // nullptr if no cut
    if (cut && *cut)
        form.reset(new TTreeFormula("sel", cut, t));

    const Long64_t nTot = t->GetEntries();
    Long64_t nKept = 0;

    std::vector<double> sum(leaves.size(), 0.0);
    double Sp00 = 0, Sp10 = 0, Sp11 = 0, Sp1m1 = 0, Sp20 = 0, Sp21 = 0, Sp2m1 = 0, Sp22 = 0, Sp2m2 = 0;

    for (Long64_t i = 0; i < nTot; ++i) {
        t->GetEntry(i);
        if (form && form->EvalInstance() == 0)
            continue; // cut failed
        ++nKept;

        for (size_t j = 0; j < leaves.size(); ++j)
            sum[j] += leaves[j]->GetValue();

        double th = t->GetLeaf("th") ? t->GetLeaf("th")->GetValue() : 0;
        double ct = std::cos(th), st = std::sin(th);
        Sp00 += 1.0;
        Sp10 += P_lm(1, 0, ct, st);
        Sp11 += P_lm(1, 1, ct, st);
        Sp1m1 += P_lm(1, -1, ct, st);
        Sp20 += P_lm(2, 0, ct, st);
        Sp21 += P_lm(2, 1, ct, st);
        Sp2m1 += P_lm(2, -1, ct, st);
        Sp22 += P_lm(2, 2, ct, st);
        Sp2m2 += P_lm(2, -2, ct, st);
    }

    if (nKept == 0) {
        std::cerr << "[kinBins] " << outfile << " : 0 events after cut\n";
        return;
    }

    std::ofstream csv(outfile);
    // header row
    for (auto* l : leaves)
        csv << l->GetName() << ',';
    csv << "P0_0,P1_0,P1_1,P1_-1,P2_0,P2_1,P2_-1,P2_2,P2_-2\n";

    csv << std::fixed << std::setprecision(8);
    for (double s : sum)
        csv << s / nKept << ',';
    csv << Sp00 / nKept << ',' << Sp10 / nKept << ',' << Sp11 / nKept << ',' << Sp1m1 / nKept << ',' << Sp20 / nKept << ','
        << Sp21 / nKept << ',' << Sp2m1 / nKept << ',' << Sp22 / nKept << ',' << Sp2m2 / nKept << "\n";
    csv.close();

    std::cout << "  wrote " << outfile << "  (" << nKept << " events)\n";
}

// ---------- macro entry -------------------------------------------
void kinematicBins(const char* file, const char* treeName, const char* pair, const char* outDir) {
    TFile f(file, "READ");
    if (f.IsZombie()) {
        std::cerr << "[kinBins] bad file " << file << "\n";
        return;
    }
    TTree* t = static_cast<TTree*>(f.Get(treeName));
    if (!t) {
        std::cerr << "[kinBins] tree " << treeName << " missing\n";
        return;
    }

    gSystem->mkdir(outDir, true);
    auto leaves = numericLeaves(t);

    // full range
    writeCsv(t, leaves, "", (std::string(outDir) + "/full.csv").c_str());

    std::string p(pair);
    if (p == "piplus_pi0" || p == "piminus_pi0") {
        writeCsv(t, leaves, "M2>0.106 && M2<0.166", (std::string(outDir) + "/signal.csv").c_str());
        writeCsv(t, leaves, "M2>0.2 && M2<0.4", (std::string(outDir) + "/background.csv").c_str());
    }
    f.Close();
}
