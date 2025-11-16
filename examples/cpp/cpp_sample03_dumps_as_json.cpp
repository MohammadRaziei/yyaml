#include "yyaml.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

std::string escape_json_string(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 10);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c >= 0 && c <= 0x1F) {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

std::string node_to_json(const yyaml::node& node, int depth = 0);

std::string node_to_json(const yyaml::node& node, int depth) {
    if (!node) {
        return "null";
    }
    if (node.is_string()) {
        return "\"" + escape_json_string(node.as_string()) + "\"";
    }
    if (node.is_scalar()) {
        return node.to_string();
    }
    if (node.is_sequence()) {
        if (node.empty()) {
            return "[]";
        }
        std::string out = "[\n";
        for (std::size_t i = 0; i < node.size(); ++i) {
            if (i > 0) {
                out += ",\n";
            }
            out += std::string((depth + 1) * 2, ' ');
            out += node_to_json(node[i], depth + 1);
        }
        out += "\n" + std::string(depth * 2, ' ') + "]";
        return out;
    }
    if (node.is_mapping()) {
        if (node.empty()) {
            return "{}";
        }
        std::string out = "{\n";
        bool first = true;
        node.foreach([&](const std::string& key, yyaml::node val) {
            if (!first) {
                out += ",\n";
            }
            out += std::string((depth + 1) * 2, ' ');
            out += "\"" + escape_json_string(key) + "\": ";
            out += node_to_json(val, depth + 1);
            first = false;
        });
        out += "\n" + std::string(depth * 2, ' ') + "}";
        return out;
    }
    return "null";
}

void dump_file_as_json(const std::string& path) {
    try {
        auto doc = yyaml::document::parse_file(path);
        std::cout << "=== " << path << " ===\n";
        std::cout << doc.root().to_string() << "\n";
        std::cout << "===\n";
        std::cout << node_to_json(doc.root()) << "\n\n";
    } catch (const yyaml::yyaml_error& err) {
        std::cerr << "Failed to load " << path << ": " << err.what() << "\n";
    }
}

int main() {
    const auto data_dir = fs::path(__FILE__).parent_path().parent_path() / "data";
    const std::vector<std::string> files = {"app_config.yaml", "inventory.yml"};

    for (const auto& file : files) {
        dump_file_as_json((data_dir / file).string());
    }

    return 0;
}