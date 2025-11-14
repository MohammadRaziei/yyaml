#include "yyaml.hpp"

#include <iostream>
#include <string>

int main() {
    const std::string config_path = std::string(YYAML_EXAMPLE_DATA_DIR) + "/app_config.yaml";

    try {
        auto doc = yyaml::document::parse_file(config_path);
        auto root = doc.root();

        std::cout << "Loaded config from: " << config_path << "\n";

        if (auto name = root["name"]; name.isScalar()) {
            std::cout << "Application: " << name.toString() << "\n";
        }

        if (auto version = root["version"]; version) {
            std::cout << "Version: " << version.toString() << "\n";
        }

        if (auto debug = root["debug"]; debug) {
            std::cout << "Debug mode: " << debug.toString() << "\n";
        }

        if (auto features = root["features"]; features.isSequence()) {
            std::cout << "Features:\n";
            for (std::size_t i = 0; i < features.size(); ++i) {
                auto item = features.at(i);
                if (item) {
                    std::cout << "  - " << item.toString() << "\n";
                }
            }
        }

        if (auto db = root["database"]; db.isMapping()) {
            std::cout << "Database host: " << db["host"].toString() << "\n";
            std::cout << "Database port: " << db["port"].toString() << "\n";
        }

    } catch (const yyaml::error &err) {
        std::cerr << "yyaml error (line " << err.line << ":" << err.column << ") - "
                  << err.what() << "\n";
        return 1;
    }

    return 0;
}
