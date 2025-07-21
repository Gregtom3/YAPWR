#include <TEntryList.h>
#include <TTree.h>
#include <TFile.h>
#include <TSystem.h>
#include <yaml-cpp/yaml.h>

#include <iostream>
#include <memory>
#include <regex>
#include <sstream>

namespace util
{
// ------------------------------------------------------------------
//  Regex: standalone identifiers      Mh ,  x1 ,  var_42
//         but not                     trueMh ,  and ,  or ,  not
// ------------------------------------------------------------------
static const std::regex var_re(R"(\b(?!true|and|or|not)([A-Za-z_][A-Za-z0-9_]*)\b)");

// ------------------------------------------------------------------
//  Transform a single cut if flag set
// ------------------------------------------------------------------
inline std::string transformCut(const std::string& in, bool useTrue)
{
    return useTrue ? std::regex_replace(in, var_re, "true$1") : in;
}

// ------------------------------------------------------------------
//  Join YAML sequence ["a>1","b<2"] -> "a>1&&b<2" (with optional transform)
// ------------------------------------------------------------------
inline std::string joinCuts(const YAML::Node& seq, bool useTrue)
{
    std::ostringstream oss;
    for (std::size_t i = 0; i < seq.size(); ++i) {
        if (i) oss << "&&";
        oss << transformCut(seq[i].as<std::string>(), useTrue);
    }
    return oss.str();
}

// ------------------------------------------------------------------
//  loadEntryList : attach filtered TEntryList to <tree>
// ------------------------------------------------------------------
void loadEntryList(TTree*       tree,
                   const char*  yamlPath,
                   bool         useTrueVariable = false)  
{
    if (!tree) { std::cerr << "[loadEntryList] null TTree pointer\n"; return; }

    // 1) parse YAML
    YAML::Node cfg = YAML::LoadFile(yamlPath);

    // 2) infer pair key
    std::string searchStr = tree->GetCurrentFile()
                          ? tree->GetCurrentFile()->GetName()
                          : tree->GetName();

    std::string pairKey;
    for (const auto& kv : cfg) {
        std::string key = kv.first.as<std::string>();
        if (key == "bin_variable") continue;
        if (searchStr.find("/" + key + "/") != std::string::npos) { pairKey = key; break; }
    }
    if (pairKey.empty()) {
        std::cerr << "[loadEntryList] could not infer pair name from "
                  << searchStr << '\n';
        return;
    }

    // 3) build logical cut
    const YAML::Node& cutsNode = cfg[pairKey]["cuts"];
    if (!cutsNode || !cutsNode.IsSequence()) {
        std::cerr << "[loadEntryList] no cuts for " << pairKey << '\n';
        return;
    }
    std::string cutExpr = joinCuts(cutsNode, useTrueVariable);

    // 4) create entry list
    TString tmpName = Form("elist_tmp_%s", pairKey.c_str());
    tree->Draw(Form(">>%s", tmpName.Data()),
               cutExpr.c_str(),
               "entrylist");

    auto* tmp = static_cast<TEntryList*>(gDirectory->Get(tmpName));
    if (!tmp) {
        std::cerr << "[loadEntryList] Draw failed for cuts '" << cutExpr << "'\n";
        return;
    }

    auto* elist = static_cast<TEntryList*>(tmp->Clone(Form("elist_%s", pairKey.c_str())));
    tree->SetEntryList(elist);

    std::cout << "[loadEntryList] " << pairKey
              << (useTrueVariable ? " (true vars)" : "")
              << " → \"" << cutExpr << "\"  ("
              << elist->GetN() << " / " << tree->GetEntries()
              << " entries kept)\n";
}
} // namespace util
