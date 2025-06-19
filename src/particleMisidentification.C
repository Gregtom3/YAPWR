#include <TFile.h>
#include <TTree.h>
#include <TSystem.h>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

// Macro entry:
void particleMisidentification(const char* filePath,
			       const char* treeName,
			       const char* yamlPath) {
  // 1) ensure output directory exists
  {
    std::string dir = std::string(yamlPath);
    auto pos = dir.find_last_of("/\\");
    if (pos != std::string::npos) {
      gSystem->mkdir(dir.substr(0, pos).c_str(), true);
    }
  }

  // 2) open YAML file
  std::ofstream out(yamlPath);
  if (!out) {
    std::cerr << "[particleMisidentification] ERROR: cannot open " << yamlPath << "\n";
    return;
  }

  // 3) open ROOT file & tree
  TFile* f = TFile::Open(filePath, "READ");
  if (!f || f->IsZombie()) {
    out << "error: cannot open file: " << filePath << "\n";
    return;
  }
  TTree* t = dynamic_cast<TTree*>(f->Get(treeName));
  if (!t) {
    out << "error: tree '" << treeName << "' not found in " << filePath << "\n";
    f->Close();
    return;
  }

  // 4) total entries
  Long64_t nEntries = t->GetEntries();
  out << "total_entries: " << nEntries << "\n";

  // 5) branches to scan
  std::vector<std::string> branchNames = {
    "truepid_e",
    "truepid_1",
    "truepid_2",
    "truepid_11",
    "truepid_12",
    "truepid_21",
            "truepid_22"
  };

  // prepare maps and storage
  std::vector< std::map<Int_t, Long64_t> > counts(branchNames.size());
  std::vector<Int_t> values(branchNames.size(), 0);

  // 6) attach branches
  for (size_t i = 0; i < branchNames.size(); ++i) {
    const char* nm = branchNames[i].c_str();
    if (t->GetBranch(nm)) {
      t->SetBranchStatus(nm, 1);
      t->SetBranchAddress(nm, &values[i]);
    } else {
      // branch missing: leave map empty
    }
  }

  // 7) loop over entries
  for (Long64_t entry = 0; entry < nEntries; ++entry) {
    t->GetEntry(entry);
    for (size_t i = 0; i < branchNames.size(); ++i) {
      if (t->GetBranch(branchNames[i].c_str())) {
	counts[i][ values[i] ]++;
      }
    }
  }

  // 8) write each branch’s counts under its YAML key
  for (size_t i = 0; i < branchNames.size(); ++i) {
    const auto& name = branchNames[i];
    out << name << ":\n";
    const auto& mp = counts[i];
    if (mp.empty()) {
      out << "  {}\n";
      continue;
    }
    // sort by descending count
    std::vector< std::pair<Int_t, Long64_t> > vec(mp.begin(), mp.end());
    std::sort(vec.begin(), vec.end(),
	      [](auto &a, auto &b){ return a.second > b.second; });
    // emit pid: count
    for (auto &p : vec) {
      out << "  \"" << p.first << "\": " << p.second << "\n";
    }
  }

  f->Close();
  out.close();
}
