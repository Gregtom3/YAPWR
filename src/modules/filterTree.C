// filterTree.C
#include <TFile.h>
#include <TSystem.h>
#include <TTree.h>
#include <iostream>
#include <yaml-cpp/yaml.h>

const std::vector<std::string>& keepBranches = {"x",
                                                "Q2",
                                                "y",
                                                "hel",
                                                "eps",
                                                "Mh",
                                                "M2",
                                                "Pol",
                                                "phi_h",
                                                "phi_R1",
                                                "th",
                                                "z",
                                                "xF",
                                                "Mx",
                                                "MCmatch",
                                                "pTtot",
                                                "truex",
                                                "trueQ2",
                                                "truey",
                                                "trueeps",
                                                "trueMh",
                                                "trueM2",
                                                "truephi_h",
                                                "truephi_R1",
                                                "trueth",
                                                "truez",
                                                "truexF",
                                                "trueMx",
                                                "truepTtot",
                                                "truepid_1",
                                                "truepid_2",
                                                "truepid_21",
                                                "truepid_22",
                                                "trueparentpid_1",
                                                "trueparentpid_2"};

void filterTree(const char* inputPath, const char* treeName, const char* configPath, const char* pairName, const char* outputDir,
                Int_t maxEntries = -1) {
    // 1) Read YAML
    YAML::Node cfg = YAML::LoadFile(configPath);
    auto cutsNode = cfg[pairName]["cuts"];
    std::string selection;
    for (auto expr : cutsNode) {
        if (!selection.empty())
            selection += " && ";
        selection += "(" + expr.as<std::string>() + ")";
    }
    std::cerr << "Selection: " << selection << "\n";

    // 2a) Open input
    TFile* inF = TFile::Open(inputPath, "READ");
    TTree* inT = (TTree*)inF->Get(treeName);
    const bool isMC = std::string(inputPath).find("MC_") != std::string::npos;
    // 2b) Keep only desired branches  ---------------------------------------
    inT->SetBranchStatus("*", 0); // disable all
    if (keepBranches.empty()) {
        std::cerr << "WARNING: keepBranches is empty; output tree will be empty\n";
    }
    for (const auto& br : keepBranches) {
        if (!isMC && br.find("true") != std::string::npos)
            continue; // Skip true data for non-MC
        inT->SetBranchStatus(br.c_str(), 1);
    }
    // 3) Prepare output file *first* and cd into it
    std::string fname = gSystem->BaseName(inputPath);
    gSystem->mkdir(outputDir, kTRUE);
    std::string outPath = std::string(outputDir) + "/" + fname;
    TFile* outF = TFile::Open(outPath.c_str(), "RECREATE");
    outF->cd(); // <-- make this file the current directory

    // 4) Now CopyTree() will create the new TTree in outF
    if (maxEntries == -1) {
        maxEntries = 1000000000;
    }
    TTree* outT = inT->CopyTree(selection.c_str(), "", maxEntries);
    outT->SetName(treeName);

    // 5) Write and close
    outT->Write();
    outF->Close();
    inF->Close();

    std::cout << "Wrote filtered tree to " << outPath << "\n";
}
