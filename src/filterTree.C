// filterTree.C
#include <TFile.h>
#include <TTree.h>
#include <TSystem.h>
#include <yaml-cpp/yaml.h>
#include <iostream>

void filterTree(const char* inputPath,
                const char* treeName,
                const char* configPath,
                const char* pairName,
                const char* outputDir,
                Int_t       maxEntries = -1)
{
    // 1) Read YAML
    YAML::Node cfg = YAML::LoadFile(configPath);
    auto cutsNode = cfg[pairName]["cuts"];
    std::string selection;
    for (auto expr : cutsNode) {
      if (!selection.empty()) selection += " && ";
      selection += "(" + expr.as<std::string>() + ")";
    }
    std::cerr<<"Selection: "<<selection<<"\n";

    // 2) Open input
    TFile *inF = TFile::Open(inputPath,"READ");
    TTree *inT = (TTree*)inF->Get(treeName);

    // 3) Prepare output file *first* and cd into it
    std::string fname = gSystem->BaseName(inputPath);
    gSystem->mkdir(outputDir, kTRUE);
    std::string outPath = std::string(outputDir)+"/"+fname;
    TFile *outF = TFile::Open(outPath.c_str(),"RECREATE");
    outF->cd();                 // <-- make this file the current directory

    // 4) Now CopyTree() will create the new TTree in outF
    TTree *outT = inT->CopyTree(selection.c_str(),"",maxEntries);
    outT->SetName(treeName);

    // 5) Write and close
    outT->Write();
    outF->Close();
    inF->Close();

    std::cout<<"Wrote filtered tree to "<<outPath<<"\n";
}
