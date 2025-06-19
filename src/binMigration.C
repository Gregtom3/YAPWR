#include <TSystem.h>
#include <TFile.h>
#include <TTree.h>
#include <TSystemDirectory.h>
#include <TSystemFile.h>
#include <TList.h>
#include <TIterator.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <cstring>

// ------------------------------------------------------------------
// Dump a YAML section for key, with indent preserved
// ------------------------------------------------------------------
static void dumpYamlSection(const std::string& path,
                            const std::string& key,
                            const std::string& prefix="    ") {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "[binMigration] ERROR: cannot open " << path << "\n";
        return;
    }
    std::string line;
    bool inSection = false;
    int baseIndent = 0;
    while (std::getline(in, line)) {
        int indent = 0;
        while (indent < (int)line.size() && line[indent] == ' ')
            ++indent;
        std::string trimmed = line.substr(indent);
        if (!inSection) {
            if (trimmed.rfind(key + ":", 0) == 0) {
                inSection = true;
                baseIndent = indent;
                std::cout << prefix << trimmed << "\n";
            }
        } else {
            if (indent > baseIndent) {
                std::cout << prefix << trimmed << "\n";
            } else {
                break;
            }
        }
    }
}

// ------------------------------------------------------------------
// Parse the "cuts:" list under key in a YAML file
// ------------------------------------------------------------------
static std::vector<std::string> parseCuts(const std::string& path,
                                          const std::string& key) {
    std::vector<std::string> cuts;
    std::ifstream in(path);
    if (!in) return cuts;

    std::string line;
    bool inSection = false, inCuts = false;
    int baseIndent = 0, cutsIndent = 0;

    auto trim = [](std::string& s) {
        size_t a = s.find_first_not_of(" \"-");
        size_t b = s.find_last_not_of(" \"");
        if (a == std::string::npos || b == std::string::npos) {
            s.clear();
        } else {
            s = s.substr(a, b - a + 1);
        }
    };

    while (std::getline(in, line)) {
        int indent = 0;
        while (indent < (int)line.size() && line[indent] == ' ')
            ++indent;
        std::string trimmed = line.substr(indent);

        if (!inSection) {
            if (trimmed.rfind(key + ":", 0) == 0) {
                inSection = true;
                baseIndent = indent;
            }
        } else if (!inCuts) {
            if (indent > baseIndent && trimmed.rfind("cuts:", 0) == 0) {
                inCuts = true;
                cutsIndent = indent;
            }
        } else {
            if (indent > cutsIndent && trimmed.rfind("-", 0) == 0) {
                trim(trimmed);
                if (!trimmed.empty()) cuts.push_back(trimmed);
            } else {
                break;
            }
        }
    }
    return cuts;
}

// ------------------------------------------------------------------
// Prefix every standalone variable name with "true", skipping tokens
// already beginning with "true".
// ------------------------------------------------------------------
static std::string transformCut(const std::string& in) {
    static const std::regex var_re(R"(\b(?!true)([A-Za-z_][A-Za-z0-9_]*)\b)");
    return std::regex_replace(in, var_re, "true$1");
}

// ------------------------------------------------------------------
// Find all top‐level config_<NAME>/<NAME>.yaml under projectDir
// ------------------------------------------------------------------
static std::vector<std::string> findAllConfigYamls(const std::string& projectDir) {
    std::vector<std::string> yamls;
    TSystemDirectory projD("proj", projectDir.c_str());
    TList* configs = projD.GetListOfFiles();
    if (!configs) return yamls;

    TIter cIt(configs);
    while (auto* cfgEnt = (TSystemFile*)cIt()) {
        const char* cfgName = cfgEnt->GetName();
        if (!cfgEnt->IsDirectory() ||
            strcmp(cfgName,".")==0 ||
            strcmp(cfgName,"..")==0 ||
            strncmp(cfgName,"config_",7)!=0) continue;

        std::string name = std::string(cfgName).substr(7);
        std::string yaml = projectDir + "/" + cfgName + "/" + name + ".yaml";
        if (gSystem->AccessPathName(yaml.c_str()) == 0) {
            yamls.push_back(yaml);
        }
    }
    return yamls;
}

// ------------------------------------------------------------------
// Called by ROOT as:
//   root -l -b -q 'src/binMigration.C("filtered.root","treeName",
//                                      "config_X/config_X.yaml","out/project")'
// ------------------------------------------------------------------

void binMigration(const char* filePath,
                  const char* treeName,
                  const char* primaryYaml,
                  const char* projectDir)
{
    // 1) Open filtered file & TTree
    TFile* f = TFile::Open(filePath, "READ");
    TTree* t = f && !f->IsZombie()
               ? dynamic_cast<TTree*>(f->Get(treeName))
               : nullptr;

    std::cout << "=== FILTERED FILE ===\n"
              << "file:    " << filePath << "\n"
              << "tree:    " << treeName << "\n"
              << "entries: " << (t ? t->GetEntries() : 0) << "\n\n";

    // 2) Derive pionPair
    const char* leafDir   = gSystem->DirName(filePath);
    const char* parentDir = gSystem->DirName(leafDir);
    const char* pionPairC = gSystem->BaseName(parentDir);
    std::string pionPair(pionPairC);
    std::cout << "pion pair: " << pionPair << "\n\n";

    // 3) Dump PRIMARY YAML section
    std::cout << "=== PRIMARY YAML (" << primaryYaml << ") ===\n";
    dumpYamlSection(primaryYaml, pionPair);
    std::cout << "\n";

    // 4) Parse, transform, and apply PRIMARY cuts
    {
        auto primCuts = parseCuts(primaryYaml, pionPair);
        if (!primCuts.empty() && t) {
            // build transformed expression
            std::string expr;
            for (size_t i = 0; i < primCuts.size(); ++i) {
                if (i) expr += " && ";
                expr += transformCut(primCuts[i]);
            }
            std::cout << "=== PRIMARY TRANSFORMED CUTS ===\n"
                      << expr << "\n";
            Long64_t nPrim = t->GetEntries(expr.c_str());
            std::cout << "entries passing primary cuts: " << nPrim << "\n\n";
        }
    }

    // 5) For each OTHER config YAML: do the same
    auto yamls = findAllConfigYamls(projectDir);
    std::cout << "=== OTHER CONFIGS for pion pair \"" << pionPair << "\" ===\n";
    for (auto& y : yamls) {
        if (y == primaryYaml) continue;

        std::cout << "-- " << y << "\n";
        dumpYamlSection(y, pionPair);

        auto cuts = parseCuts(y, pionPair);
        if (cuts.empty() || !t) {
            std::cout << "    (no cuts found or tree missing)\n\n";
            continue;
        }
        std::string expr;
        for (size_t i = 0; i < cuts.size(); ++i) {
            if (i) expr += " && ";
            expr += transformCut(cuts[i]);
        }
        std::cout << "    transformed cuts: " << expr << "\n";
        Long64_t nPass = t->GetEntries(expr.c_str());
        std::cout << "    entries passing:   " << nPass << "\n\n";
    }

    if (f) f->Close();
}
