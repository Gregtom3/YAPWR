#include <TEntryList.h>
#include <TTree.h>
#include <TFile.h>
#include <TSystem.h>
#include <yaml-cpp/yaml.h>

#include <iostream>
#include <memory>
#include <sstream>

namespace util {

// join ["a>1","b<2"] → "a>1&&b<2"
inline std::string joinCuts(const YAML::Node& seq)
{
    std::ostringstream oss;
    for (std::size_t i = 0; i < seq.size(); ++i) {
        if (i) oss << "&&";
        oss << seq[i].as<std::string>();
    }
    return oss.str();
}

void loadEntryList(TTree* tree, const char* yamlPath)
{
    if (!tree) { std::cerr << "[loadEntryList] null TTree pointer\n"; return; }

    // --- 1) parse YAML -------------------------------------------------------
    YAML::Node cfg = YAML::LoadFile(yamlPath);

    // --- 2) decide which pair key matches the file or tree name -------------
    std::string searchStr = tree->GetCurrentFile() ?
                            tree->GetCurrentFile()->GetName() : tree->GetName();

    std::string pairKey;
    for (const auto& kv : cfg) {
        std::string key = kv.first.as<std::string>();
        if (key == "bin_variable") continue;
        if (searchStr.find("/" + key + "/") != std::string::npos) {
            pairKey = key;
            break;
        }
    }
    if (pairKey.empty()) {
        std::cerr << "[loadEntryList] could not infer pair name from "
                  << searchStr << '\n';
        return;
    }

    // --- 3) build logical cut expression ------------------------------------
    const YAML::Node& cutsNode = cfg[pairKey]["cuts"];
    if (!cutsNode || !cutsNode.IsSequence()) {
        std::cerr << "[loadEntryList] no cuts for " << pairKey << '\n';
        return;
    }
    std::string cutExpr = joinCuts(cutsNode);

    // --- 4) create entry list (Draw → tmp → clone) --------------------------
    TString tmpName = Form("elist_tmp_%s", pairKey.c_str());
    tree->Draw(Form(">>%s", tmpName.Data()), cutExpr.c_str(), "entrylist");

    auto* tmp = static_cast<TEntryList*>(gDirectory->Get(tmpName));
    if (!tmp) {
        std::cerr << "[loadEntryList] Draw failed for cuts '" << cutExpr << "'\n";
        return;
    }

    auto* elist = static_cast<TEntryList*>(tmp->Clone(Form("elist_%s", pairKey.c_str())));
    tree->SetEntryList(elist);

    std::cout << "[loadEntryList] " << pairKey << " → \"" << cutExpr
              << "\"  (" << elist->GetN() << " / " << tree->GetEntries()
              << " entries kept)\n";
}
};