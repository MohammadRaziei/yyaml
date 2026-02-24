#include "utest/utest.h"
#include "yyaml.hpp"

#include <string>
#include <cmath>
#include <fstream>
#include <vector>
#include <filesystem>
#include <iostream>

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

UTEST(cpp_tests, document_parses_scalar_and_container_nodes) {
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

    ASSERT_TRUE(root.is_mapping());
    ASSERT_EQ(6, root.size());

    auto name = root["name"];
    ASSERT_TRUE(name.is_string());
    ASSERT_STREQ("Example", name.as_string().c_str());

    auto count = root["count"];
    ASSERT_TRUE(count.is_int());
    ASSERT_EQ(42, count.as_int());
    ASSERT_TRUE(count.is_number());
    ASSERT_TRUE(std::abs(count.as_number() - 42.0) < 0.0001);

    auto price = root["price"];
    ASSERT_TRUE(price.is_double());
    ASSERT_TRUE(std::abs(price.as_double() - 13.37) < 0.0001);

    auto active = root["active"];
    ASSERT_TRUE(active.is_bool());
    ASSERT_STREQ("true", active.to_string().c_str());

    auto items = root["items"];
    ASSERT_TRUE(items.is_sequence());
    ASSERT_EQ(3, items.size());
    ASSERT_STREQ("second", items.at(1).as_string().c_str());

    auto nested = root["nested"];
    ASSERT_TRUE(nested.is_mapping());
    ASSERT_TRUE(std::abs(nested["inner"].as_double() - 2.5) < 0.0001);
    ASSERT_TRUE(nested["empty"].is_null());
}

UTEST(cpp_tests, document_dump_supports_roundtrip_serialization) {
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
    ASSERT_TRUE(serialized.find("order-pipeline") != std::string::npos);

    auto roundtrip = yyaml::document::parse(serialized);
    ASSERT_TRUE(nodes_equal(doc.root(), roundtrip.root()));
}

UTEST(cpp_tests, node_to_string_emits_yaml_for_scalars_and_subtrees) {
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

    // scalar roundtrip
    {
        auto flag = root["flag"];
        auto serialized = flag.to_string();
        ASSERT_STREQ("false", serialized.c_str());
        auto roundtrip = yyaml::document::parse(serialized);
        ASSERT_TRUE(nodes_equal(flag, roundtrip.root()));
    }

    // mapping roundtrip
    {
        auto nested = root["nested"];
        auto serialized = nested.to_string();
        auto parsed = yyaml::document::parse(serialized);
        ASSERT_TRUE(nodes_equal(nested, parsed.root()));
    }

    // sequence roundtrip
    {
        auto list = root["list"];
        auto serialized = list.to_string();
        ASSERT_STREQ("- a\n- b\n- c", serialized.c_str());

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
        ASSERT_TRUE(nodes_equal(list, parsed.root()["wrapper"]));
    }

    // root equivalence
    {
        auto doc_root = doc.root();
        yyaml::write_opts opts{};
        opts.indent = 2;
        opts.final_newline = false;
        ASSERT_STREQ(doc_root.to_string().c_str(), doc.dump(&opts).c_str());
    }
}

UTEST(cpp_tests, node_empty_reflects_structure_and_scalar_content) {
    yyaml::node unbound;
    ASSERT_TRUE(unbound.empty());

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

    ASSERT_FALSE(root.empty());
    ASSERT_TRUE(root["missing"].empty());
    ASSERT_TRUE(root["empty_seq"].is_sequence());
    ASSERT_TRUE(root["empty_seq"].empty());
    ASSERT_FALSE(root["filled_map"].empty());
    ASSERT_FALSE(root["filled_seq"].empty());
    ASSERT_TRUE(root["blank"].empty());
    ASSERT_FALSE(root["text"].empty());
    ASSERT_TRUE(root["nullish"].empty());
    ASSERT_FALSE(root["flag"].empty());
}

