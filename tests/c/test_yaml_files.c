#include "unity.h"
#include "yyaml.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <limits.h>

// Test fixture
static yyaml_doc *doc = NULL;
static yyaml_err err = {0};

void setUp(void) {
    doc = NULL;
    memset(&err, 0, sizeof(err));
}

void tearDown(void) {
    if (doc) {
        yyaml_doc_free(doc);
        doc = NULL;
    }
}

// Helper function to read file content
static void build_full_path(char *buffer, size_t buffer_size, const char *relative) {
    const char *this_file = __FILE__;
    const char *last_slash = strrchr(this_file, '/');
    size_t dir_len = 0;

    if (last_slash) {
        dir_len = (size_t)(last_slash - this_file + 1);
        if (dir_len >= buffer_size) {
            dir_len = buffer_size - 1;
        }
        memcpy(buffer, this_file, dir_len);
    }

    buffer[dir_len] = '\0';

    if (relative) {
        size_t remaining = buffer_size - dir_len;
        if (remaining > 0) {
            int written = snprintf(buffer + dir_len, remaining, "%s", relative);
            if (written < 0 || (size_t)written >= remaining) {
                buffer[buffer_size - 1] = '\0';
            }
        }
    }
}

char* read_file(const char* filename) {
    char full_path[PATH_MAX];
    const char *path_to_use = filename;

    if (filename && filename[0] == '/') {
        path_to_use = filename;
    } else {
#ifdef YYAML_TEST_DATA_DIR
        const char prefix[] = "../tests/data/";
        const char *relative = filename ? filename : "";
        if (strncmp(relative, prefix, sizeof(prefix) - 1) == 0) {
            relative += sizeof(prefix) - 1;
        }
        int written = snprintf(full_path, sizeof(full_path), "%s/%s", YYAML_TEST_DATA_DIR, relative);
        if (written < 0 || (size_t)written >= sizeof(full_path)) {
            full_path[sizeof(full_path) - 1] = '\0';
        }
        path_to_use = full_path;
#else
        build_full_path(full_path, sizeof(full_path), filename);
        path_to_use = full_path;
#endif
    }

    FILE* file = fopen(path_to_use, "rb");
    if (!file) {
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = malloc(length + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    
    fclose(file);
    return buffer;
}

// Test loading simple scalars
void test_load_simple_scalars(void) {
    char* yaml_content = read_file("../tests/data/simple_scalars.yaml");
    TEST_ASSERT_NOT_NULL(yaml_content);
    
    doc = yyaml_read(yaml_content, strlen(yaml_content), NULL, &err);
    TEST_ASSERT_NOT_NULL_MESSAGE(doc, err.msg);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL(YYAML_MAPPING, root->type);
    
    // Test null value
    const yyaml_node *null_val = yyaml_map_get(root, "null_value");
    TEST_ASSERT_NOT_NULL(null_val);
    TEST_ASSERT_EQUAL(YYAML_NULL, null_val->type);
    
    // Test boolean true
    const yyaml_node *bool_true = yyaml_map_get(root, "boolean_true");
    TEST_ASSERT_NOT_NULL(bool_true);
    TEST_ASSERT_EQUAL(YYAML_BOOL, bool_true->type);
    TEST_ASSERT_TRUE(bool_true->val.boolean);
    
    // Test boolean false
    const yyaml_node *bool_false = yyaml_map_get(root, "boolean_false");
    TEST_ASSERT_NOT_NULL(bool_false);
    TEST_ASSERT_EQUAL(YYAML_BOOL, bool_false->type);
    TEST_ASSERT_FALSE(bool_false->val.boolean);
    
    // Test integer positive
    const yyaml_node *int_pos = yyaml_map_get(root, "integer_positive");
    TEST_ASSERT_NOT_NULL(int_pos);
    TEST_ASSERT_EQUAL(YYAML_INT, int_pos->type);
    TEST_ASSERT_EQUAL(42, int_pos->val.integer);
    
    // Test integer negative
    const yyaml_node *int_neg = yyaml_map_get(root, "integer_negative");
    TEST_ASSERT_NOT_NULL(int_neg);
    TEST_ASSERT_EQUAL(YYAML_INT, int_neg->type);
    TEST_ASSERT_EQUAL(-123, int_neg->val.integer);
    
    // Test float number
    const yyaml_node *float_val = yyaml_map_get(root, "float_number");
    TEST_ASSERT_NOT_NULL(float_val);
    TEST_ASSERT_EQUAL(YYAML_DOUBLE, float_val->type);
    TEST_ASSERT_TRUE(fabs(float_val->val.real - 3.14159) < 0.0001);
    
    free(yaml_content);
}

// Test loading sequences
void test_load_sequences(void) {
    char* yaml_content = read_file("../tests/data/sequences.yaml");
    TEST_ASSERT_NOT_NULL(yaml_content);
    
    doc = yyaml_read(yaml_content, strlen(yaml_content), NULL, &err);
    TEST_ASSERT_NOT_NULL_MESSAGE(doc, err.msg);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL(YYAML_MAPPING, root->type);
    
    // Test simple list
    const yyaml_node *simple_list = yyaml_map_get(root, "simple_list");
    TEST_ASSERT_NOT_NULL(simple_list);
    TEST_ASSERT_EQUAL(YYAML_SEQUENCE, simple_list->type);
    TEST_ASSERT_EQUAL(3, yyaml_seq_len(simple_list));
    
    // Test mixed types sequence
    const yyaml_node *mixed_types = yyaml_map_get(root, "mixed_types");
    TEST_ASSERT_NOT_NULL(mixed_types);
    TEST_ASSERT_EQUAL(YYAML_SEQUENCE, mixed_types->type);
    TEST_ASSERT_EQUAL(5, yyaml_seq_len(mixed_types));
    
    // Test inline sequence
    const yyaml_node *inline_seq = yyaml_map_get(root, "inline_sequence");
    TEST_ASSERT_NOT_NULL(inline_seq);
    TEST_ASSERT_EQUAL(YYAML_SEQUENCE, inline_seq->type);
    TEST_ASSERT_EQUAL(3, yyaml_seq_len(inline_seq));
    
    free(yaml_content);
}

// Test loading mappings
void test_load_mappings(void) {
    char* yaml_content = read_file("../tests/data/mappings.yaml");
    TEST_ASSERT_NOT_NULL(yaml_content);
    
    doc = yyaml_read(yaml_content, strlen(yaml_content), NULL, &err);
    TEST_ASSERT_NOT_NULL_MESSAGE(doc, err.msg);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL(YYAML_MAPPING, root->type);
    
    // Test simple map
    const yyaml_node *simple_map = yyaml_map_get(root, "simple_map");
    TEST_ASSERT_NOT_NULL(simple_map);
    TEST_ASSERT_EQUAL(YYAML_MAPPING, simple_map->type);
    TEST_ASSERT_EQUAL(3, yyaml_map_len(simple_map));
    
    // Test nested maps
    const yyaml_node *nested_maps = yyaml_map_get(root, "nested_maps");
    TEST_ASSERT_NOT_NULL(nested_maps);
    TEST_ASSERT_EQUAL(YYAML_MAPPING, nested_maps->type);
    
    const yyaml_node *person = yyaml_map_get(nested_maps, "person");
    TEST_ASSERT_NOT_NULL(person);
    TEST_ASSERT_EQUAL(YYAML_MAPPING, person->type);
    
    // Test complex structure
    const yyaml_node *complex_struct = yyaml_map_get(root, "complex_structure");
    TEST_ASSERT_NOT_NULL(complex_struct);
    TEST_ASSERT_EQUAL(YYAML_MAPPING, complex_struct->type);
    
    free(yaml_content);
}

// Test write/read cycle to /tmp
void test_write_read_cycle(void) {
    // First load a YAML file
    char* yaml_content = read_file("../tests/data/simple_scalars.yaml");
    TEST_ASSERT_NOT_NULL(yaml_content);
    
    doc = yyaml_read(yaml_content, strlen(yaml_content), NULL, &err);
    TEST_ASSERT_NOT_NULL_MESSAGE(doc, err.msg);
    
    // Write to /tmp
    char* output = NULL;
    size_t output_len = 0;
    const yyaml_node *root = yyaml_doc_get_root(doc);
    bool result = yyaml_write(root, &output, &output_len, NULL, &err);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(output);
    
    // Write to temporary file
    const char* tmp_file = "/tmp/test_yaml_roundtrip.yaml";
    FILE* tmp_fp = fopen(tmp_file, "w");
    TEST_ASSERT_NOT_NULL(tmp_fp);
    fwrite(output, 1, output_len, tmp_fp);
    fclose(tmp_fp);
    
    yyaml_free_string(output);
    
    // Read back from /tmp
    char* read_back_content = read_file(tmp_file);
    TEST_ASSERT_NOT_NULL(read_back_content);
    
    // Parse the read-back content
    yyaml_doc* read_back_doc = yyaml_read(read_back_content, strlen(read_back_content), NULL, &err);
    TEST_ASSERT_NOT_NULL_MESSAGE(read_back_doc, err.msg);
    
    // Compare the documents
    const yyaml_node *original_root = yyaml_doc_get_root(doc);
    const yyaml_node *read_back_root = yyaml_doc_get_root(read_back_doc);
    
    TEST_ASSERT_NOT_NULL(original_root);
    TEST_ASSERT_NOT_NULL(read_back_root);
    TEST_ASSERT_EQUAL(original_root->type, read_back_root->type);
    
    // Clean up
    yyaml_doc_free(read_back_doc);
    free(read_back_content);
    free(yaml_content);
    
    // Remove temporary file
    remove(tmp_file);
}

// Test writing and reading complex structure
void test_write_read_complex(void) {
    // Load complex structure
    char* yaml_content = read_file("../tests/data/mappings.yaml");
    TEST_ASSERT_NOT_NULL(yaml_content);
    
    doc = yyaml_read(yaml_content, strlen(yaml_content), NULL, &err);
    TEST_ASSERT_NOT_NULL_MESSAGE(doc, err.msg);
    
    // Write to /tmp
    char* output = NULL;
    size_t output_len = 0;
    const yyaml_node *root = yyaml_doc_get_root(doc);
    bool result = yyaml_write(root, &output, &output_len, NULL, &err);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(output);
    
    // Write to temporary file
    const char* tmp_file = "/tmp/test_complex_roundtrip.yaml";
    FILE* tmp_fp = fopen(tmp_file, "w");
    TEST_ASSERT_NOT_NULL(tmp_fp);
    fwrite(output, 1, output_len, tmp_fp);
    fclose(tmp_fp);
    
    yyaml_free_string(output);
    
    // Read back and verify
    char* read_back_content = read_file(tmp_file);
    TEST_ASSERT_NOT_NULL(read_back_content);
    
    yyaml_doc* read_back_doc = yyaml_read(read_back_content, strlen(read_back_content), NULL, &err);
    TEST_ASSERT_NOT_NULL_MESSAGE(read_back_doc, err.msg);
    
    // Verify structure is preserved
    const yyaml_node *original_root = yyaml_doc_get_root(doc);
    const yyaml_node *read_back_root = yyaml_doc_get_root(read_back_doc);
    
    TEST_ASSERT_NOT_NULL(original_root);
    TEST_ASSERT_NOT_NULL(read_back_root);
    TEST_ASSERT_EQUAL(original_root->type, read_back_root->type);
    
    // Clean up
    yyaml_doc_free(read_back_doc);
    free(read_back_content);
    free(yaml_content);
    
    // Remove temporary file
    remove(tmp_file);
}

// Test error handling with invalid files
void test_error_handling(void) {
    // Test with non-existent file
    char* yaml_content = read_file("../tests/data/nonexistent.yaml");
    TEST_ASSERT_NULL(yaml_content);
    
    // Test with empty content
    doc = yyaml_read("", 0, NULL, &err);
    TEST_ASSERT_NOT_NULL(doc);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL(YYAML_NULL, root->type);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_load_simple_scalars);
    RUN_TEST(test_load_sequences);
    RUN_TEST(test_load_mappings);
    RUN_TEST(test_write_read_cycle);
    RUN_TEST(test_write_read_complex);
    RUN_TEST(test_error_handling);
    
    return UNITY_END();
}
