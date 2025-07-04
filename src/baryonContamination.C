// src/baryonContamination.C
#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TSystem.h>
#include <TString.h>
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
        if (counts[i].empty()) {
            out << "  {}\n";
            continue;
        }
        // sort by descending count
        std::vector<std::pair<Int_t,Long64_t>> vec(counts[i].begin(), counts[i].end());
        std::sort(vec.begin(), vec.end(),
                  [](auto &a, auto &b){ return a.second > b.second; });
        for (auto& p : vec) {
            out << "  '" << p.first << "': " << p.second << "\n";
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

    // Attach MCMatch branch for filtering
    Int_t  mcMatchVal = 0;
    bool   hasMCMatch = false;
    if (auto b = t->GetBranch("MCmatch")) {
        hasMCMatch = true;
        t->SetBranchStatus("MCmatch", 1);
        t->SetBranchAddress("MCmatch", &mcMatchVal);
    } else {
        std::cerr << "[baryonContamination] WARNING: MCMatch branch not found; no filtering applied\n";
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

    // Enable and attach branches
    for (size_t i = 0; i < branchNames.size(); ++i) {
        const char* nm = branchNames[i].c_str();
        if (t->GetBranch(nm)) {
            t->SetBranchStatus(nm, 1);
            t->SetBranchAddress(nm, &values[i]);
        }
    }

    // Loop entries
    Long64_t nEntries = t->GetEntries();
    Long64_t nEntries_good = t->GetEntries("MCmatch==1");
    for (Long64_t entry = 0; entry < nEntries; ++entry) {
        t->GetEntry(entry);
        // skip entries failing MCMatch == 1
        if (hasMCMatch && mcMatchVal != 1) continue;
        for (size_t i = 0; i < branchNames.size(); ++i) {
            if (t->GetBranch(branchNames[i].c_str())) {
                counts[i][ values[i] ]++;
            }
        }
    }

    // Ensure YAML directory exists
    TString dir = gSystem->DirName(yamlPath);
    gSystem->mkdir(dir, true);

    // Write YAML
    writeYaml(yamlPath, nEntries_good, branchNames, counts);

    f->Close();
}
