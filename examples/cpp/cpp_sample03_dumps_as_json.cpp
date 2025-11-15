#include "yyaml.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

std::string escape_json(std::string_view value) {
    std::ostringstream out;
    out << '"';
    for (char ch : value) {
        switch (ch) {
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default: {
            const unsigned char uc = static_cast<unsigned char>(ch);
            if (uc < 0x20) {
                static const char *digits = "0123456789abcdef";
                out << "\\u00";
                out << digits[(uc >> 4) & 0xF];
                out << digits[uc & 0xF];
            } else {
                out << ch;
            }
            break;
        }
        }
    }
    out << '"';
    return out.str();
}

std::string node_to_json(const yyaml::node &node) {
    if (!node) {
        return "null";
    }
    if (node.is_null()) {
        return "null";
    }
    if (node.is_bool()) {
        return node.as_bool() ? "true" : "false";
    }
    if (node.is_int()) {
        return std::to_string(node.as_int());
    }
    if (node.is_double()) {
        std::ostringstream out;
        out << node.as_double();
        return out.str();
    }
    if (node.is_string()) {
        return escape_json(node.as_string());
    }
    if (node.is_sequence()) {
        std::string out = "[";
        for (std::size_t i = 0; i < node.size(); ++i) {
            if (i > 0) {
                out += ",";
            }
            out += node_to_json(node.at(i));
        }
        out += "]";
        return out;
    }
    if (node.is_mapping()) {
        std::string out = "{";
        bool first = true;
        node.for_each_member([&](const std::string &key, yyaml::node child) {
            if (!first) {
                out += ",";
            }
            out += escape_json(key);
            out += ":";
            out += node_to_json(child);
            first = false;
        });
        out += "}";
        return out;
    }
    return "null";
}

void dump_file_as_json(const std::string &path) {
    try {
        auto doc = yyaml::document::parse_file(path);
        std::cout << "=== " << path << " ===\n";
        std::cout << node_to_json(doc.root()) << "\n\n";
    } catch (const yyaml::yyaml_error &err) {
        std::cerr << "Failed to load " << path << ": " << err.what() << "\n";
    }
}

} // namespace

int main() {
    const auto data_dir = fs::path(__FILE__).parent_path().parent_path() / "data";
    const std::vector<std::string> files = {"app_config.yaml", "inventory.yml"};

    for (const auto &file : files) {
        dump_file_as_json((data_dir / file).string());
    }

    return 0;
}
