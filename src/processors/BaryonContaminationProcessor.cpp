#include "BaryonContaminationProcessor.h"
#include "ModuleProcessorFactory.h"

#include <TPie.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TObject.h>

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
    std::filesystem::path dir = effectiveOutDir(moduleOutDir, cfg);
    // We assume moduleOutDir already points at ".../runPeriod/module-out___baryonContamination"
    auto yamlPath = dir / std::string("baryonContamination.yaml");
    if (!std::filesystem::exists(yamlPath)) {
        LOG_ERROR("plotSummary: YAML not found: " + yamlPath.string());
        return;
    }

    YAML::Node root = YAML::LoadFile(yamlPath.string());

    static const std::vector<std::string> sections = {
        "trueparentpid_1", "trueparentpid_2",
        "trueparentparentpid_1", "trueparentparentpid_2"
    };

    // If you prefer one global pie, accumulate across sections:
    std::map<int, int> globalCounts;

    // Also keep per-section pies
    for (const auto& sec : sections) {
        if (!root[sec]) continue;

        std::vector<double> vals;
        std::vector<std::string> labels;

        for (const auto& kv : root[sec]) {
            int pid   = std::stoi(kv.first.as<std::string>());
            int count = kv.second.as<int>();
            vals.push_back(count);
            labels.push_back(std::to_string(pid));
            globalCounts[pid] += count;
        }

        if (vals.empty()) continue;

        std::string cname = "c_" + sec;
        TCanvas* c = new TCanvas(cname.c_str(), sec.c_str(), 800, 600);
        gKeepAlive.push_back(c);

        TPie* pie = new TPie(("pie_" + sec).c_str(), sec.c_str(), static_cast<Int_t>(vals.size()));
        gKeepAlive.push_back(pie);

        for (size_t i = 0; i < vals.size(); ++i) {
            pie->SetEntryVal(i, vals[i]);
            pie->SetEntryLabel(i, labels[i].c_str());
        }

        pie->SetRadius(0.35);
        pie->Draw("nol"); // "nol" = no legend if you do not want TPaveText clutter

        // Save
        std::string base = (dir / ("baryonContamination_" + sec)).string();
        c->SaveAs((base + ".png").c_str());
        c->SaveAs((base + ".pdf").c_str());
    }

    // One global combined pie
    {
        std::vector<double> vals;
        std::vector<std::string> labels;
        vals.reserve(globalCounts.size());
        labels.reserve(globalCounts.size());
        for (auto& kv : globalCounts) {
            vals.push_back(kv.second);
            labels.push_back(std::to_string(kv.first));
        }
        if (!vals.empty()) {
            TCanvas* c = new TCanvas("c_baryon_global", "baryonContamination (all)", 800, 600);
            gKeepAlive.push_back(c);

            TPie* pie = new TPie("pie_baryon_global", "All sources", static_cast<Int_t>(vals.size()));
            gKeepAlive.push_back(pie);

            for (size_t i = 0; i < vals.size(); ++i) {
                pie->SetEntryVal(i, vals[i]);
                pie->SetEntryLabel(i, labels[i].c_str());
            }
            pie->SetRadius(0.35);
            pie->Draw("nol");

            std::string base = (dir / "baryonContamination_all").string();
            c->SaveAs((base + ".png").c_str());
            c->SaveAs((base + ".pdf").c_str());
        }
    }
}
