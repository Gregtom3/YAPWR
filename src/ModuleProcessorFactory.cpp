#include "ModuleProcessorFactory.h"
ModuleProcessorFactory& ModuleProcessorFactory::instance() {
    static ModuleProcessorFactory inst;
    return inst;
}
void ModuleProcessorFactory::registerProcessor(const std::string& name, Creator c) {
    creators_[name] = c;
}
std::unique_ptr<ModuleProcessor> ModuleProcessorFactory::create(const std::string& name) const {
    auto it = creators_.find(name);
    if (it == creators_.end())
        throw std::runtime_error("No processor: " + name);
    return it->second();
}