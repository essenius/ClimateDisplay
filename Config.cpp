#include <fstream>
#include <sstream>
#include <iostream>
#include "Config.h"

void Config::readStream(std::istream& inputStream) {
    std::string line;
    while (std::getline(inputStream, line)) {
        // ignore comments
        if (line.rfind('#', 0) == 0) {
            continue;
        }
        const size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string entity = line.substr(0, pos);
            const std::string value = line.substr(pos + 1);
            _config[entity] = value;
        }
    }
    if (_config.empty()) {
        std::cout << "Warning: did not find valid config data\n";
    }
}

bool Config::begin(const std::string& configInput, const std::string& hostName) {
    if (std::ifstream inputFile(configInput); !inputFile.is_open()) {
        std::stringstream stringStream(configInput);
        readStream(stringStream);
    } else {
        readStream(inputFile);
    }
    return setDevice(hostName);
}

bool Config::setDevice(std::string_view hostName) {
    if (const auto device = getEntry("device"); !device.empty()) return true;
    if (hostName.empty()) {
        std::cerr << "'device' not set in config, and failed to determine device name\n";
        return false;
    }
    _config["device"] = hostName;
    return true;
}

std::string Config::getEntry(const std::string& key, const std::string& defaultValue) const {
	const auto iterator = _config.find(key);
    if (iterator == _config.end()) {
        return defaultValue;
    }
    return iterator->second;
}
