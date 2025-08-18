#include "BaryonContaminationProcessor.h"
#include "ModuleProcessorFactory.h"

#include <TCanvas.h>
#include <TColor.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TObject.h>
#include <TPie.h>
#include <TStyle.h>
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

    // pick exactly two sections
    std::vector<std::string> sections;
    if (cfg.contains_pi0()) {
        sections = {"trueparentpid_1", "trueparentparentpid_2"};
    } else {
        sections = {"trueparentpid_1", "trueparentpid_2"};
    }

    // color palette: [0]=gray for non‑baryon, then colors for baryons
    static const Color_t sliceColors[6] = {kGray, kRed, kBlue, kGreen + 2, kMagenta, kCyan};

    // pie title
    auto pairName = cfg.getPionPair();
    auto title = Constants::pionPairLatex().at(pairName);

    // unified lookup
    auto const& pmap = Constants::particlePalette();
    int totalEntries = root["total_entries"].as<int>();
    for (auto const& sec : sections) {
        if (!root[sec])
            continue;

        // 1) read all counts
        std::map<int, int> counts;
        for (auto const& kv : root[sec]) {
            int pid = std::stoi(kv.first.as<std::string>());
            counts[pid] = kv.second.as<int>();
        }

        // 2) separate non‑baryons vs baryons
        int nonBaryonSum = 0;
        std::map<int, int> baryonCounts;
        for (auto const& pr : counts) {
            int pid = pr.first, cnt = pr.second;
            if (pid == 2212) {
                nonBaryonSum += cnt;
                continue;
            }
            auto it = pmap.find(pid);
            bool isBaryon = (it != pmap.end()) && it->second.isBaryon;
            if (isBaryon) {
                baryonCounts[pid] = cnt;
            } else {
                nonBaryonSum += cnt;
            }
        }

        // 3) pick top‑5 baryons by count
        std::vector<std::pair<int, int>> vec(baryonCounts.begin(), baryonCounts.end());
        std::sort(vec.begin(), vec.end(), [](auto& a, auto& b) {
            return a.second > b.second;
        });
        if (vec.size() > 5)
            vec.resize(5);

        // 4) build values & labels, starting with non-baryon slice
        std::vector<double> vals;
        std::vector<std::string> labels;
        
        vals.push_back(std::max(0, nonBaryonSum));
        labels.push_back("Non-baryon");
        
        // then baryon slices with percentages
        for (auto& pr : vec) {
            int pid = pr.first;
            int cnt = std::max(0, pr.second);            // sanitize negatives
            vals.push_back(static_cast<double>(cnt));
            double pct = (totalEntries > 0) ? 100.0 * cnt / totalEntries : 0.0;
            if (!std::isfinite(pct)) pct = 0.0;
        
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << pct;
            auto it = pmap.find(pid);
            if (it != pmap.end()) labels.push_back(it->second.texName + " (" + oss.str() + "%)");
            else                  labels.push_back(std::to_string(pid)      + " (" + oss.str() + "%)");
        }
        
        // ---- NEW: guard against zero or invalid totals ----
        double sum = 0.0;
        for (double v : vals) {
            if (!std::isfinite(v) || v < 0) v = 0; // sanitize
            sum += v;
        }
        
        std::string cName = "c_baryon_" + sec;
        TCanvas* c = new TCanvas(cName.c_str(), "", 800, 600);
        gKeepAlive.push_back(c);
        c->cd();
        
        // If no entries, draw a friendly message instead of a pie
        if (sum <= 0.0) {
            std::cout << "[WARN] " << sec << ": all counts are zero; skipping TPie.\n";
            TLatex *msg = new TLatex();
            gKeepAlive.push_back(msg);
            msg->SetNDC(true);
            msg->SetTextFont(42);
            msg->SetTextSize(0.045);
            msg->SetTextAlign(22);
            msg->DrawLatex(0.5, 0.6, "No entries available for this section");
            // header
            std::string singleLatex = (sec.back() == '1') ? Constants::firstHadronLatex(pairName)
                                                          : Constants::secondHadronLatex(pairName);
            std::string header = title + " dihadrons: Parent PID of " + singleLatex;
            TLatex *hdr = new TLatex();
            gKeepAlive.push_back(hdr);
            hdr->SetNDC(true);
            hdr->SetTextFont(42);
            hdr->SetTextSize(0.05);
            hdr->SetTextAlign(13);
            hdr->DrawLatex(0.01, 0.98, header.c_str());
        
            std::string base = (dir / ("baryon_" + sec)).string();
            c->SaveAs((base + ".png").c_str());
            c->SaveAs((base + ".pdf").c_str());
            continue; // go to next section
        }
        
        // ---- draw a robust pie ----
        TPie* pie = new TPie(("pie_" + sec).c_str(), "", vals.size());
        gKeepAlive.push_back(pie);
        
        for (size_t i = 0; i < vals.size(); ++i) {
            double v = std::isfinite(vals[i]) && vals[i] >= 0 ? vals[i] : 0.0;
            pie->SetEntryVal(i, v);
            // Use TString to force an internal copy (avoid pointer aliasing issues)
            pie->SetEntryLabel(i, TString(labels[i]));
            // Clamp color index just in case vals grows
            pie->SetEntryFillColor(i, sliceColors[ std::min<size_t>(i, 5) ]);
        }
        pie->SetRadius(0.35);
        pie->SetLabelFormat(""); // hide on-slice labels (legend will show them)
        pie->Draw("nol");
        
        // ---- build our own legend (avoid TPie::MakeLegend paths) ----
        auto *leg = new TLegend(0.70, 0.66, 0.95, 0.95);
        gKeepAlive.push_back(leg);
        leg->SetBorderSize(0);
        leg->SetFillStyle(0);
        for (size_t i = 0; i < labels.size(); ++i) {
            auto *ph = new TBox(0,0,0,0);                 // dummy object just for color swatch
            gKeepAlive.push_back(ph);
            ph->SetFillColor(sliceColors[ std::min<size_t>(i, 5) ]);
            ph->SetLineColor(sliceColors[ std::min<size_t>(i, 5) ]);
            leg->AddEntry(ph, labels[i].c_str(), "f");
        }
        leg->Draw();
        
        // header (unchanged)
        std::string singleLatex = (sec.back() == '1') ? Constants::firstHadronLatex(pairName)
                                                      : Constants::secondHadronLatex(pairName);
        std::string header = title + " dihadrons: Parent PID of " + singleLatex;
        TLatex *latex = new TLatex();
        gKeepAlive.push_back(latex);
        latex->SetNDC(true);
        latex->SetTextFont(42);
        latex->SetTextSize(0.05);
        latex->SetTextAlign(13);
        latex->DrawLatex(0.01, 0.98, header.c_str());
        
        // save
        std::string base = (dir / ("baryon_" + sec)).string();
        c->SaveAs((base + ".png").c_str());
        c->SaveAs((base + ".pdf").c_str());
    }
}