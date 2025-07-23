// ------------------------------------------------------------------
//  kinematicBins_RDF.C   – same interface/output as kinematicBins.C
//                          but accelerated with ROOT::RDataFrame
// ------------------------------------------------------------------
#include <ROOT/RDataFrame.hxx>
#include <TBranch.h>
#include <TEntryList.h>
#include <TFile.h>
#include <TLeaf.h>
#include <TStopwatch.h>
#include <TSystem.h>
#include <TTree.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "TreeManager.C" // util::loadEntryList

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

// ------------------------------------------------------------------
//  One region → one CSV (RDataFrame variant)
// ------------------------------------------------------------------
static void writeCsvRDF(TTree* t, const std::vector<TLeaf*>& leaves, const std::string& cut, const std::string& outfile,
                        const char* cutYamlPath) {
    // --------------------------------- entry list (YAML) ------------
    util::loadEntryList(t, cutYamlPath);
    TEntryList* elist = t->GetEntryList();

    // --------------------------------- enable IMT & build RDF -------
    ROOT::EnableImplicitMT(); // comment out for single‑thread
    ROOT::RDataFrame df(*t);  // respects active entry‑list

    // --------------------------------- optional cut -----------------
    ROOT::RDF::RNode rdf = df;
    if (!cut.empty())
        rdf = rdf.Filter(cut);

    // --------------------------------- Legendre helpers -------------
    rdf = rdf.Define("ct", "cos(th)")
              .Define("st", "sin(th)")
              .Define("P00", "1.0")
              .Define("P10", "ct")
              .Define("P11", "st")
              .Define("P1m1", "st")
              .Define("P20", "0.5*(3*ct*ct - 1)")
              .Define("P21", "2*st*ct")
              .Define("P2m1", "2*st*ct")
              .Define("P22", "st*st")
              .Define("P2m2", "st*st");

    // --------------------------------- schedule computations --------
    std::vector<ROOT::RDF::RResultPtr<double>> leafMeans;
    leafMeans.reserve(leaves.size());
    for (auto* l : leaves)
        leafMeans.emplace_back(rdf.Mean(l->GetName()));

    auto nKept = rdf.Count(); // entry count after cut
    auto mP00 = rdf.Mean("P00");
    auto mP10 = rdf.Mean("P10");
    auto mP11 = rdf.Mean("P11");
    auto mP1m1 = rdf.Mean("P1m1");
    auto mP20 = rdf.Mean("P20");
    auto mP21 = rdf.Mean("P21");
    auto mP2m1 = rdf.Mean("P2m1");
    auto mP22 = rdf.Mean("P22");
    auto mP2m2 = rdf.Mean("P2m2");

    // --------------------------------- trigger & time ---------------
    TStopwatch sw;
    sw.Start();
    const ULong64_t n = *nKept; // triggers the whole loop
    sw.Stop();

    if (n == 0) {
        std::cerr << "[kinBins] " << outfile << " : 0 events after cut\n";
        return;
    }

    // --------------------------------- write CSV --------------------
    std::ofstream csv(outfile);
    for (auto* l : leaves)
        csv << l->GetName() << ',';
    csv << "P0_0,P1_0,P1_1,P1_-1,P2_0,P2_1,P2_-1,P2_2,P2_-2\n";

    csv << std::fixed << std::setprecision(8);
    for (auto& m : leafMeans)
        csv << *m << ',';
    csv << *mP00 << ',' << *mP10 << ',' << *mP11 << ',' << *mP1m1 << ',' << *mP20 << ',' << *mP21 << ',' << *mP2m1 << ',' << *mP22
        << ',' << *mP2m2 << '\n';
    csv.close();

    std::cout << "  wrote " << outfile << "  (" << n << " events, " << sw.RealTime() << " s wall‑time, " << sw.CpuTime()
              << " s CPU)\n";
}

// ------------------------------------------------------------------
//  macro entry (same signature as before)
// ------------------------------------------------------------------
void kinematicBins(const char* file, const char* treeName, const char* pair, const char* cutYamlPath, const char* outDir) {
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
    writeCsvRDF(t, leaves, "", std::string(outDir) + "/full.csv", cutYamlPath);

    // special regions for π±π0 pairs
    std::string p(pair);
    if (p == "piplus_pi0" || p == "piminus_pi0") {
        writeCsvRDF(t, leaves, "M2>0.106 && M2<0.166", std::string(outDir) + "/signal.csv", cutYamlPath);

        writeCsvRDF(t, leaves, "M2>0.2 && M2<0.4", std::string(outDir) + "/background.csv", cutYamlPath);
    }
    f.Close();
}
