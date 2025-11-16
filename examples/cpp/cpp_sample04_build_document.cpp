#include "yyaml.hpp"

#include <iostream>
#include <string>

namespace {

yyaml::document build_release_manifest() {
    auto doc = yyaml::document::create();

    auto root = doc.add_mapping();
    doc.set_root(root);

    doc.map_append(root, "name", doc.add_string("yyaml-builder"));
    doc.map_append(root, "version", doc.add_string("1.0.0"));
    doc.map_append(root, "stable", doc.add_bool(true));

    auto targets = doc.add_sequence();
    doc.map_append(root, "targets", targets);

    auto linux_target = doc.add_mapping();
    doc.seq_append(targets, linux_target);
    doc.map_append(linux_target, "os", doc.add_string("linux"));
    doc.map_append(linux_target, "arch", doc.add_string("x86_64"));

    auto macos = doc.add_mapping();
    doc.seq_append(targets, macos);
    doc.map_append(macos, "os", doc.add_string("macos"));
    doc.map_append(macos, "arch", doc.add_string("arm64"));

    auto notes = doc.add_sequence();
    doc.map_append(root, "notes", notes);
    doc.seq_append(notes, doc.add_string("Built with the yyaml C builder API"));
    doc.seq_append(notes, doc.add_string("Demonstrates the C++ convenience layer"));

    return doc;
}

} // namespace

int main() {
    try {
        auto doc = build_release_manifest();
        std::cout << doc.dump();
        return 0;
    } catch (const yyaml::yyaml_error &err) {
        std::cerr << "yyaml error: " << err.what() << "\n";
        return 1;
    }
}
