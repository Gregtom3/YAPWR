// filterTree.C
#include <TFile.h>
#include <TSystem.h>
#include <TTree.h>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <regex>            // for transformCut

// ------------------------------------------------------------------
// Prefix every standalone variable with "true", except keywords.
// Skips tokens that already begin with "true" or are logical ops.
// ------------------------------------------------------------------
static const std::regex var_re(R"(\b(?!true|and|or|not)([A-Za-z_][A-Za-z0-9_]*)\b)");

static std::string transformCut(const std::string& in) {
    return std::regex_replace(in, var_re, "true$1");
}

void filterTreeMC(const char* inputPath,
                  const char* treeName,
                  const char* configPath,
                  const char* pairName,
                  const char* outputDir,
                  Int_t      maxEntries = -1)
{
    // 1) Read YAML
    YAML::Node cfg       = YAML::LoadFile(configPath);
    auto      cutsNode   = cfg[pairName]["cuts"];

    // 2) Build selection string *with* transformCut
    std::string selection;
    for (auto expr : cutsNode) {
        std::string raw = expr.as<std::string>();
        std::string cut = transformCut(raw);
        if (!selection.empty())
            selection += " && ";
        selection += "(" + cut + ")";
    }
    std::cerr << "Transformed selection: " << selection << "\n";

    // 3) Open input
    TFile* inF = TFile::Open(inputPath, "READ");
    TTree* inT = static_cast<TTree*>(inF->Get(treeName));

    // 4) Prepare output
    std::string fname   = gSystem->BaseName(inputPath);
    gSystem->mkdir(outputDir, kTRUE);
    std::string outPath = std::string(outputDir) + "/" + "gen_" + fname;
    TFile*    outF     = TFile::Open(outPath.c_str(), "RECREATE");
    outF->cd();

    // 5) Copy with maxEntries guard
    if (maxEntries < 0) maxEntries = INT_MAX;
    TTree* outT = inT->CopyTree(selection.c_str(), "", maxEntries);
    outT->SetName(treeName);

    // 6) Write & close
    outT->Write();
    outF->Close();
    inF->Close();

    std::cout << "Wrote filtered tree to " << outPath << "\n";
}
