#include "ConfigFile.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace YAML {
template <>
struct convert<ConfigFile> {
    // Config --> YAML::Node
    static Node encode(const ConfigFile& cfgFile) {
        Node node(NodeType::Map);

        for (const auto& [pair, cuts] : cfgFile.cutsByPair) {
            Node cutsNode(NodeType::Sequence);
            for (const auto& c : cuts)
                cutsNode.push_back(c);
            node[pair]["cuts"] = std::move(cutsNode);
        }
        return node;
    }

    // YAML::Node --> Config
    static bool decode(const Node& node, ConfigFile& cfgFile) {
        if (!node.IsMap())
            return false;

        cfgFile.cutsByPair.clear();
        for (const auto& kv : node) {
            const std::string pair = kv.first.as<std::string>();
            const Node& cutsNode = kv.second["cuts"];

            if (!cutsNode || !cutsNode.IsSequence())
                return false;

            std::vector<std::string> cuts;
            cuts.reserve(cutsNode.size());
            for (const auto& c : cutsNode)
                cuts.push_back(c.as<std::string>());

            cfgFile.cutsByPair.emplace(pair, std::move(cuts));
        }
        return true;
    }
};
} // namespace YAML

ConfigFile ConfigFile::loadFromFile(const std::string& yamlPath) {
    // Load the YAML file
    YAML::Node node = YAML::LoadFile(yamlPath);

    // Basic sanity check
    if (!node) {
        throw std::runtime_error("Failed to load config file: " + yamlPath);
    }

    // Build Config
    ConfigFile cfgFile = node.as<ConfigFile>();
    return cfgFile;
}

void ConfigFile::print() const {
    LOG_DEBUG("ConfigFile Contents:\n");
    for (const auto& [pairName, cuts] : cutsByPair) {
        LOG_DEBUG("Pair: " + pairName);
        LOG_DEBUG("  Cuts:");
        for (const auto& cut : cuts) {
            LOG_DEBUG("    - " + cut);
        }
    }
}
