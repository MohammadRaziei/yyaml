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
    if (lhs.is_scalar()) {
        return lhs.to_string() == rhs.to_string();
    }
    if (lhs.is_sequence()) {
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
    if (lhs.is_mapping()) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        bool ok = true;
        lhs.foreach([&](const std::string &key, yyaml::node child) {
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

    REQUIRE(root.is_mapping());
    REQUIRE(root.size() == 6);

    auto name = root["name"];
    CHECK(name.is_string());
    CHECK(name.as_string() == "Example");

    auto count = root["count"];
    CHECK(count.is_int());
    CHECK(count.as_int() == 42);
    CHECK(count.is_number());
    CHECK(count.as_number() == doctest::Approx(42));

    auto price = root["price"];
    CHECK(price.is_double());
    CHECK(price.as_double() == doctest::Approx(13.37));

    auto active = root["active"];
    CHECK(active.is_bool());
    CHECK(active.to_string() == "true");

    auto items = root["items"];
    REQUIRE(items.is_sequence());
    CHECK(items.size() == 3);
    CHECK(items.at(1).as_string() == "second");

    auto nested = root["nested"];
    REQUIRE(nested.is_mapping());
    CHECK(nested["inner"].as_double() == doctest::Approx(2.5));
    CHECK(nested["empty"].is_null());
    CHECK_NOTHROW(nested["empty"].as_null());
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

TEST_CASE("node to_string emits YAML for scalars and subtrees") {
    const char *yaml = R"(root:
  nested:
    value: 99
    empty: null
  list:
    - a
    - b
    - c
  flag: false
)";

    auto doc = yyaml::document::parse(yaml);
    auto root = doc.root()["root"];

    SUBCASE("scalar roundtrip") {
        auto flag = root["flag"];
        auto serialized = flag.to_string();
        CHECK(serialized == "false");
        auto roundtrip = yyaml::document::parse(serialized);
        CHECK(nodes_equal(flag, roundtrip.root()));
    }

    SUBCASE("mapping roundtrip") {
        auto nested = root["nested"];
        auto serialized = nested.to_string();
        CAPTURE(serialized);
        auto parsed = yyaml::document::parse(serialized);
        CHECK(nodes_equal(nested, parsed.root()));
    }

    SUBCASE("sequence roundtrip") {
        auto list = root["list"];
        auto serialized = list.to_string();
        CAPTURE(serialized);
        CHECK(serialized == "- a\n- b\n- c");

        auto indent = [](const std::string &src) {
            std::string out;
            out.reserve(src.size() * 2 + 2);
            out.append("  ");
            for (std::size_t i = 0; i < src.size(); ++i) {
                out.push_back(src[i]);
                if (src[i] == '\n' && i + 1 < src.size()) {
                    out.append("  ");
                }
            }
            return out;
        };

        auto wrapped_yaml = std::string("wrapper:\n") + indent(serialized);
        auto parsed = yyaml::document::parse(wrapped_yaml);
        CHECK(nodes_equal(list, parsed.root()["wrapper"]));
    }

    SUBCASE("root equivalence") {
        auto doc_root = doc.root();
        yyaml::write_opts opts{};
        opts.indent = 2;
        opts.final_newline = false;
        CHECK(doc_root.to_string() == doc.dump(&opts));
    }
}


TEST_CASE("node to_string emits YAML for scalars and subtrees") {
    const char *yaml = R"(root:
  nested:
    value: 99
    empty: null
  list:
    - a
    - b
    - c
  flag: false
)";

    auto doc = yyaml::document::parse(yaml);
    auto root = doc.root()["root"];

    SUBCASE("scalar roundtrip") {
        auto flag = root["flag"];
        auto serialized = flag.to_string();
        CHECK(serialized == "false");
        auto roundtrip = yyaml::document::parse(serialized);
        CHECK(nodes_equal(flag, roundtrip.root()));
    }

    SUBCASE("mapping roundtrip") {
        auto nested = root["nested"];
        auto serialized = nested.to_string();
        CAPTURE(serialized);
        auto parsed = yyaml::document::parse(serialized);
        CHECK(nodes_equal(nested, parsed.root()));
    }

    SUBCASE("sequence roundtrip") {
        auto list = root["list"];
        auto serialized = list.to_string();
        CAPTURE(serialized);
        CHECK(serialized == "- a\n- b\n- c");

        auto indent = [](const std::string &src) {
            std::string out;
            out.reserve(src.size() * 2 + 2);
            out.append("  ");
            for (std::size_t i = 0; i < src.size(); ++i) {
                out.push_back(src[i]);
                if (src[i] == '\n' && i + 1 < src.size()) {
                    out.append("  ");
                }
            }
            return out;
        };

        auto wrapped_yaml = std::string("wrapper:\n") + indent(serialized);
        auto parsed = yyaml::document::parse(wrapped_yaml);
        CHECK(nodes_equal(list, parsed.root()["wrapper"]));
    }

    SUBCASE("root equivalence") {
        auto doc_root = doc.root();
        yyaml::write_opts opts{};
        opts.indent = 2;
        opts.final_newline = false;
        CHECK(doc_root.to_string() == doc.dump(&opts));
    }
}

TEST_CASE("node empty reflects structure and scalar content") {
    yyaml::node unbound;
    CHECK(unbound.empty());

    const char *yaml = R"(empty_seq: []
filled_map:
  key: value
filled_seq:
  - one
blank: ""
text: hello
nullish: null
flag: false
)";

    auto doc = yyaml::document::parse(yaml);
    auto root = doc.root();

    CHECK_FALSE(root.empty());
    CHECK(root["missing"].empty());
    REQUIRE(root["empty_seq"].is_sequence());
    CHECK(root["empty_seq"].empty());
    CHECK_FALSE(root["filled_map"].empty());
    CHECK_FALSE(root["filled_seq"].empty());
    CHECK(root["blank"].empty());
    CHECK_FALSE(root["text"].empty());
    CHECK(root["nullish"].empty());
    CHECK_FALSE(root["flag"].empty());
}

