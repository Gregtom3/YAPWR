// src/baryonContamination.C
#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <string>

void baryonContamination(const char* filePath, const char* treeName) {
    // 1) open file
    TFile* f = TFile::Open(filePath, "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "[baryonContamination] ERROR: cannot open " << filePath << "\n";
        return;
    }

    // 2) get tree
    TTree* t = dynamic_cast<TTree*>(f->Get(treeName));
    if (!t) {
        std::cerr << "[baryonContamination] ERROR: tree '" << treeName
                  << "' not found in " << filePath << "\n";
        f->Close();
        return;
    }

    // 3) total entries
    Long64_t nEntries = t->GetEntries();
    std::cout << "[baryonContamination] File: " << filePath
              << "  Tree: '" << treeName
              << "' → " << nEntries << " entries\n\n";

    // 4) define the four branches we want to scan
    std::vector<std::string> branchNames = {
        "trueparentpid_1",
        "trueparentpid_2",
        "trueparentparentpid_1",
        "trueparentparentpid_2"
    };

    // for each branch we keep a map<pid,count>
    std::vector<std::map<Int_t,Long64_t>> counts(branchNames.size());
    // and an array of storage for the current entry
    std::vector<Int_t> values(branchNames.size(), 0);

    // 5) attach branches
    for (size_t i = 0; i < branchNames.size(); ++i) {
        const char* nm = branchNames[i].c_str();
        if (t->GetBranch(nm)) {
            t->SetBranchStatus(nm, 1);
            t->SetBranchAddress(nm, &values[i]);
        } else {
            std::cerr << "[baryonContamination] WARNING: branch '"
                      << nm << "' not found, skipping\n";
            // leave its map empty and value unused
        }
    }

    // 6) loop over all entries
    for (Long64_t entry = 0; entry < nEntries; ++entry) {
        t->GetEntry(entry);
        for (size_t i = 0; i < branchNames.size(); ++i) {
            // if the branch existed, values[i] was filled
            if (t->GetBranch(branchNames[i].c_str())) {
                counts[i][ values[i] ]++;
            }
        }
    }

    // 7) for each branch, sort and print
    for (size_t i = 0; i < branchNames.size(); ++i) {
        const auto& name = branchNames[i];
        auto& mp        = counts[i];

        std::cout << "[baryonContamination] summary for '" << name << "':\n";
        if (mp.empty()) {
            std::cout << "   (no entries or branch missing)\n\n";
            continue;
        }

        // move to vector and sort by descending count
        std::vector<std::pair<Int_t,Long64_t>> vec;
        vec.reserve(mp.size());
        for (auto& kv : mp) vec.emplace_back(kv.first, kv.second);
        std::sort(vec.begin(), vec.end(),
                  [](auto &a, auto &b){ return a.second > b.second; });

        // print
        for (auto &p : vec) {
            std::cout << "   " << p.first << " : " << p.second << "\n";
        }
        std::cout << "\n";
    }

    f->Close();
}
