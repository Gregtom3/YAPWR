#include <TFile.h>
#include <TIterator.h>
#include <TList.h>
#include <TSystem.h>
#include <TSystemDirectory.h>
#include <TSystemFile.h>
#include <TTree.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

// ------------------------------------------------------------------
// Dump a YAML section for `key`, preserving indentation.
// Writes each trimmed line prefixed by `indent` to `out`.
// ------------------------------------------------------------------
static void dumpYamlSection(const std::string& path, const std::string& key, std::ostream& out, const std::string& indent = "  ") {
    std::ifstream in(path);
    if (!in) {
        out << indent << "# ERROR: cannot open " << path << "\n";
        return;
    }

    std::string line;
    bool inSection = false;
    int baseIndent = 0;

    while (std::getline(in, line)) {
        int indentCount = 0;
        while (indentCount < (int)line.size() && line[indentCount] == ' ')
            ++indentCount;
        std::string trimmed = line.substr(indentCount);

        if (!inSection) {
            if (trimmed.rfind(key + ":", 0) == 0) {
                inSection = true;
                baseIndent = indentCount;
                out << indent << trimmed << "\n";
            }
        } else {
            if (indentCount > baseIndent) {
                int relIndent = indentCount - baseIndent;
                out << indent << std::string(relIndent, ' ') << trimmed << "\n";
            } else {
                break;
            }
        }
    }
}

// ------------------------------------------------------------------
// Parse the list under `key:` → `cuts:` in a YAML file.
// Returns vector of the raw cut strings (without leading "- " or quotes).
// ------------------------------------------------------------------
static std::vector<std::string> parseCuts(const std::string& path, const std::string& key) {
    std::vector<std::string> cuts;
    std::ifstream in(path);
    if (!in)
        return cuts;

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
                if (!trimmed.empty())
                    cuts.push_back(trimmed);
            } else {
                break;
            }
        }
    }

    return cuts;
}

// ------------------------------------------------------------------
// Prefix every standalone variable with "true", except keywords.
// Skips tokens that already begin with "true" or are logical ops.
// ------------------------------------------------------------------
static const std::regex var_re(R"(\b(?!true|and|or|not)([A-Za-z_][A-Za-z0-9_]*)\b)");

static std::string transformCut(const std::string& in) {
    //return std::regex_replace(in, var_re, "true$1");
    return in; // 07/18/2025 no changen needed, using split config TTrees based on generated kinematics
}

// ------------------------------------------------------------------
// Find all top‐level config_<NAME>/<NAME>.yaml under `projectDir`.
// ------------------------------------------------------------------
static std::vector<std::string> findAllConfigYamls(const std::string& projectDir) {
    std::vector<std::string> yamls;
    TSystemDirectory projD("proj", projectDir.c_str());
    TList* configs = projD.GetListOfFiles();
    if (!configs)
        return yamls;

    TIter cIt(configs);
    while (auto* ent = (TSystemFile*)cIt()) {
        const char* name = ent->GetName();
        if (!ent->IsDirectory() || strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || strncmp(name, "config_", 7) != 0)
            continue;

        std::string key = std::string(name).substr(7);
        std::string yamlPath = projectDir + "/" + name + "/" + key + ".yaml";
        if (gSystem->AccessPathName(yamlPath.c_str()) == 0) {
            yamls.push_back(yamlPath);
        }
    }
    return yamls;
}

// ------------------------------------------------------------------
// Main entry point for ROOT:
//   root -l -b -q 'src/binMigration.C("filtered.root","tree",
//                                      "primary.yaml","out/project",
//                                      "out/project/report.yaml")'
// ------------------------------------------------------------------
void binMigration(const char* filePath, const char* treeName, const char* primaryYaml, const char* projectDir, const char* yamlPath) {
    // Ensure output directory exists
    TString outDir = gSystem->DirName(yamlPath);
    gSystem->mkdir(outDir, true);

    std::ofstream out(yamlPath);
    if (!out) {
        std::cerr << "[binMigration] ERROR: cannot open " << yamlPath << "\n";
        return;
    }

    // 1) Open ROOT file & TTree
    TFile* f = TFile::Open(filePath, "READ");
    TTree* t = (f && !f->IsZombie()) ? dynamic_cast<TTree*>(f->Get(treeName)) : nullptr;
    Long64_t totalEntries = t ? t->GetEntries() : 0;

    // 2) Top‐level metadata
    out << "file:    \"" << filePath << "\"\n";
    out << "tree:    \"" << treeName << "\"\n";
    out << "entries: " << totalEntries << "\n\n";

    // 3) Derive pionPair
    const char* leafDir = gSystem->DirName(filePath);
    const char* parentDir = gSystem->DirName(leafDir);
    std::string pionPair = gSystem->BaseName(parentDir);
    out << "pion_pair: \"" << pionPair << "\"\n\n";

    // 4) PRIMARY YAML section
    out << "primary_config: \"" << primaryYaml << "\"\n";
    out << "primary_section:\n";
    dumpYamlSection(primaryYaml, pionPair, out, "  ");
    out << "\n";

    // 5) PRIMARY cuts & count
    {
        auto primCuts = parseCuts(primaryYaml, pionPair);
        if (!primCuts.empty() && t) {
            std::string expr = "MCmatch==1 && ";
            for (size_t i = 0; i < primCuts.size(); ++i) {
                if (i)
                    expr += " && ";
                expr += transformCut(primCuts[i]);
            }
            out << "primary_cuts_expr: \"" << expr << "\"\n";
            Long64_t nPrim = t->GetEntries(expr.c_str());
            out << "primary_passing:   " << nPrim << "\n\n";
        }
    }

    // 6) OTHER configs
    auto yamls = findAllConfigYamls(projectDir);
    out << "other_configs:\n";
    for (auto& y : yamls) {
        if (y == primaryYaml)
            continue;

        out << "- config: \"" << y << "\"\n";
        out << "  section:\n";
        dumpYamlSection(y, pionPair, out, "    ");

        auto cuts = parseCuts(y, pionPair);
        if (cuts.empty() || !t) {
            out << "  note: \"no cuts found or tree missing\"\n\n";
            continue;
        }

        std::string expr;
        for (size_t i = 0; i < cuts.size(); ++i) {
            if (i)
                expr += " && ";
            expr += transformCut(cuts[i]);
        }
        out << "  transformed_expr: \"" << expr << "\"\n";
        Long64_t nPass = t->GetEntries(expr.c_str());
        out << "  passing:          " << nPass << "\n\n";
    }

    if (f)
        f->Close();
    out.close();
}
