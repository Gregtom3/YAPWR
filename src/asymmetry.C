#include <TFile.h>
#include <TTree.h>
#include <iostream>

/// Simple template: open the file, load the tree, and print the entry count.
void asymmetry(const char* inputPath,
               const char* treeName,
               const char* pairName)
{
    // open file
    TFile* f = TFile::Open(inputPath, "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "[asymmetry] ERROR: cannot open " << inputPath << "\n";
        return;
    }

    // get tree
    TTree* t = dynamic_cast<TTree*>(f->Get(treeName));
    if (!t) {
        std::cerr << "[asymmetry] ERROR: tree '" << treeName
                  << "' not found in " << inputPath << "\n";
        f->Close();
        return;
    }

    // print number of events
    Long64_t n = t->GetEntries();
    std::cout << "[asymmetry] Pair [" << pairName
              << "] has " << n << " entries\n";

    f->Close();
}
