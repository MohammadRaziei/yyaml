#include "yyaml.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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
    if (node.isNull()) {
        return "null";
    }
    if (node.isBool()) {
        return node.asBool() ? "true" : "false";
    }
    if (node.isInt()) {
        return std::to_string(node.asInt());
    }
    if (node.isDouble()) {
        std::ostringstream out;
        out << node.asDouble();
        return out.str();
    }
    if (node.isString()) {
        return escape_json(node.asString());
    }
    if (node.isSequence()) {
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
    if (node.isMapping()) {
        std::string out = "{";
        bool first = true;
        node.forEachMember([&](const std::string &key, yyaml::node child) {
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
    } catch (const yyaml::error &err) {
        std::cerr << "Failed to load " << path << ": " << err.what() << "\n";
    }
}

} // namespace

int main() {
    const std::string data_dir = YYAML_EXAMPLE_DATA_DIR;
    const std::vector<std::string> files = {"app_config.yaml", "inventory.yml"};

    for (const auto &file : files) {
        dump_file_as_json(data_dir + "/" + file);
    }

    return 0;
}