UTEST(cpp_tests, node_iterator_walks_children_forward) {
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

    // sequence iteration
    {
        auto iter = root["items"].iter();
        auto first = iter.next();
        ASSERT_TRUE(first);
        ASSERT_STREQ("zero", first->as_string().c_str());

        auto second = iter.next();
        ASSERT_TRUE(second);
        ASSERT_STREQ("one", second->as_string().c_str());

        auto third = iter.next();
        ASSERT_TRUE(third);
        ASSERT_STREQ("two", third->as_string().c_str());

        ASSERT_FALSE(iter.next());
    }

    // mapping iteration yields values
    {
        auto iter = root["mapping"].iter();

        auto first = iter.next();
        ASSERT_TRUE(first);
        ASSERT_EQ(1, first->as_int());

        auto second = iter.next();
        ASSERT_TRUE(second);
        ASSERT_EQ(2, second->as_int());

        ASSERT_FALSE(iter.next());
    }

    // const iteration uses const_node_iterator
    {
        const yyaml::node const_items = root["items"];
        auto iter = const_items.iter();

        const yyaml::node *first = iter.next();
        ASSERT_TRUE(first);
        ASSERT_STREQ("zero", first->as_string().c_str());

        const yyaml::node *second = iter.next();
        ASSERT_TRUE(second);
        ASSERT_STREQ("one", second->as_string().c_str());

        const yyaml::node *third = iter.next();
        ASSERT_TRUE(third);
        ASSERT_STREQ("two", third->as_string().c_str());

        ASSERT_FALSE(iter.next());
    }
}

UTEST(cpp_tests, document_builder_constructs_nested_structures) {
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
    ASSERT_TRUE(nodes_equal(doc.root(), roundtrip.root()));

    auto built_root = doc.root();
    ASSERT_STREQ("builder-demo", built_root["title"].as_string().c_str());
    ASSERT_EQ(7, built_root["count"].as_int());
    ASSERT_TRUE(built_root["active"].as_bool());
    ASSERT_EQ(2, built_root["tags"].size());
    ASSERT_TRUE(built_root["meta"].is_mapping());
}

// Helper function to read file content
static std::string read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return "";
    }
    
    file.seekg(0, std::ios::end);
    std::size_t length = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::string buffer(length, '\0');
    file.read(&buffer[0], length);
    return buffer;
}

// Helper function to get test data directory
static std::string get_test_data_dir() {
    // Try to get from environment variable first
    const char* env_dir = std::getenv("YYAML_TEST_DATA_DIR");
    if (env_dir && env_dir[0] != '\0') {
        return env_dir;
    }
    
    // Fallback: relative path from current file
    std::filesystem::path current_file(__FILE__);
    std::filesystem::path data_dir = current_file.parent_path().parent_path() / "data";
    return data_dir.string();
}

// Test reading all YAML files in data directory (similar to Go and C tests)
UTEST(cpp_tests, test_read_all_data_files) {
    std::string data_dir = get_test_data_dir();
    std::cout << "Scanning data directory: " << data_dir << std::endl;
    
    // Check if directory exists
    if (!std::filesystem::exists(data_dir)) {
        std::cout << "Data directory does not exist: " << data_dir << std::endl;
        return;
    }
    
    // Collect all .yaml files
    std::vector<std::filesystem::path> yaml_files;
    for (const auto& entry : std::filesystem::directory_iterator(data_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".yaml") {
            yaml_files.push_back(entry.path());
        }
    }
    
    if (yaml_files.empty()) {
        std::cout << "No YAML files found in " << data_dir << std::endl;
        return;
    }
    
    // Process each file
    for (const auto& file_path : yaml_files) {
        std::string filename = file_path.filename().string();
        std::cout << "Testing file: " << filename << std::endl;
        
        // Read file content
        std::string yaml_content = read_file(file_path.string());
        ASSERT_TRUE(!yaml_content.empty());
        
        try {
            // Parse YAML using yyaml::document::parse
            auto doc = yyaml::document::parse(yaml_content);
            auto root = doc.root();
            
            // Basic validation - just ensure it parsed without error
            ASSERT_TRUE(doc.valid());
            
            if (!yaml_content.empty()) {
                ASSERT_TRUE(root.is_valid());
            }
            
            // Log success for debugging
            std::cout << "Successfully parsed " << filename << std::endl;
            
        } catch (const yyaml::yyaml_error& e) {
            // If parsing fails, the test should fail
            std::cerr << "Failed to parse " << filename << ": " << e.what() 
                      << " (line " << e.line << ", column " << e.column << ")" << std::endl;
            ASSERT_TRUE(false && "Failed to parse YAML file");
        }
    }
}
