#include "utest/utest.h"
#include "yyaml.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// Test basic scalar parsing
UTEST(yyaml_tests, test_parse_null) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "null";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_NULL, root->type);
    
    if (doc) yyaml_doc_free(doc);
}

UTEST(yyaml_tests, test_parse_boolean_true) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "true";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_BOOL, root->type);
    ASSERT_TRUE(root->val.boolean);
    
    if (doc) yyaml_doc_free(doc);
}

UTEST(yyaml_tests, test_parse_boolean_false) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "false";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_BOOL, root->type);
    ASSERT_FALSE(root->val.boolean);
    
    if (doc) yyaml_doc_free(doc);
}

UTEST(yyaml_tests, test_parse_integer) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "42";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_INT, root->type);
    ASSERT_EQ(42, root->val.integer);
    
    if (doc) yyaml_doc_free(doc);
}

UTEST(yyaml_tests, test_parse_negative_integer) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    // Negative integers need to be properly formatted
    const char *yaml = "-123";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    // This should fail because it looks like a sequence item
    ASSERT_TRUE(doc == NULL);
    
    // Test with a proper scalar negative integer
    const char *proper_yaml = "value: -123";
    doc = yyaml_read(proper_yaml, strlen(proper_yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_MAPPING, root->type);
    
    const yyaml_node *value = yyaml_map_get(root, "value");
    ASSERT_TRUE(value != NULL);
    ASSERT_EQ(YYAML_INT, value->type);
    ASSERT_EQ(-123, value->val.integer);
    
    if (doc) yyaml_doc_free(doc);
}

UTEST(yyaml_tests, test_parse_double) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "3.14";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_DOUBLE, root->type);
    // Use regular equality check
    ASSERT_TRUE(fabs(root->val.real - 3.14) < 0.0001);
    
    if (doc) yyaml_doc_free(doc);
}

UTEST(yyaml_tests, test_parse_string) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "hello world";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_STRING, root->type);
    
    const char *scalar_buf = yyaml_doc_get_scalar_buf(doc);
    ASSERT_TRUE(scalar_buf != NULL);
    ASSERT_STREQ("hello world", scalar_buf + root->val.str.ofs);
    
    if (doc) yyaml_doc_free(doc);
}

UTEST(yyaml_tests, test_parse_quoted_string) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "\"quoted string\"";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_STRING, root->type);
    
    const char *scalar_buf = yyaml_doc_get_scalar_buf(doc);
    ASSERT_TRUE(scalar_buf != NULL);
    ASSERT_STREQ("quoted string", scalar_buf + root->val.str.ofs);
    
    if (doc) yyaml_doc_free(doc);
}

// Test sequence parsing
UTEST(yyaml_tests, test_parse_simple_sequence) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    // The parser creates a mapping with a sequence inside
    const char *yaml = "items:\n  - item1\n  - item2\n  - 42";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    if (!doc) {
        printf("ERROR: Failed to parse sequence. Error: %s at pos %zu\n", err.msg, err.pos);
    }
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_MAPPING, root->type);
    
    const yyaml_node *items = yyaml_map_get(root, "items");
    ASSERT_TRUE(items != NULL);
    ASSERT_EQ(YYAML_SEQUENCE, items->type);
    ASSERT_EQ(3, yyaml_seq_len(items));

    const yyaml_node *item1 = yyaml_seq_get(items, 0);
    ASSERT_TRUE(item1 != NULL);
    ASSERT_EQ(YYAML_STRING, item1->type);
    
    const yyaml_node *item2 = yyaml_seq_get(items, 1);
    ASSERT_TRUE(item2 != NULL);
    ASSERT_EQ(YYAML_STRING, item2->type);
    
    const yyaml_node *item3 = yyaml_seq_get(items, 2);
    ASSERT_TRUE(item3 != NULL);
    ASSERT_EQ(YYAML_INT, item3->type);
    ASSERT_EQ(42, item3->val.integer);
    
    if (doc) yyaml_doc_free(doc);
}

// Test mapping parsing
UTEST(yyaml_tests, test_parse_simple_mapping) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "key1: value1\nkey2: 123\nkey3: true";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_MAPPING, root->type);
    ASSERT_EQ(3, yyaml_map_len(root));
    
    const yyaml_node *value1 = yyaml_map_get(root, "key1");
    ASSERT_TRUE(value1 != NULL);
    ASSERT_EQ(YYAML_STRING, value1->type);
    
    const yyaml_node *value2 = yyaml_map_get(root, "key2");
    ASSERT_TRUE(value2 != NULL);
    ASSERT_EQ(YYAML_INT, value2->type);
    ASSERT_EQ(123, value2->val.integer);
    
    const yyaml_node *value3 = yyaml_map_get(root, "key3");
    ASSERT_TRUE(value3 != NULL);
    ASSERT_EQ(YYAML_BOOL, value3->type);
    ASSERT_TRUE(value3->val.boolean);
    
    if (doc) yyaml_doc_free(doc);
}

