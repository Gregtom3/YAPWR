// src/baryonContamination.C
#include <TBranch.h>
#include <TFile.h>
#include <TString.h>
#include <TSystem.h>
#include <TTree.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "TreeManager.C"

// Write contamination counts to YAML
static void writeYaml(const std::string& path, Long64_t total, const std::vector<std::string>& branches,
                      const std::vector<std::map<Int_t, Long64_t>>& counts) {
    std::ofstream out(path);
    if (!out) {
        std::cerr << "[baryonContamination] ERROR: cannot open YAML output " << path << "\n";
        return;
    }
    out << "total_entries: " << total << "\n";
    for (size_t i = 0; i < branches.size(); ++i) {
        out << branches[i] << ":\n";
        if (counts[i].empty()) {
            out << "  {}\n";
            continue;
        }
        // sort by descending count
        std::vector<std::pair<Int_t, Long64_t>> vec(counts[i].begin(), counts[i].end());
        std::sort(vec.begin(), vec.end(), [](auto& a, auto& b) {
            return a.second > b.second;
        });
        for (auto& p : vec) {
            out << "  '" << p.first << "': " << p.second << "\n";
        }
    }
    out.close();
}

void baryonContamination(const char* filePath, const char* treeName, const char* cutYamlPath, const char* outYamlPath){
    TFile*  f = new TFile(filePath,"READ");
    TTree*  t = f->Get<TTree>(treeName);
    util::loadEntryList(t, cutYamlPath);
    std::cout << t->GetEntries("") << std::endl;

    // Attach MCMatch branch for filtering
    Int_t mcMatchVal = 0;
    bool hasMCMatch = false;
    if (auto b = t->GetBranch("MCmatch")) {
        hasMCMatch = true;
        t->SetBranchStatus("MCmatch", 1);
        t->SetBranchAddress("MCmatch", &mcMatchVal);
    } else {
        std::cerr << "[baryonContamination] WARNING: MCMatch branch not found; no filtering applied\n";
    }

    // Prepare branches and counts
    std::vector<std::string> branchNames = {"trueparentpid_1", "trueparentpid_2", "trueparentparentpid_1", "trueparentparentpid_2"};
    std::vector<std::map<Int_t, Long64_t>> counts(branchNames.size());
    std::vector<Int_t> values(branchNames.size(), 0);

    // Enable and attach branches
    for (size_t i = 0; i < branchNames.size(); ++i) {
        const char* nm = branchNames[i].c_str();
        if (t->GetBranch(nm)) {
            t->SetBranchStatus(nm, 1);
            t->SetBranchAddress(nm, &values[i]);
        }
    }

    // Loop entries
    Long64_t nEntries = t->GetEntries("");
    Long64_t nEntries_good = t->GetEntries("MCmatch==1");
    for (Long64_t entry = 0; entry < nEntries; ++entry) {
        t->GetEntry(entry);
        // skip entries failing MCMatch == 1
        if (hasMCMatch && mcMatchVal != 1)
            continue;
        for (size_t i = 0; i < branchNames.size(); ++i) {
            if (t->GetBranch(branchNames[i].c_str())) {
                counts[i][values[i]]++;
            }
        }
    }

    // Ensure YAML directory exists
    TString dir = gSystem->DirName(outYamlPath);
    gSystem->mkdir(dir, true);

    // Write YAML
    writeYaml(outYamlPath, nEntries_good, branchNames, counts);

    f->Close();
}
