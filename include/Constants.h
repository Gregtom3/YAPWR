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

struct ParticleInfo {
    int     pid;        ///< PDG ID
    std::string engName;///< English name
    std::string texName;///< LaTeX label
    std::string hexColor;///< Color‑blind‑safe hex
    bool    isBaryon;   ///< true if baryon or antibaryon
    bool    isMeson;    ///< true if meson
};

inline const std::unordered_map<int, ParticleInfo>& particlePalette() {
    static const std::unordered_map<int, ParticleInfo> tbl = {
        {2112, {2112, "Neutron", "n", "#ff7f0e",true,false}},
        {2214, {2214, "Delta+", "\\Delta^{+}", "#2ca02c",true,false}},
        {3122, {3122, "Lambda^{0}", "\\Lambda^{0}", "#d62728",true,false}},
        {2114, {2114, "Delta^{0}", "\\Delta^{0}", "#9467bd",true,false}},
        {3112, {3112, "Sigma^{-}", "\\Sigma^{-}", "#8c564b",true,false}},
        {3222, {3222, "Sigma^{+}", "\\Sigma^{+}", "#e377c2",true,false}},
        {2224, {2224, "Delta^{++}", "\\Delta^{++}", "#7f7f7f",true,false}},
        {3212, {3212, "Sigma^{0}", "\\Sigma^{0}", "#bcbd22",true,false}},
        {1114, {1114, "Delta^{-}", "\\Delta^{-}", "#17becf",true,false}},
        {3224, {3224, "Sigma^{\\!*+}", "\\Sigma^{\\!*+}", "#aec7e8",true,false}},
        {-2112, {-2112, "Anti‑neutron", "\\bar n", "#ffbb78",true,false}},
        {3214, {3214, "Sigma^{\\!*0}", "\\Sigma^{\\!*0}", "#98df8a",true,false}},
        {3114, {3114, "Sigma^{\\!*‑}", "\\Sigma^{\\!*‑}", "#ff9896",true,false}},
        {-2212, {-2212, "Antiproton", "\\bar p", "#c5b0d5",true,false}},
        {3322, {3322, "Xi^{0}", "\\Xi^{0}", "#c49c94",true,false}},
        {-3112, {-3112, "Anti‑Sigma^{-}", "\\bar\\Sigma^{-}", "#f7b6d2",true,false}},
        {3324, {3324, "Xi^{\\!*0}", "\\Xi^{\\!*0}", "#c7c7c7",true,false}},
        {3312, {3312, "Xi^{-}", "\\Xi^{-}", "#dbdb8d",true,false}},
        {-3212, {-3212, "Anti‑Sigma^{0}", "\\bar\\Sigma^{0}", "#9edae5",true,false}},
        {-3122, {-3122, "Anti‑Lambda^{0}", "\\bar\\Lambda^{0}", "#ffbf00",true,false}},
        {-3322, {-3322, "Anti‑Xi^{0}", "\\bar\\Xi^{0}", "#ff8000",true,false}},
        {-2214, {-2214, "Anti‑Delta^{+}", "\\bar\\Delta^{+}", "#ff4000",true,false}},
        {-1114, {-1114, "Anti‑Delta^{-}", "\\bar\\Delta^{-}", "#800000",true,false}},
        {3314, {3314, "Xi^{\\!*‑}", "\\Xi^{\\!*‑}", "#808000",true,false}},
        {-2114, {-2114, "Anti‑Delta^{0}", "\\bar\\Delta^{0}", "#008080",true,false}},
        {-3222, {-3222, "Anti‑Sigma^{+}", "\\bar\\Sigma^{+}", "#800080",true,false}},
        {-3114, {-3114, "Anti‑Sigma^{\\!*‑}", "\\bar\\Sigma^{\\!*‑}", "#4b0082",true,false}},
        {-2224, {-2224, "Anti‑Delta^{++}", "\\bar\\Delta^{++}", "#ff00ff",true,false}},
        {-3312, {-3312, "Anti‑Xi^{-}", "\\bar\\Xi^{-}", "#00ff00",true,false}},
        { 111  , { 111,  "π⁰"          , "$\\pi^{0}$"    , "#E69F00",false,true }}, 
        { 113  , { 113,  "ρ⁰(770)"     , "$\\rho^{0}$"   , "#56B4E9",false,true }},
        { 221  , { 221,  "η"            , "$\\eta$"       , "#009E73",false,true }},
        { 223  , { 223,  "ω(782)"      , "$\\omega$"     , "#D55E00",false,true }},
        { 310  , { 310,  "K⁰ₛ"         , "$K^{0}_{S}$"   , "#CC79A7",false,true }},
        { 333  , { 333,  "ϕ(1020)"     , "$\\phi$"       , "#0072B2",false,true }},
        { 213  , { 213,  "ρ⁺(770)"     , "$\\rho^{+}$"   , "#F0E442",false,true }},
        {-213 , {-213,  "ρ⁻(770)"     , "$\\rho^{-}$"   , "#0072B2",false,true }},
        { 323  , { 323,  "K*⁺(892)"    , "$K^{*+}$"      , "#E69F00",false,true }},
        {-323 , {-323,  "K*⁻(892)"    , "$K^{*-}$"      , "#56B4E9",false,true }},
        {211, {211, "Pi^{+}", "\\pi^{+}", "#D55E00",false,true}},
        {-211, {-211, "Pi^{-}", "\\pi^{-}", "#E69F00",false,true}},
        {321, {321, "K^{+}", "K^{+}", "#8E44AD",false,true}},
        {-321, {-321, "K^{-}", "K^{-}", "#A569BD",false,true}},
        {130, {130, "K^{0}_{\\,L}", "K^{0}_{L}", "#FF5733",false,true}},
        {11, {11, "Electron", "e^{-}", "#0072B2"}},
        {-11, {-11, "Positron", "e^{+}", "#56B4E9"}},
        {22, {22, "Photon", "\\gamma", "#009E73"}}
    };
    return tbl;
}


/** pion pair → LaTeX‑ready label */
inline const std::unordered_map<std::string, std::string>& pionPairLatex() {
    static const std::unordered_map<std::string, std::string> table = {
        {"piplus_pi0",    "$\\pi^{+}\\,\\pi^{0}$"},
        {"piminus_pi0",   "$\\pi^{-}\\,\\pi^{0}$"},
        {"piplus_piplus", "$\\pi^{+}\\,\\pi^{+}$"},
        {"piplus_piminus","$\\pi^{+}\\,\\pi^{-}$"},
        {"piminus_piminus","$\\pi^{-}\\,\\pi^{-}$"}
    };
    return table;
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
