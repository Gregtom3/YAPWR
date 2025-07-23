#include "AsymmetryHandler.h"
#include "Logger.h"
#include "Synthesizer.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    Logger::setLevel(Logger::Level::Info);

    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <projectDir> <pionPair> <runPeriod>\n";
        return 1;
    }

    std::string projectDir = argv[1];
    std::string pionPair = argv[2];
    std::string runPeriod = argv[3];

    // run the synthesizer
    Synthesizer synth(projectDir, pionPair, runPeriod);
    synth.discoverConfigs();
    synth.runAll();

    // set up the asymmetry handler
    AsymmetryHandler asym(synth.getResults(), synth.getConfigsMap());

    // pick the right region name
    std::string regionName = (pionPair == "piplus_pi0" || pionPair == "piminus_pi0") ? "signal_purity_1_1" : "signal";

    // always use "full"
    std::string regionType = "full";

    // loop over pw = 0..12
    for (int pw = 0; pw <= 11; ++pw) {
        asym.reportAsymmetry(regionName, pw, regionType);
    }

    // dump everything to YAML
    std::string outPath = "out/" + projectDir + "/asymmetry_results.yaml";
    asym.dumpYaml(outPath);

    std::cout << "Done!" << std::endl;
    return 0;
}
