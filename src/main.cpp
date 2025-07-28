#include "AsymmetryHandler.h"
#include "Logger.h"
#include "Synthesizer.h"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    Logger::setLevel(Logger::Level::Error);

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <projectDir>\n";
        return 1;
    }

    std::string projectDir = argv[1];

    // All pion pairs except pi0_pi0 (edit this list if you add/remove channels)
    const std::vector<std::string> pionPairs = {"piplus_piplus", "piplus_piminus", "piplus_pi0", "piminus_piminus", "piminus_pi0"};

    // Both run versions to check
    const std::vector<std::string> runVersions = {"Fall2018Spring2019_RGA_inbending", "Fall2018_RGA_outbending"};

    // Where all YAML will go
    const std::string outPath = "out/" + projectDir + "/asymmetry_results.yaml";

    bool append = false; // first dump truncates; subsequent dumps append

    for (const auto& pair : pionPairs) {
        for (const auto& runVersion : runVersions) {
            // Build and run the synthesizer
            Synthesizer synth(projectDir, pair, runVersion);
            synth.discoverConfigs();
            if (synth.getConfigsVector().empty()) { // skip if nothing found
                LOG_WARN("No configs for pair=" << pair << ", run=" << runVersion << " — skipping.");
                continue;
            }
            synth.runAll();

            // Set up the asymmetry handler
            AsymmetryHandler asym(synth.getResults(), synth.getConfigsMap());

            // Mutate for binMigration
            asym.setMutateBinMigration(true);

            // Region logic
            const bool hasPi0 = (pair.find("pi0") != std::string::npos);
            std::string regionName = hasPi0 ? Constants::DEFAULT_PI0_SIGNAL_REGION : "signal";
            std::string regionType = hasPi0 ? "signal" : "full";

            // Loop over pw = 0..11
            for (int pw = 0; pw <= 11; ++pw) {
                asym.reportAsymmetry(regionName, pw, regionType);
            }

            // Dump/append YAML
            asym.dumpYaml(outPath, append);
            append = true;
        }
    }

    std::cout << "Done!" << std::endl;
    return 0;
}
