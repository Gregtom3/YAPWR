#include "AsymmetryHandler.h"
#include "Logger.h"
#include "Synthesizer.h"
#include <iostream>

int main(int argc, char** argv) {

    Logger::setLevel(Logger::Level::Info);

    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <projectDir> <pionPair> <runPeriod>\n";
        return 1;
    }
    Synthesizer synth(argv[1], argv[2], argv[3]);
    synth.discoverConfigs();
    synth.runAll();

    AsymmetryHandler asym(synth.getResults(), synth.getConfigsMap());
    asym.reportAsymmetry("signal_purity_5_5", 2, "full");
    asym.dumpYaml("out/" + std::string(argv[1]) + "/asymmetry_results.yaml");
    std::cout << "Done!" << std::endl;
    return 0;
}