// Test error handling
UTEST(yyaml_tests, test_parse_invalid_yaml) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    // Test with malformed YAML that should definitely fail
    const char *invalid_yaml = "key: value\n  - item"; // mixed mapping and sequence without proper structure
    doc = yyaml_read(invalid_yaml, strlen(invalid_yaml), NULL, &err);
    ASSERT_TRUE(doc == NULL);
    ASSERT_NE(0, err.pos);
}

UTEST(yyaml_tests, test_parse_empty_string) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_NULL, root->type);
    
    if (doc) yyaml_doc_free(doc);
}

// Test writing functionality
UTEST(yyaml_tests, test_write_simple_values) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    // Test writing a simple integer
    const char *yaml = "42";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    char *output = NULL;
    size_t output_len = 0;
    const yyaml_node *root = yyaml_doc_get_root(doc);
    bool result = yyaml_write(root, &output, &output_len, NULL, &err);
    ASSERT_TRUE(result);
    ASSERT_TRUE(output != NULL);
    ASSERT_STREQ("42\n", output);
    
    yyaml_free_string(output);
    if (doc) yyaml_doc_free(doc);
}

UTEST(yyaml_tests, test_write_sequence) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    // Test writing a nested sequence
    const char *yaml = "items:\n  - a\n  - b\n  - c";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    if (!doc) {
        printf("ERROR: Failed to parse sequence for writing. Error: %s at pos %zu\n", err.msg, err.pos);
    }
    ASSERT_TRUE(doc != NULL);
    
    char *output = NULL;
    size_t output_len = 0;
    const yyaml_node *root = yyaml_doc_get_root(doc);
    bool result = yyaml_write(root, &output, &output_len, NULL, &err);
    if (!result) {
        printf("ERROR: Failed to write sequence. Error: %s\n", err.msg);
    }
    ASSERT_TRUE(result);
    ASSERT_TRUE(output != NULL);
    
    // The output should contain the sequence items
    ASSERT_TRUE(strstr(output, "- a") != NULL);
    ASSERT_TRUE(strstr(output, "- b") != NULL);
    ASSERT_TRUE(strstr(output, "- c") != NULL);
    
    yyaml_free_string(output);
    if (doc) yyaml_doc_free(doc);
}

UTEST(yyaml_tests, test_write_preserves_decimal_suffix) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "value: 1.0";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);

    char *output = NULL;
    size_t output_len = 0;
    const yyaml_node *root = yyaml_doc_get_root(doc);
    bool result = yyaml_write(root, &output, &output_len, NULL, &err);
    ASSERT_TRUE(result);
    ASSERT_TRUE(output != NULL);
    ASSERT_TRUE(strstr(output, "1.0") != NULL);

    yyaml_free_string(output);
    if (doc) yyaml_doc_free(doc);
}

UTEST(yyaml_tests, test_write_sequence_of_maps_inlines_keys) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml =
        "items:\n"
        "  - id: 1001\n"
        "    name: Hammer\n"
        "  - id: 1002\n"
        "    name: Nails\n";

    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);

    char *output = NULL;
    size_t output_len = 0;
    const yyaml_node *root = yyaml_doc_get_root(doc);
    bool result = yyaml_write(root, &output, &output_len, NULL, &err);
    ASSERT_TRUE(result);
    ASSERT_TRUE(output != NULL);

    ASSERT_TRUE(strstr(output, "- id: 1001") != NULL);
    ASSERT_TRUE(strstr(output, "-\n    id: 1001") == NULL);
    ASSERT_TRUE(strstr(output, "- id: 1002") != NULL);
    ASSERT_TRUE(strstr(output, "-\n    id: 1002") == NULL);

    yyaml_free_string(output);
    if (doc) yyaml_doc_free(doc);
}

// Test utility functions
UTEST(yyaml_tests, test_is_scalar) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "test";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_TRUE(yyaml_is_scalar(root));
    ASSERT_FALSE(yyaml_is_container(root));
    
    if (doc) yyaml_doc_free(doc);
}

UTEST(yyaml_tests, test_is_container) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    // Test with a mapping container instead of sequence
    const char *yaml = "key: value";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_FALSE(yyaml_is_scalar(root));
    ASSERT_TRUE(yyaml_is_container(root));
    
    if (doc) yyaml_doc_free(doc);
}

UTEST(yyaml_tests, test_str_eq) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "hello";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_TRUE(yyaml_str_eq(doc, root, "hello"));
    ASSERT_FALSE(yyaml_str_eq(doc, root, "world"));
    
    if (doc) yyaml_doc_free(doc);
}

// Test node count
UTEST(yyaml_tests, test_node_count) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "key: value";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    ASSERT_EQ(2, yyaml_doc_node_count(doc)); // key and value nodes
    
    if (doc) yyaml_doc_free(doc);
}

// Test document get functions
UTEST(yyaml_tests, test_doc_get_functions) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    const char *yaml = "test";
    doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    
    const yyaml_node *node0 = yyaml_doc_get(doc, 0);
    ASSERT_TRUE(node0 != NULL);
    ASSERT_EQ(root, node0);
    
    const yyaml_node *invalid_node = yyaml_doc_get(doc, 999);
    ASSERT_TRUE(invalid_node == NULL);
    
    if (doc) yyaml_doc_free(doc);
}
