#include "ParticleMisidentificationProcessor.h"
#include "ModuleProcessorFactory.h"
#include <TCanvas.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TPie.h>
#include <algorithm>
#include <iomanip>
#include <sstream>

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

void ParticleMisidentificationProcessor::plotSummary(const std::string& moduleOutDir, const Config& cfg) const {
    namespace fs = std::filesystem;
    fs::path dir = effectiveOutDir(moduleOutDir, cfg);
    auto yamlPath = dir / "particleMisidentification.yaml";
    if (!fs::exists(yamlPath)) {
        LOG_ERROR("plotSummary: YAML not found: " + yamlPath.string());
        return;
    }

    YAML::Node root = YAML::LoadFile(yamlPath.string());
    const int totalEntries = root["total_entries"] ? root["total_entries"].as<int>() : 0;

    // choose which sections to draw
    std::vector<std::string> sections = {"truepid_e", "truepid_1"};
    if (!cfg.contains_pi0()) {
        sections.push_back("truepid_2");
    } else {
        sections.push_back("truepid_21");
        sections.push_back("truepid_22");
    }

    // Colors: gray for "Correct" then a palette for mis-IDs
    static const Color_t sliceColors[6] = {kGray, kRed, kBlue, kGreen + 2, kMagenta, kCyan};

    // Unified PID -> ParticleInfo
    auto const& pmap = Constants::particlePalette();

    for (auto const& sec : sections) {
        if (!root[sec])
            continue;

        // 1) Read all recon-PID counts for this true-PID section
        std::map<int, int> counts;
        for (auto const& kv : root[sec]) {
            int pid = 0;
            try {
                pid = std::stoi(kv.first.as<std::string>());
            } catch (...) {
                continue; // skip malformed keys
            }
            counts[pid] = kv.second.as<int>();
        }
        if (counts.empty()) {
            // Make a simple "no entries" canvas so downstream expects files to exist
            std::string cName = "c_misid_" + sec;
            TCanvas* c = new TCanvas(cName.c_str(), "", 800, 600);
            gKeepAlive.push_back(c);
            c->cd();

            TLatex* txt = new TLatex();
            gKeepAlive.push_back(txt);
            txt->SetNDC(true);
            txt->SetTextFont(42);
            txt->SetTextSize(0.05);
            txt->SetTextAlign(22);
            txt->DrawLatex(0.5, 0.55, "No entries for this section");

            std::string base = (dir / ("misid_" + sec)).string();
            c->SaveAs((base + ".png").c_str());
            c->SaveAs((base + ".pdf").c_str());
            continue;
        }

        // 2) Identify which PID is "correct"
        int correctPid = 0;
        if (sec == "truepid_e") {
            correctPid = 11;
        } else if (sec == "truepid_1") {
            auto p = cfg.getPionPair();
            std::string tex = Constants::firstHadronLatex(p);
            for (auto const& pr : pmap) {
                if (pr.second.texName == tex) {
                    correctPid = pr.first;
                    break;
                }
            }
        } else if (sec == "truepid_2") {
            auto p = cfg.getPionPair();
            std::string tex = Constants::secondHadronLatex(p);
            for (auto const& pr : pmap) {
                if (pr.second.texName == tex) {
                    correctPid = pr.first;
                    break;
                }
            }
        }
        if (correctPid == 0) {
            // fallback: PID with the max count
            correctPid = std::max_element(counts.begin(), counts.end(), [](auto& a, auto& b) {
                             return a.second < b.second;
                         })->first;
        }
        int correctCount = 0;
        if (auto itc = counts.find(correctPid); itc != counts.end()) {
            correctCount = std::max(0, itc->second);
        }

        // 3) Collect top mis-IDs (exclude correctPid), top-5
        std::vector<std::pair<int, int>> mis;
        mis.reserve(counts.size());
        for (auto& pr : counts) {
            if (pr.first == correctPid)
                continue;
            mis.push_back({pr.first, std::max(0, pr.second)});
        }
        std::sort(mis.begin(), mis.end(), [](auto& a, auto& b) {
            return a.second > b.second;
        });
        if (mis.size() > 5)
            mis.resize(5);

        // 4) Build pie values and labels (sanitize)
        std::vector<double> vals;
        std::vector<std::string> labels;

        // first slice: correct
        vals.push_back(static_cast<double>(correctCount));
        labels.push_back("Correct");

        for (auto& pr : mis) {
            const int pid = pr.first;
            const int cnt = std::max(0, pr.second);
            vals.push_back(static_cast<double>(cnt));

            double pct = (totalEntries > 0) ? (100.0 * cnt / static_cast<double>(totalEntries)) : 0.0;
            if (!std::isfinite(pct) || pct < 0)
                pct = 0.0;

            auto it = pmap.find(pid);
            std::string name = (it != pmap.end()) ? it->second.texName : std::to_string(pid);

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << pct;
            labels.push_back(name + " (" + oss.str() + "%)");
        }

        // sum check; if all zero, skip TPie and render a message
        double sum = 0.0;
        for (double v : vals) {
            if (!std::isfinite(v) || v < 0)
                v = 0.0;
            sum += v;
        }

        // 5) Prepare canvas
        std::string cName = "c_misid_" + sec;
        TCanvas* c = new TCanvas(cName.c_str(), "", 800, 600);
        gKeepAlive.push_back(c);
        c->cd();

        // 6) Build header text
        const std::string suffix = (sec.size() > 8) ? sec.substr(8) : "";
        std::string trueLabel;
        if (suffix == "e") {
            auto itE = pmap.find(11);
            trueLabel = (itE != pmap.end()) ? itE->second.texName : "e";
        } else if (suffix == "1") {
            trueLabel = Constants::firstHadronLatex(cfg.getPionPair());
        } else if (suffix == "2") {
            trueLabel = Constants::secondHadronLatex(cfg.getPionPair());
        } else {
            try {
                int tok = std::stoi(suffix);
                auto it2 = pmap.find(tok);
                trueLabel = (it2 != pmap.end() ? it2->second.texName : suffix);
            } catch (...) {
                trueLabel = suffix;
            }
        }
        const std::string header = "Particle MisID: True PID of " + trueLabel;

        // If no data to show, draw only the header and a message
        if (sum <= 0.0) {
            TLatex* hdr = new TLatex();
            gKeepAlive.push_back(hdr);
            hdr->SetNDC(true);
            hdr->SetTextFont(42);
            hdr->SetTextSize(0.05);
            hdr->SetTextAlign(13);
            hdr->DrawLatex(0.01, 0.98, header.c_str());

            TLatex* msg = new TLatex();
            gKeepAlive.push_back(msg);
            msg->SetNDC(true);
            msg->SetTextFont(42);
            msg->SetTextSize(0.045);
            msg->SetTextAlign(22);
            msg->DrawLatex(0.5, 0.55, "No entries available for this section");

            std::string base = (dir / ("misid_" + sec)).string();
            c->SaveAs((base + ".png").c_str());
            c->SaveAs((base + ".pdf").c_str());
            continue;
        }

        // 7) Draw pie (sanitized, no MakeLegend)
        TPie* pie = new TPie(("pie_" + sec).c_str(), "", vals.size());
        gKeepAlive.push_back(pie);
        pie->SetLabelFormat(""); // hide on-slice labels (legend will carry labels)
        for (size_t i = 0; i < vals.size(); ++i) {
            const double v = (std::isfinite(vals[i]) && vals[i] >= 0) ? vals[i] : 0.0;
            pie->SetEntryVal(i, v);
            pie->SetEntryLabel(i, TString(labels[i])); // force ROOT to own a copy
            pie->SetEntryFillColor(i, sliceColors[std::min<size_t>(i, 5)]);
        }
        pie->SetRadius(0.35);
        pie->Draw("nol");

        // Header
        {
            TLatex* latex = new TLatex();
            gKeepAlive.push_back(latex);
            latex->SetNDC(true);
            latex->SetTextFont(42);
            latex->SetTextSize(0.05);
            latex->SetTextAlign(13);
            latex->DrawLatex(0.01, 0.98, header.c_str());
        }

        // 8) Manual legend (avoid TPie::MakeLegend paths)
        TLegend* leg = new TLegend(0.70, 0.66, 0.95, 0.95);
        gKeepAlive.push_back(leg);
        leg->SetBorderSize(0);
        leg->SetFillStyle(0);
        for (size_t i = 0; i < labels.size(); ++i) {
            // small color box as a swatch
            TBox* sw = new TBox(0, 0, 0, 0);
            gKeepAlive.push_back(sw);
            const auto col = sliceColors[std::min<size_t>(i, 5)];
            sw->SetFillColor(col);
            sw->SetLineColor(col);
            leg->AddEntry(sw, labels[i].c_str(), "f");
        }
        leg->Draw();

        // 9) Save
        std::string base = (dir / ("misid_" + sec)).string();
        c->SaveAs((base + ".png").c_str());
        c->SaveAs((base + ".pdf").c_str());
    }
}
