#pragma once

#include <map>
#include <string>
#include <vector>

namespace Constants {

static const std::map<std::string, std::string> runToMc = {{"Fall2018_RGA_inbending", "MC_RGA_inbending"},
                                                           {"Fall2018_RGA_outbending", "MC_RGA_outbending"},
                                                           {"Fall2018Spring2019_RGA_inbending", "MC_RGA_inbending"}};

static const std::vector<std::string> validPairs = {"piplus_piplus", "piplus_piminus", "piplus_pi0", "piminus_pi0", "piminus_piminus"};

} // namespace Constants
