#include "yyaml.hpp"

#include <iostream>
#include <string>

namespace {

yyaml::document build_document() {
    const char *yaml = R"(service:
  name: order-pipeline
  replicas: 3
  enabled: true
  features:
    - name: payments
      retries: 5
      timeout: 1.5
    - name: analytics
      retries: 2
      timeout: 0.75
metadata:
  owners:
    - alice
    - bob
  tags:
    environment: staging
    region: eu-west-1
)";

    return yyaml::document::parse(yaml);
}

bool compare_nodes(const yyaml::node &lhs, const yyaml::node &rhs, const std::string &path) {
    if (!lhs && !rhs) {
        std::cout << "[OK] " << path << " -> both null\n";
        return true;
    }
    if (!lhs || !rhs) {
        std::cout << "[FAIL] " << path << " -> missing node\n";
        return false;
    }

    if (lhs.get_type() != rhs.get_type()) {
        std::cout << "[FAIL] " << path << " -> type mismatch\n";
        return false;
    }

    if (lhs.is_scalar()) {
        const bool match = lhs.to_string() == rhs.to_string();
        std::cout << (match ? "[OK] " : "[FAIL] ") << path << " -> "
                  << lhs.to_string() << " == " << rhs.to_string() << "\n";
        return match;
    }

    if (lhs.is_sequence()) {
        if (lhs.size() != rhs.size()) {
            std::cout << "[FAIL] " << path << " -> sequence size mismatch\n";
            return false;
        }
        bool ok = true;
        for (std::size_t i = 0; i < lhs.size(); ++i) {
            ok &= compare_nodes(lhs.at(i), rhs.at(i), path + "/" + std::to_string(i));
        }
        return ok;
    }

    if (lhs.is_mapping()) {
        if (lhs.size() != rhs.size()) {
            std::cout << "[FAIL] " << path << " -> mapping size mismatch\n";
            return false;
        }
        bool ok = true;
        lhs.foreach([&](const std::string &key, yyaml::node child) {
            ok &= compare_nodes(child, rhs[key], path + "/" + key);
        });
        return ok;
    }

    return false;
}

} // namespace

int main() {
    try {
        auto doc = build_document();
        std::cout << "Original document:\n" << doc.dump() << "\n";

        const std::string serialized = doc.dump();
        auto roundtrip = yyaml::document::parse(serialized);

        const bool ok = compare_nodes(doc.root(), roundtrip.root(), "/");
        std::cout << (ok ? "Roundtrip comparison succeeded" : "Roundtrip comparison failed")
                  << "\n";
        return ok ? 0 : 1;
    } catch (const yyaml::yyaml_error &err) {
        std::cerr << "yyaml error: " << err.what() << "\n";
        return 1;
    }
}
