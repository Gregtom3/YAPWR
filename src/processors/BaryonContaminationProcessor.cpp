#include "BaryonContaminationProcessor.h"
#include "ModuleProcessorFactory.h"

#include <TPie.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TObject.h>
#include <TColor.h>          
#include <algorithm>       
#include <TLegend.h>
#include <TLatex.h>

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


void BaryonContaminationProcessor::plotSummary(const std::string& moduleOutDir,
                                               const Config& cfg) const
{
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
        sections = { "trueparentpid_1", "trueparentparentpid_2" };
    } else {
        sections = { "trueparentpid_1", "trueparentpid_2" };
    }

    // color palette: [0]=gray for non‑baryon, then colors for baryons
    static const Color_t sliceColors[6] = {
        kGray, kRed, kBlue, kGreen+2, kMagenta, kCyan
    };

    // pie title
    auto pairName = cfg.getPionPair();
    auto title    = Constants::pionPairLatex().at(pairName);

    // unified lookup
    auto const& pmap = Constants::particlePalette();
    int totalEntries = root["total_entries"].as<int>();
    for (auto const& sec : sections) {
        if (!root[sec]) continue;

        // 1) read all counts
        std::map<int,int> counts;
        for (auto const& kv : root[sec]) {
            int pid = std::stoi(kv.first.as<std::string>());
            counts[pid] = kv.second.as<int>();
        }

        // 2) separate non‑baryons vs baryons
        int nonBaryonSum = 0;
        std::map<int,int> baryonCounts;
        for (auto const& pr : counts) {
            int pid = pr.first, cnt = pr.second;
            auto it = pmap.find(pid);
            bool isBaryon = (it != pmap.end()) && it->second.isBaryon;
            if (isBaryon) {
                baryonCounts[pid] = cnt;
            } else {
                nonBaryonSum += cnt;
            }
        }

        // 3) pick top‑5 baryons by count
        std::vector<std::pair<int,int>> vec(baryonCounts.begin(), baryonCounts.end());
        std::sort(vec.begin(), vec.end(),
                  [](auto &a, auto &b){ return a.second > b.second; });
        if (vec.size() > 5) vec.resize(5);

        // 4) build values & labels, starting with non‑baryon slice
        std::vector<double> vals;
        std::vector<std::string> labels;

        vals.push_back(nonBaryonSum);
        labels.push_back("Non-baryon");

        // then baryon slices with percentages
        for (auto &pr : vec) {
            int pid = pr.first;
            int cnt = pr.second;
            vals.push_back(cnt);

            // pct = cnt/totalEntries*100
            double pct = 100.0 * cnt / totalEntries;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << pct;

            auto it = pmap.find(pid);
            if (it != pmap.end()) {
                labels.push_back(it->second.texName + " (" + oss.str() + "%)");
            } else {
                labels.push_back(std::to_string(pid) + " (" + oss.str() + "%)");
            }
        }

        // 5) draw pie
        std::string cName = "c_baryon_" + sec;
        TCanvas* c = new TCanvas(cName.c_str(), "", 800,600);
        gKeepAlive.push_back(c);

        TPie* pie = new TPie(("pie_"+sec).c_str(), "", vals.size());
        gKeepAlive.push_back(pie);

        for (size_t i = 0; i < vals.size(); ++i) {
            pie->SetEntryVal(i,           vals[i]);
            pie->SetEntryLabel(i,         labels[i].c_str());
            pie->SetEntryFillColor(i,     sliceColors[i]);
        }
        pie->SetRadius(0.35);
        pie->SetLabelFormat("");
        pie->Draw("nol");
        TLegend *pieleg = pie->MakeLegend();
        pieleg->SetY1(.66);
        pieleg->SetY2(.95);
        pieleg->SetX1(.7);
        pieleg->SetX2(.95);

        // 6) Draw the header
        //    Determine which pion to show (first or second)
        std::string singleLatex = (sec.back()=='1')
            ? Constants::firstHadronLatex(pairName)
            : Constants::secondHadronLatex(pairName);
        std::string header = title + " dihadrons: Parent PID of " + singleLatex;
        TLatex latex;
        latex.SetTextFont(42);
        latex.SetTextSize(0.05);    // larger text
        latex.SetTextAlign(13);     // left + top
        latex.DrawLatexNDC(0.01, 0.98, header.c_str());
        
        // 7) save
        std::string base = (dir / ("baryon_"+sec)).string();
        c->SaveAs((base + ".png").c_str());
        c->SaveAs((base + ".pdf").c_str());
    }
}