TEST_CASE("node iterator walks children forward") {
    const char *yaml = R"(items:
  - zero
  - one
  - two
mapping:
  first: 1
  second: 2
)";

    auto doc = yyaml::document::parse(yaml);
    auto root = doc.root();

    SUBCASE("sequence iteration") {
        auto iter = root["items"].iter();
        auto first = iter.next();
        REQUIRE(first);
        CHECK(first->as_string() == "zero");

        auto second = iter.next();
        REQUIRE(second);
        CHECK(second->as_string() == "one");

        auto third = iter.next();
        REQUIRE(third);
        CHECK(third->as_string() == "two");

        CHECK(iter.next() == nullptr);
    }

    SUBCASE("mapping iteration yields values") {
        auto iter = root["mapping"].iter();

        auto first = iter.next();
        REQUIRE(first);
        CHECK(first->as_int() == 1);

        auto second = iter.next();
        REQUIRE(second);
        CHECK(second->as_int() == 2);

        CHECK(iter.next() == nullptr);
    }

    SUBCASE("const iteration uses const_node_iterator") {
        const yyaml::node const_items = root["items"];
        auto iter = const_items.iter();

        const yyaml::node *first = iter.next();
        REQUIRE(first);
        CHECK(first->as_string() == "zero");

        const yyaml::node *second = iter.next();
        REQUIRE(second);
        CHECK(second->as_string() == "one");

        const yyaml::node *third = iter.next();
        REQUIRE(third);
        CHECK(third->as_string() == "two");

        CHECK(iter.next() == nullptr);
    }
}

TEST_CASE("document builder constructs nested structures") {
    auto doc = yyaml::document::create();

    auto root = doc.add_mapping();
    doc.set_root(root);

    doc.map_append(root, "title", doc.add_string("builder-demo"));
    doc.map_append(root, "count", doc.add_int(7));
    doc.map_append(root, "active", doc.add_bool(true));

    auto tags = doc.add_sequence();
    doc.seq_append(tags, doc.add_string("alpha"));
    doc.seq_append(tags, doc.add_string("beta"));
    doc.map_append(root, "tags", tags);

    auto meta = doc.add_mapping();
    doc.map_append(meta, "version", doc.add_double(1.5));
    doc.map_append(meta, "notes", doc.add_null());
    doc.map_append(root, "meta", meta);

    auto roundtrip = yyaml::document::parse(doc.dump());
    CHECK(nodes_equal(doc.root(), roundtrip.root()));

    auto built_root = doc.root();
    CHECK(built_root["title"].as_string() == "builder-demo");
    CHECK(built_root["count"].as_int() == 7);
    CHECK(built_root["active"].as_bool());
    CHECK(built_root["tags"].size() == 2);
    CHECK(built_root["meta"].is_mapping());
}

