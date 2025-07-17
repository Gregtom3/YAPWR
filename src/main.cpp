#include "Logger.h"
#include "Synthesizer.h"
#include <iostream>

int main(int argc, char** argv) {

    Logger::setLevel(Logger::Level::Debug);

    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <projectDir> <pionPair> <runPeriod>\n";
        return 1;
    }
    Synthesizer synth(argv[1], argv[2], argv[3]);
    synth.discoverConfigs();
    synth.runAll();
    // synth.synthesizeFinal();
    std::cout << "Done!" << std::endl;
    return 0;
}
