#include "BaryonContaminationProcessor.h"
#include "ModuleProcessorFactory.h"

#include <TPie.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TObject.h>
#include <TColor.h>          
#include <algorithm>       

// Self register
namespace {
std::unique_ptr<ModuleProcessor> make() {
    return std::make_unique<BaryonContaminationProcessor>();
}
const bool registered = []() {
    ModuleProcessorFactory::instance().registerProcessor("baryonContamination", make);
    return true;
}();
std::vector<TObject*> gKeepAlive;

} // namespace



std::string BaryonContaminationProcessor::name() const {
    return "baryonContamination";
}

Result BaryonContaminationProcessor::loadData(const std::filesystem::path& dir) const {
    Result r;
    r.moduleName = name();

    auto yamlPath = dir / "baryonContamination.yaml";
    if (!std::filesystem::exists(yamlPath)) {
        LOG_ERROR("YAML not found: " + yamlPath.string());
        return r;
    }

    YAML::Node root = YAML::LoadFile(yamlPath.string());

    // 1) total_entries
    int total = root["total_entries"].as<int>();
    r.scalars["total_entries"] = total;
    LOG_DEBUG("total_entries = " + std::to_string(total));

    // 2) all the pid maps
    static const std::vector<std::string> sections = {"trueparentpid_1", "trueparentpid_2", "trueparentparentpid_1",
                                                      "trueparentparentpid_2"};

    for (auto const& sec : sections) {
        if (!root[sec])
            continue;
        LOG_DEBUG(sec + ":");
        for (const auto& kv : root[sec]) {
            int pid = std::stoi(kv.first.as<std::string>());
            int count = kv.second.as<int>();
            std::string label = sec + "_" + std::to_string(pid);
            r.scalars[label] = count;
            LOG_DEBUG("  " + label + " = " + std::to_string(count));
        }
    }

    return r;
}

void BaryonContaminationProcessor::plotSummary(const std::string& moduleOutDir, const Config& cfg) const {
    namespace fs = std::filesystem;
    fs::path dir = effectiveOutDir(moduleOutDir, cfg);
    auto yamlPath = dir / "baryonContamination.yaml";
    if (!fs::exists(yamlPath)) {
        LOG_ERROR("plotSummary: YAML not found: " + yamlPath.string());
        return;
    }

    YAML::Node root = YAML::LoadFile(yamlPath.string());

    static const std::vector<std::string> sections = {
        "trueparentpid_1", "trueparentpid_2",
        "trueparentparentpid_1", "trueparentparentpid_2"
    };
    std::map<int,int> globalCounts;

    // color palette for up to 5 slices
    static const Color_t sliceColors[5] = {
        kRed, kBlue, kGreen+2, kMagenta, kCyan
    };

    // --- per‐section pies ---
    for (auto const& sec : sections) {
        if (!root[sec]) continue;

        // collect counts
        std::map<int,int> sectionCounts;
        for (auto const& kv : root[sec]) {
            int pid   = std::stoi(kv.first.as<std::string>());
            int cnt   = kv.second.as<int>();
            sectionCounts[pid] = cnt;
            globalCounts[pid] += cnt;
        }
        if (sectionCounts.empty()) continue;

        // move into vector and sort descending
        std::vector<std::pair<int,int>> vec(sectionCounts.begin(), sectionCounts.end());
        std::sort(vec.begin(), vec.end(),
            [](auto &a, auto &b){ return a.second > b.second; });
        if (vec.size() > 5) vec.resize(5);

        // build vals/labels
        std::vector<double> vals;
        std::vector<std::string> labels;
        for (auto &pr : vec) {
            labels.push_back(std::to_string(pr.first));
            vals.push_back(pr.second);
        }

        // draw
        TCanvas* c = new TCanvas(("c_"+sec).c_str(), sec.c_str(), 800,600);
        gKeepAlive.push_back(c);

        TPie* pie = new TPie(("pie_"+sec).c_str(), sec.c_str(), vals.size());
        gKeepAlive.push_back(pie);

        for (size_t i = 0; i < vals.size(); ++i) {
            pie->SetEntryVal(i, vals[i]);
            pie->SetEntryLabel(i, labels[i].c_str());
            pie->SetEntryFillColor(i, sliceColors[i]);  // distinct color
        }
        pie->SetRadius(0.35);
        pie->Draw("nol");

        std::string base = (dir / ("baryonContamination_"+sec)).string();
        c->SaveAs((base+".png").c_str());
        c->SaveAs((base+".pdf").c_str());
    }

    // --- global pie (top 5 overall) ---
    if (!globalCounts.empty()) {
        std::vector<std::pair<int,int>> gvec(globalCounts.begin(), globalCounts.end());
        std::sort(gvec.begin(), gvec.end(),
            [](auto &a, auto &b){ return a.second > b.second; });
        if (gvec.size() > 5) gvec.resize(5);

        std::vector<double> vals;
        std::vector<std::string> labels;
        for (auto &pr : gvec) {
            labels.push_back(std::to_string(pr.first));
            vals.push_back(pr.second);
        }

        TCanvas* c = new TCanvas("c_baryon_global","baryonContamination (top 5)",800,600);
        gKeepAlive.push_back(c);

        TPie* pie = new TPie("pie_baryon_global","All sources (top 5)", vals.size());
        gKeepAlive.push_back(pie);

        for (size_t i = 0; i < vals.size(); ++i) {
            pie->SetEntryVal(i, vals[i]);
            pie->SetEntryLabel(i, labels[i].c_str());
            pie->SetEntryFillColor(i, sliceColors[i]);
        }
        pie->SetRadius(0.35);
        pie->Draw("nol");

        std::string base = (dir / "baryonContamination_all").string();
        c->SaveAs((base+".png").c_str());
        c->SaveAs((base+".pdf").c_str());
    }
}