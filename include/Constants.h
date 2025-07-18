#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace Constants {

static const std::map<std::string, std::string> runToMc = {{"Fall2018_RGA_inbending", "MC_RGA_inbending"},
                                                           {"Spring2019_RGA_inbending", "MC_RGA_inbending"},
                                                           {"Fall2018_RGA_outbending", "MC_RGA_outbending"},
                                                           {"Fall2018Spring2019_RGA_inbending", "MC_RGA_inbending"}};

static const std::vector<std::string> validPairs = {"piplus_piplus", "piplus_piminus", "piplus_pi0", "piminus_pi0", "piminus_piminus"};

/** One baryon’s display properties */
struct BaryonInfo {
    int pid;              ///< PDG ID
    std::string engName;  ///< human‑readable name
    std::string texName;  ///< LaTeX‑ready label
    std::string hexColor; ///< web hex colour
};

/** Fast lookup: pid → BaryonInfo */
static const std::unordered_map<int, BaryonInfo>& baryonPalette() {
    static const std::unordered_map<int, BaryonInfo> table = {
        //  pid ,  English name         ,  LaTeX name          , colour
        {2112, {2112, "Neutron", "n", "#ff7f0e"}},
        {2214, {2214, "Delta+", "\\Delta^{+}", "#2ca02c"}},
        {3122, {3122, "Lambda^{0}", "\\Lambda^{0}", "#d62728"}},
        {2114, {2114, "Delta^{0}", "\\Delta^{0}", "#9467bd"}},
        {3112, {3112, "Sigma^{-}", "\\Sigma^{-}", "#8c564b"}},
        {3222, {3222, "Sigma^{+}", "\\Sigma^{+}", "#e377c2"}},
        {2224, {2224, "Delta^{++}", "\\Delta^{++}", "#7f7f7f"}},
        {3212, {3212, "Sigma^{0}", "\\Sigma^{0}", "#bcbd22"}},
        {1114, {1114, "Delta^{-}", "\\Delta^{-}", "#17becf"}},
        {3224, {3224, "Sigma^{\\!*+}", "\\Sigma^{\\!*+}", "#aec7e8"}},
        {-2112, {-2112, "Anti‑neutron", "\\bar n", "#ffbb78"}},
        {3214, {3214, "Sigma^{\\!*0}", "\\Sigma^{\\!*0}", "#98df8a"}},
        {3114, {3114, "Sigma^{\\!*‑}", "\\Sigma^{\\!*‑}", "#ff9896"}},
        {-2212, {-2212, "Antiproton", "\\bar p", "#c5b0d5"}},
        {3322, {3322, "Xi^{0}", "\\Xi^{0}", "#c49c94"}},
        {-3112, {-3112, "Anti‑Sigma^{-}", "\\bar\\Sigma^{-}", "#f7b6d2"}},
        {3324, {3324, "Xi^{\\!*0}", "\\Xi^{\\!*0}", "#c7c7c7"}},
        {3312, {3312, "Xi^{-}", "\\Xi^{-}", "#dbdb8d"}},
        {-3212, {-3212, "Anti‑Sigma^{0}", "\\bar\\Sigma^{0}", "#9edae5"}},
        {-3122, {-3122, "Anti‑Lambda^{0}", "\\bar\\Lambda^{0}", "#ffbf00"}},
        {-3322, {-3322, "Anti‑Xi^{0}", "\\bar\\Xi^{0}", "#ff8000"}},
        {-2214, {-2214, "Anti‑Delta^{+}", "\\bar\\Delta^{+}", "#ff4000"}},
        {-1114, {-1114, "Anti‑Delta^{-}", "\\bar\\Delta^{-}", "#800000"}},
        {3314, {3314, "Xi^{\\!*‑}", "\\Xi^{\\!*‑}", "#808000"}},
        {-2114, {-2114, "Anti‑Delta^{0}", "\\bar\\Delta^{0}", "#008080"}},
        {-3222, {-3222, "Anti‑Sigma^{+}", "\\bar\\Sigma^{+}", "#800080"}},
        {-3114, {-3114, "Anti‑Sigma^{\\!*‑}", "\\bar\\Sigma^{\\!*‑}", "#4b0082"}},
        {-2224, {-2224, "Anti‑Delta^{++}", "\\bar\\Delta^{++}", "#ff00ff"}},
        {-3312, {-3312, "Anti‑Xi^{-}", "\\bar\\Xi^{-}", "#00ff00"}}};
    return table;
}

} // namespace Constants
