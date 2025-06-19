// src/baryonContamination.C
#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TSystem.h>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

// Write contamination counts to YAML
static void writeYaml(const std::string& path,
                      Long64_t total,
                      const std::vector<std::string>& branches,
                      const std::vector<std::map<Int_t,Long64_t>>& counts) {
    std::ofstream out(path);
    if (!out) {
        std::cerr << "[baryonContamination] ERROR: cannot open YAML output " << path << "\n";
        return;
    }
    out << "total_entries: " << total << "\n";
    for (size_t i = 0; i < branches.size(); ++i) {
        out << branches[i] << ":\n";
        for (auto it = counts[i].begin(); it != counts[i].end(); ++it) {
            out << "  '" << it->first << "': " << it->second << "\n";
        }
    }
    out.close();
}

void baryonContamination(const char* filePath,
                         const char* treeName,
                         const char* yamlPath) {
    // Open ROOT file
    TFile* f = TFile::Open(filePath, "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "[baryonContamination] ERROR: cannot open file " << filePath << "\n";
        return;
    }
    // Get tree
    TTree* t = dynamic_cast<TTree*>(f->Get(treeName));
    if (!t) {
        std::cerr << "[baryonContamination] ERROR: tree '" << treeName
                  << "' not found in " << filePath << "\n";
        f->Close();
        return;
    }
    // Prepare branches and counts
    std::vector<std::string> branchNames = {
        "trueparentpid_1",
        "trueparentpid_2",
        "trueparentparentpid_1",
        "trueparentparentpid_2"
    };
    std::vector<std::map<Int_t,Long64_t>> counts(branchNames.size());
    std::vector<Int_t> values(branchNames.size(), 0);
    // Enable branches
    for (size_t i = 0; i < branchNames.size(); ++i) {
        if (t->GetBranch(branchNames[i].c_str())) {
            t->SetBranchStatus(branchNames[i].c_str(), 1);
            t->SetBranchAddress(branchNames[i].c_str(), &values[i]);
        }
    }
    // Loop entries
    Long64_t nEntries = t->GetEntries();
    for (Long64_t entry = 0; entry < nEntries; ++entry) {
        t->GetEntry(entry);
        for (size_t i = 0; i < branchNames.size(); ++i) {
            if (t->GetBranch(branchNames[i].c_str())) {
                counts[i][ values[i] ]++;
            }
        }
    }
    // Ensure yaml directory exists
    TString dir = gSystem->DirName(yamlPath);
    gSystem->mkdir(dir, true);
    // Write YAML
    writeYaml(yamlPath, nEntries, branchNames, counts);
    f->Close();
}
