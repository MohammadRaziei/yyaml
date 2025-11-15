#include "yyaml.hpp"

#include <iostream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    const std::string config_path = (fs::path(__FILE__).parent_path().parent_path() / "data" / "app_config.yaml").string();

    try {
        auto doc = yyaml::document::parse_file(config_path);
        auto root = doc.root();

        std::cout << "Loaded config from: " << config_path << "\n";

        if (auto name = root["name"]; name.is_scalar()) {
            std::cout << "Application: " << name.to_string() << "\n";
        }

        if (auto version = root["version"]; version) {
            std::cout << "Version: " << version.to_string() << "\n";
        }

        if (auto debug = root["debug"]; debug) {
            std::cout << "Debug mode: " << debug.to_string() << "\n";
        }

        if (auto features = root["features"]; features.is_sequence()) {
            std::cout << "Features:\n";
            for (std::size_t i = 0; i < features.size(); ++i) {
                auto item = features.at(i);
                if (item) {
                    std::cout << "  - " << item.to_string() << "\n";
                }
            }
        }

        if (auto db = root["database"]; db.is_mapping()) {
            std::cout << "Database host: " << db["host"].to_string() << "\n";
            std::cout << "Database port: " << db["port"].to_string() << "\n";
        }

    } catch (const yyaml::yyaml_error &err) {
        std::cerr << "yyaml error (line " << err.line << ":" << err.column << ") - "
                  << err.what() << "\n";
        return 1;
    }

    return 0;
}
