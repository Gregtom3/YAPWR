#include "ParticleMisidentificationProcessor.h"
#include "ModuleProcessorFactory.h"
#include <TPie.h>
#include <TCanvas.h>
#include <TLatex.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <TLegend.h>

namespace {
std::unique_ptr<ModuleProcessor> make() {
    return std::make_unique<ParticleMisidentificationProcessor>();
}
const bool registered = []() {
    ModuleProcessorFactory::instance().registerProcessor("particleMisidentification", make);
    return true;
}();
std::vector<TObject*> gKeepAlive;
} // namespace

std::string ParticleMisidentificationProcessor::name() const {
    return "particleMisidentification";
}

Result ParticleMisidentificationProcessor::loadData(const std::filesystem::path& dir) const {
    Result r;
    r.moduleName = name();

    auto yamlPath = dir / "particleMisidentification.yaml";
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
    static const std::vector<std::string> keys = {"truepid_e",  "truepid_1",  "truepid_2", "truepid_11",
                                                  "truepid_12", "truepid_21", "truepid_22"};

    for (auto const& key : keys) {
        if (!root[key])
            continue; // skip missing sections
        LOG_DEBUG(key + ":");
        for (const auto& kv : root[key]) {
            int pid = std::stoi(kv.first.as<std::string>());
            int count = kv.second.as<int>();
            std::string label = key + "_" + std::to_string(pid);
            r.scalars[label] = count;
            LOG_DEBUG("  " + label + " = " + std::to_string(count));
        }
    }

    return r;
}


void ParticleMisidentificationProcessor::plotSummary(const std::string& moduleOutDir,
                                                     const Config& cfg) const
{
    namespace fs = std::filesystem;
    fs::path dir = effectiveOutDir(moduleOutDir, cfg);
    auto yamlPath = dir / "particleMisidentification.yaml";
    if (!fs::exists(yamlPath)) {
        LOG_ERROR("plotSummary: YAML not found: " + yamlPath.string());
        return;
    }

    YAML::Node root = YAML::LoadFile(yamlPath.string());
    int totalEntries = root["total_entries"].as<int>();

    // 1) choose which sections to draw
    std::vector<std::string> sections = {"truepid_e", "truepid_1"};
    if (!cfg.contains_pi0()) {
        sections.push_back("truepid_2");
    } else {
        sections.push_back("truepid_21");
        sections.push_back("truepid_22");
    }

    // Colors: gray for “Correct” then a palette for mis‑IDs
    static const Color_t sliceColors[6] = {
        kGray, kRed, kBlue, kGreen+2, kMagenta, kCyan
    };

    // Unified PID → ParticleInfo
    auto const& pmap = Constants::particlePalette();

    for (auto const& sec : sections) {
        if (!root[sec]) continue;

        // 1) Read all recon‐PID counts for this true‐PID section
        std::map<int,int> counts;
        for (auto const& kv : root[sec]) {
            int pid = std::stoi(kv.first.as<std::string>());
            counts[pid] = kv.second.as<int>();
        }

        // 2) Identify which PID is “correct”:
        //    - for truepid_e: correct PID = 11 (electron)
        //    - for truepid_1 / truepid_2: correct PID = first/second pion of the pair
        //    - otherwise: take the PID with the largest count
        int correctPid = 0;
        if (sec == "truepid_e") {
            correctPid = 11;
        } else if (sec == "truepid_1") {
            // first hadron of the dihadron pair
            auto p = cfg.getPionPair();
            std::string tex = Constants::firstHadronLatex(p);
            // reverse‐lookup: find the PID in pmap by matching texName
            for (auto const& pr : pmap) {
                if (pr.second.texName == tex) { correctPid = pr.first; break; }
            }
        } else if (sec == "truepid_2") {
            auto p = cfg.getPionPair();
            std::string tex = Constants::secondHadronLatex(p);
            for (auto const& pr : pmap) {
                if (pr.second.texName == tex) { correctPid = pr.first; break; }
            }
        }
        // fallback: the PID with the max count
        if (correctPid == 0) {
            correctPid = std::max_element(
                counts.begin(), counts.end(),
                [](auto &a, auto &b){ return a.second < b.second; }
            )->first;
        }
        int correctCount = counts[correctPid];

        // 3) Build a vector of mis‑ID counts (everything except correctPid)
        std::vector<std::pair<int,int>> mis;
        for (auto &pr : counts) {
            if (pr.first == correctPid) continue;
            mis.push_back(pr);
        }
        std::sort(mis.begin(), mis.end(),
                  [](auto &a, auto &b){ return a.second > b.second; });
        if (mis.size() > 5) mis.resize(5);

        // 4) Build the pie’s values & labels
        std::vector<double> vals;
        std::vector<std::string> labels;

        // first slice = correct
        vals.push_back(correctCount);
        labels.push_back("Correct");

        // next slices = top mis‑IDs with percentages
        for (auto &pr : mis) {
            int pid = pr.first, cnt = pr.second;
            vals.push_back(cnt);

            double pct = 100.0 * cnt / totalEntries;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << pct;

            auto it = pmap.find(pid);
            std::string name = (it!=pmap.end()) ? it->second.texName
                                                : std::to_string(pid);
            labels.push_back(name + " (" + oss.str() + "%)");
        }

        // 5) Prepare the canvas
        std::string cName = "c_misid_" + sec;
        TCanvas* c = new TCanvas(cName.c_str(),"", 800, 600);
        gKeepAlive.push_back(c);
        c->cd();

        // 6) Draw the header: “Particle MisID: True PID of <X>”
        // Header text
        std::string suffix = sec.substr(8);  // e.g. "e", "1", "21", etc.
        std::string trueLabel;
        if (suffix == "e") {
            trueLabel = pmap.at(11).texName;
        } else if (suffix == "1") {
            trueLabel = Constants::firstHadronLatex(cfg.getPionPair());
        } else if (suffix == "2") {
            trueLabel = Constants::secondHadronLatex(cfg.getPionPair());
        } else {
            // numeric suffix
            try {
                int tok = std::stoi(suffix);
                auto it2 = pmap.find(tok);
                trueLabel = (it2!=pmap.end() ? it2->second.texName : suffix);
            } catch(...) {
                trueLabel = suffix;
            }
        }
        std::string header = "Particle MisID: True PID of " + trueLabel;
        TLatex latex;
        latex.SetTextFont(42);
        latex.SetTextSize(0.05);
        latex.SetTextAlign(13);

        // 7) Draw the pie
        TPie* pie = new TPie(("pie_"+sec).c_str(), "", vals.size());
        gKeepAlive.push_back(pie);
        pie->SetLabelFormat("");
        for (size_t i = 0; i < vals.size(); ++i) {
            pie->SetEntryVal(i,       vals[i]);
            pie->SetEntryLabel(i,     labels[i].c_str());
            pie->SetEntryFillColor(i, sliceColors[i]);
        }
        pie->SetRadius(0.35);
        pie->Draw();
        latex.DrawLatexNDC(0.01, 0.98, header.c_str());
        // 8) Legend
        auto leg = pie->MakeLegend();
        gKeepAlive.push_back(leg);
        leg->SetX1(0.70); leg->SetX2(0.95);
        leg->SetY1(0.66); leg->SetY2(0.95);

        // 9) Save
        std::string base = (dir / ("misid_"+sec)).string();
        c->SaveAs((base + ".png").c_str());
        c->SaveAs((base + ".pdf").c_str());
    }
}
