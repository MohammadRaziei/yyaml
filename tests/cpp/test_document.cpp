#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "yyaml.hpp"

#include <string>

namespace {

bool nodes_equal(const yyaml::node &lhs, const yyaml::node &rhs) {
    if (!lhs && !rhs) {
        return true;
    }
    if (!lhs || !rhs) {
        return false;
    }
    if (lhs.get_type() != rhs.get_type()) {
        return false;
    }
    if (lhs.isScalar()) {
        return lhs.toString() == rhs.toString();
    }
    if (lhs.isSequence()) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (std::size_t i = 0; i < lhs.size(); ++i) {
            if (!nodes_equal(lhs.at(i), rhs.at(i))) {
                return false;
            }
        }
        return true;
    }
    if (lhs.isMapping()) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        bool ok = true;
        lhs.forEachMember([&](const std::string &key, yyaml::node child) {
            if (!nodes_equal(child, rhs[key])) {
                ok = false;
            }
        });
        return ok;
    }
    return false;
}

} // namespace

TEST_CASE("document parses scalar and container nodes") {
    const char *yaml = R"(name: Example
count: 42
price: 13.37
active: true
items:
  - first
  - second
  - third
nested:
  inner: 2.5
  empty:
)";

    auto doc = yyaml::document::parse(yaml);
    auto root = doc.root();

    REQUIRE(root.isMapping());
    REQUIRE(root.size() == 6);

    auto name = root["name"];
    CHECK(name.isString());
    CHECK(name.asString() == "Example");

    auto count = root["count"];
    CHECK(count.isInt());
    CHECK(count.asInt() == 42);
    CHECK(count.isNumber());
    CHECK(count.asNumber() == doctest::Approx(42));

    auto price = root["price"];
    CHECK(price.isDouble());
    CHECK(price.asDouble() == doctest::Approx(13.37));

    auto active = root["active"];
    CHECK(active.isBool());
    CHECK(active.toString() == "true");

    auto items = root["items"];
    REQUIRE(items.isSequence());
    CHECK(items.size() == 3);
    CHECK(items.at(1).asString() == "second");

    auto nested = root["nested"];
    REQUIRE(nested.isMapping());
    CHECK(nested["inner"].asDouble() == doctest::Approx(2.5));
    CHECK(nested["empty"].isNull());
    CHECK_NOTHROW(nested["empty"].asNull());
}

TEST_CASE("document dump supports roundtrip serialization") {
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

    auto doc = yyaml::document::parse(yaml);
    auto serialized = doc.dump();
    CHECK(serialized.find("order-pipeline") != std::string::npos);

    auto roundtrip = yyaml::document::parse(serialized);
    CHECK(nodes_equal(doc.root(), roundtrip.root()));
}
