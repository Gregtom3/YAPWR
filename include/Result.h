#pragma once
#include <map>
#include <string>

/// Generic holder for module outputs (errors, histograms, tables…)
struct Result {
    std::string moduleName;
    std::map<std::string, double> scalars; // e.g. {"asymmetry": 0.0123}
                                           // you can add TH1*/TGraph* from ROOT, LaTeX snippets, etc.
};