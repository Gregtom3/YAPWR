void filterTree(const char* inputPath,
                const char* treeName,
                const char* configPath,
                const char* pairName,
                const char* outputDir)
{
    // --- 1. Load YAML config and select the node for this pair ----------------
    YAML::Node config;
    try {
        config = YAML::LoadFile(configPath);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: cannot load YAML '"<< configPath << "': " 
                  << e.what() << "\n";
        return;
    }
    if (!config[pairName]) {
        std::cerr << "ERROR: no entry for pair '"<< pairName 
                  << "' in " << configPath << "\n";
        return;
    }
    YAML::Node cutsNode = config[pairName]["cuts"];
    if (!cutsNode || !cutsNode.IsSequence()) {
        std::cerr << "ERROR: '"<< pairName <<".cuts' must be a YAML sequence\n";
        return;
    }

    // Build selection string: cut0 && cut1 && ...
    std::string selection;
    for (auto exprNode : cutsNode) {
        std::string expr = exprNode.as<std::string>();
        if (!selection.empty()) selection += " && ";
        selection += "(" + expr + ")";
    }
    std::cerr << "[filterTree] ["<< pairName <<"] selection: " 
              << selection << "\n";

    // --- 2. Open input file & get tree ---------------------------------------
    TFile* inFile = TFile::Open(inputPath, "READ");
    if (!inFile || inFile->IsZombie()) {
        std::cerr << "ERROR: cannot open '"<< inputPath << "'\n";
        return;
    }
    TTree* tree = dynamic_cast<TTree*>(inFile->Get(treeName));
    if (!tree) {
        std::cerr << "ERROR: tree '"<< treeName 
                  << "' not found in '"<< inputPath << "'\n";
        inFile->Close();
        return;
    }

    // --- 3. Copy the tree with cuts ------------------------------------------
    TTree* newTree = tree->CopyTree(selection.c_str());
    if (!newTree) {
        std::cerr << "ERROR: CopyTree failed\n";
        inFile->Close();
        return;
    }

    // --- 4. Prepare output directory & file name ----------------------------
    // e.g. if inputPath="/some/dir/data.root", fname="data.root"
    std::string fname = gSystem->BaseName(inputPath);
    if (gSystem->AccessPathName(outputDir)) {
        // recursive mkdir
        if (gSystem->mkdir(outputDir, kTRUE) != 0) {
            std::cerr << "ERROR: cannot create dir '"<< outputDir << "'\n";
            inFile->Close();
            return;
        }
    }
    std::string outPath = std::string(outputDir) + "/" + fname;

    // --- 5. Write the filtered tree -----------------------------------------
    TFile* outFile = TFile::Open(outPath.c_str(), "RECREATE");
    newTree->SetName(treeName);
    newTree->Write();
    outFile->Close();
    inFile->Close();

    std::cout << "[filterTree] wrote filtered tree for '"<< pairName 
              << "' to " << outPath << "\n";
}
