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

static const std::string DEFAULT_PI0_SIGNAL_REGION = "signal_purity_1_1";

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

/** Display properties for a non‑baryonic particle */
struct ParticleInfo {
    int pid;              ///< PDG ID
    std::string engName;  ///< English name
    std::string texName;  ///< LaTeX label
    std::string hexColor; ///< Color‑blind‑safe hex
};

/** pid → ParticleInfo lookup */
inline const std::unordered_map<int, ParticleInfo>& particlePalette() {
    static const std::unordered_map<int, ParticleInfo> tbl = {// pid ,  English name     ,  LaTeX label  ,  colour
                                                              {11, {11, "Electron", "e^{-}", "#0072B2"}},
                                                              {-11, {-11, "Positron", "e^{+}", "#56B4E9"}},
                                                              {22, {22, "Photon", "\\gamma", "#009E73"}},
                                                              {211, {211, "Pi^{+}", "\\pi^{+}", "#D55E00"}},
                                                              {-211, {-211, "Pi^{-}", "\\pi^{-}", "#E69F00"}},
                                                              {2212, {2212, "Proton", "p", "#CC79A7"}},
                                                              {-2212, {-2212, "Antiproton", "\\bar p", "#F0E442"}},
                                                              {2112, {2112, "Neutron", "n", "#000000"}},
                                                              {321, {321, "K^{+}", "K^{+}", "#8E44AD"}},
                                                              {-321, {-321, "K^{-}", "K^{-}", "#A569BD"}},
                                                              {130, {130, "K^{0}_{\\,L}", "K^{0}_{L}", "#FF5733"}}};
    return tbl;
}

struct PWTerm {
    int n; ///< index 0…11
    int l;
    int m;
    int twist;
    std::string code;  ///< C++ expression
    std::string latex; ///< LaTeX expression
};

/* ---------- partial‑wave modulation lookup ------------------- */
inline const std::unordered_map<int, PWTerm>& pwTable() {
    static const std::unordered_map<int, PWTerm> tbl = {
        /* n l m tw code                              latex ------------------------------------ */
        {0, {0, 1, 1, 2, "(sin(th))*sin(phi_h - phi_R1)", "\\sin\\theta\\,\\sin(\\phi_h-\\phi_R)"}},
        {1, {1, 2, 1, 2, "(sin(2*th))*sin(phi_h - phi_R1)", "\\sin2\\theta\\,\\sin(\\phi_h-\\phi_R)"}},
        {2, {2, 2, 2, 2, "(sin(th)*sin(th))*sin(2*phi_h - 2*phi_R1)", "\\sin^{2}\\theta\\,\\sin(2\\phi_h-2\\phi_R)"}},
        {3, {3, 0, 0, 3, "1.0*sin(phi_h)", "\\sin(\\phi_h)"}},
        {4, {4, 1, -1, 3, "(sin(th))*sin(2*phi_h - phi_R1)", "\\sin\\theta\\,\\sin(2\\phi_h-\\phi_R)"}},
        {5, {5, 1, 0, 3, "(cos(th))*sin(phi_h)", "\\cos\\theta\\,\\sin(\\phi_h)"}},
        {6, {6, 1, 1, 3, "(sin(th))*sin(phi_R1)", "\\sin\\theta\\,\\sin(\\phi_R)"}},
        {7, {7, 2, -2, 3, "(sin(th)*sin(th))*sin(3*phi_h - 2*phi_R1)", "\\sin^{2}\\theta\\,\\sin(3\\phi_h-2\\phi_R)"}},
        {8, {8, 2, -1, 3, "(sin(2*th))*sin(2*phi_h - phi_R1)", "\\sin2\\theta\\,\\sin(2\\phi_h-\\phi_R)"}},
        {9, {9, 2, 0, 3, "(0.5*(3*cos(th)*cos(th)-1))*sin(phi_h)", "\\frac{1}{2}(3\\cos^{2}\\theta-1)\\,\\sin(\\phi_h)"}},
        {10, {10, 2, 1, 3, "(sin(2*th))*sin(phi_R1)", "\\sin2\\theta\\,\\sin(\\phi_R)"}},
        {11, {11, 2, 2, 3, "(sin(th)*sin(th))*sin(-phi_h + 2*phi_R1)", "\\sin^{2}\\theta\\,\\sin(-\\phi_h+2\\phi_R)"}}};
    return tbl;
}

/* lookup by index ------------------------------------------------- */
inline const PWTerm& pwTerm(int n) {
    return pwTable().at(n);
}
inline const std::string& modulationCode(int n) {
    return pwTable().at(n).code;
}
inline const std::string& modulationLatex(int n) {
    return pwTable().at(n).latex;
}

/* lookup by (l,m,twist) -------------------------------- */
inline const PWTerm* pwTerm(int l, int m, int tw) {
    for (auto& [_, t] : pwTable())
        if (t.l == l && t.m == m && t.twist == tw)
            return &t;
    return nullptr;
}

inline int pwL(int n) {
    return pwTable().at(n).l;
}
inline int pwM(int n) {
    return pwTable().at(n).m;
}
inline int pwTwist(int n) {
    return pwTable().at(n).twist;
}
} // namespace Constants
