#include <iostream>

#include "Synthesizer.h"

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " <projectDir> <pionPair> <runPeriod>\n";
    return 1;
  }
  Synthesizer synth(argv[1], argv[2], argv[3]);
  synth.discoverConfigs();
  // synth.runAll();
  // synth.synthesizeFinal();
  return 0;
}