#include "utest/utest.h"
#include "yyaml.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>

// For directory operations
#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/types.h>
#endif

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

// Function to read a file (platform-independent interface)
char* read_file(const char* filename) {
    char full_path[4096]; // Use a reasonable buffer size
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

// Structure to hold file information
typedef struct {
    char name[256];
    int is_directory;
} file_info_t;

// Callback function type for directory traversal
typedef void (*file_callback_t)(const char* file_path, const char* file_name, void* user_data);

// Platform-independent function to traverse directory and call callback for each file
static int traverse_directory(const char* dir_path, const char* extension, 
                             file_callback_t callback, void* user_data) {
#if defined(_WIN32)
    // Windows implementation
    char search_pattern[4096];
    WIN32_FIND_DATAA find_data;
    HANDLE handle;
    
    if (snprintf(search_pattern, sizeof(search_pattern), "%s\\*%s", dir_path, extension) < 0) {
        return -1;
    }
    
    handle = FindFirstFileA(search_pattern, &find_data);
    if (handle == INVALID_HANDLE_VALUE) {
        return 0; // No files found
    }
    
    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            char file_path[4096];
            if (snprintf(file_path, sizeof(file_path), "%s\\%s", dir_path, find_data.cFileName) < 0) {
                continue;
            }
            callback(file_path, find_data.cFileName, user_data);
        }
    } while (FindNextFileA(handle, &find_data));
    
    FindClose(handle);
    return 1;
#else
    // POSIX implementation
    DIR *dir = opendir(dir_path);
    if (!dir) {
        return -1;
    }
    
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files and directories
        if (entry->d_name[0] == '.') {
            continue;
        }
        
        // Check for extension
        size_t name_len = strlen(entry->d_name);
        size_t ext_len = strlen(extension);
        if (name_len >= ext_len && strcmp(entry->d_name + name_len - ext_len, extension) == 0) {
            char file_path[4096];
            if (snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name) < 0) {
                continue;
            }
            callback(file_path, entry->d_name, user_data);
            count++;
        }
    }
    
    closedir(dir);
    return count;
#endif
}

// Helper structure for collecting file names
typedef struct {
    char** files;
    size_t count;
    size_t capacity;
} file_list_t;

// Callback to collect file names
static void collect_file_callback(const char* file_path, const char* file_name, void* user_data) {
    file_list_t* list = (file_list_t*)user_data;
    
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        char** new_files = realloc(list->files, new_capacity * sizeof(char*));
        if (!new_files) return;
        list->files = new_files;
        list->capacity = new_capacity;
    }
    
    list->files[list->count] = strdup(file_name);
    if (list->files[list->count]) {
        list->count++;
    }
}

// Function to get list of files with given extension in directory
// Returns array of strings (caller must free with free_file_list)
char** list_files_in_directory(const char* dir_path, const char* extension, size_t* count) {
    file_list_t list = {0};
    
    int result = traverse_directory(dir_path, extension, collect_file_callback, &list);
    if (result < 0) {
        *count = 0;
        return NULL;
    }
    
    *count = list.count;
    return list.files;
}

// Free the file list returned by list_files_in_directory
void free_file_list(char** files, size_t count) {
    if (!files) return;
    
    for (size_t i = 0; i < count; i++) {
        free(files[i]);
    }
    free(files);
}

// Test loading simple scalars
UTEST(yyaml_file_tests, test_load_simple_scalars) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    char* yaml_content = read_file("../tests/data/simple_scalars.yaml");
    ASSERT_TRUE(yaml_content != NULL);
    
    doc = yyaml_read(yaml_content, strlen(yaml_content), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_MAPPING, root->type);
    
    // Test null value
    const yyaml_node *null_val = yyaml_map_get(root, "null_value");
    ASSERT_TRUE(null_val != NULL);
    ASSERT_EQ(YYAML_NULL, null_val->type);
    
    // Test boolean true
    const yyaml_node *bool_true = yyaml_map_get(root, "boolean_true");
    ASSERT_TRUE(bool_true != NULL);
    ASSERT_EQ(YYAML_BOOL, bool_true->type);
    ASSERT_TRUE(bool_true->val.boolean);
    
    // Test boolean false
    const yyaml_node *bool_false = yyaml_map_get(root, "boolean_false");
    ASSERT_TRUE(bool_false != NULL);
    ASSERT_EQ(YYAML_BOOL, bool_false->type);
    ASSERT_FALSE(bool_false->val.boolean);
    
    // Test integer positive
    const yyaml_node *int_pos = yyaml_map_get(root, "integer_positive");
    ASSERT_TRUE(int_pos != NULL);
    ASSERT_EQ(YYAML_INT, int_pos->type);
    ASSERT_EQ(42, int_pos->val.integer);
    
    // Test integer negative
    const yyaml_node *int_neg = yyaml_map_get(root, "integer_negative");
    ASSERT_TRUE(int_neg != NULL);
    ASSERT_EQ(YYAML_INT, int_neg->type);
    ASSERT_EQ(-123, int_neg->val.integer);
    
    // Test float number
    const yyaml_node *float_val = yyaml_map_get(root, "float_number");
    ASSERT_TRUE(float_val != NULL);
    ASSERT_EQ(YYAML_DOUBLE, float_val->type);
    ASSERT_TRUE(fabs(float_val->val.real - 3.14159) < 0.0001);
    
    free(yaml_content);
    if (doc) yyaml_doc_free(doc);
}

// Test loading sequences
UTEST(yyaml_file_tests, test_load_sequences) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    char* yaml_content = read_file("../tests/data/sequences.yaml");
    ASSERT_TRUE(yaml_content != NULL);
    
    doc = yyaml_read(yaml_content, strlen(yaml_content), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_MAPPING, root->type);
    
    // Test simple list
    const yyaml_node *simple_list = yyaml_map_get(root, "simple_list");
    ASSERT_TRUE(simple_list != NULL);
    ASSERT_EQ(YYAML_SEQUENCE, simple_list->type);
    ASSERT_EQ(3, yyaml_seq_len(simple_list));
    
    // Test mixed types sequence
    const yyaml_node *mixed_types = yyaml_map_get(root, "mixed_types");
    ASSERT_TRUE(mixed_types != NULL);
    ASSERT_EQ(YYAML_SEQUENCE, mixed_types->type);
    ASSERT_EQ(5, yyaml_seq_len(mixed_types));
    
    // Test inline sequence
    const yyaml_node *inline_seq = yyaml_map_get(root, "inline_sequence");
    ASSERT_TRUE(inline_seq != NULL);
    ASSERT_EQ(YYAML_SEQUENCE, inline_seq->type);
    ASSERT_EQ(3, yyaml_seq_len(inline_seq));
    
    free(yaml_content);
    if (doc) yyaml_doc_free(doc);
}

// Test loading mappings
UTEST(yyaml_file_tests, test_load_mappings) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    char* yaml_content = read_file("../tests/data/mappings.yaml");
    ASSERT_TRUE(yaml_content != NULL);
    
    doc = yyaml_read(yaml_content, strlen(yaml_content), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_MAPPING, root->type);
    
    // Test simple map
    const yyaml_node *simple_map = yyaml_map_get(root, "simple_map");
    ASSERT_TRUE(simple_map != NULL);
    ASSERT_EQ(YYAML_MAPPING, simple_map->type);
    ASSERT_EQ(3, yyaml_map_len(simple_map));
    
    // Test nested maps
    const yyaml_node *nested_maps = yyaml_map_get(root, "nested_maps");
    ASSERT_TRUE(nested_maps != NULL);
    ASSERT_EQ(YYAML_MAPPING, nested_maps->type);
    
    const yyaml_node *person = yyaml_map_get(nested_maps, "person");
    ASSERT_TRUE(person != NULL);
    ASSERT_EQ(YYAML_MAPPING, person->type);
    
    // Test complex structure
    const yyaml_node *complex_struct = yyaml_map_get(root, "complex_structure");
    ASSERT_TRUE(complex_struct != NULL);
    ASSERT_EQ(YYAML_MAPPING, complex_struct->type);
    
    free(yaml_content);
    if (doc) yyaml_doc_free(doc);
}

// Test write/read cycle to /tmp
UTEST(yyaml_file_tests, test_write_read_cycle) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    // First load a YAML file
    char* yaml_content = read_file("../tests/data/simple_scalars.yaml");
    ASSERT_TRUE(yaml_content != NULL);
    
    doc = yyaml_read(yaml_content, strlen(yaml_content), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    // Write to /tmp
    char* output = NULL;
    size_t output_len = 0;
    const yyaml_node *root = yyaml_doc_get_root(doc);
    bool result = yyaml_write(root, &output, &output_len, NULL, &err);
    ASSERT_TRUE(result);
    ASSERT_TRUE(output != NULL);
    
    // Write to temporary file
    const char* tmp_file = "/tmp/test_yaml_roundtrip.yaml";
    FILE* tmp_fp = fopen(tmp_file, "w");
    ASSERT_TRUE(tmp_fp != NULL);
    fwrite(output, 1, output_len, tmp_fp);
    fclose(tmp_fp);
    
    yyaml_free_string(output);
    
    // Read back from /tmp
    char* read_back_content = read_file(tmp_file);
    ASSERT_TRUE(read_back_content != NULL);
    
    // Parse the read-back content
    yyaml_doc* read_back_doc = yyaml_read(read_back_content, strlen(read_back_content), NULL, &err);
    ASSERT_TRUE(read_back_doc != NULL);
    
    // Compare the documents
    const yyaml_node *original_root = yyaml_doc_get_root(doc);
    const yyaml_node *read_back_root = yyaml_doc_get_root(read_back_doc);
    
    ASSERT_TRUE(original_root != NULL);
    ASSERT_TRUE(read_back_root != NULL);
    ASSERT_EQ(original_root->type, read_back_root->type);
    
    // Clean up
    yyaml_doc_free(read_back_doc);
    free(read_back_content);
    free(yaml_content);
    
    // Remove temporary file
    remove(tmp_file);
    if (doc) yyaml_doc_free(doc);
}

// Test writing and reading complex structure
UTEST(yyaml_file_tests, test_write_read_complex) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};
    
    // Load complex structure
    char* yaml_content = read_file("../tests/data/mappings.yaml");
    ASSERT_TRUE(yaml_content != NULL);
    
    doc = yyaml_read(yaml_content, strlen(yaml_content), NULL, &err);
    ASSERT_TRUE(doc != NULL);
    
    // Write to /tmp
    char* output = NULL;
    size_t output_len = 0;
    const yyaml_node *root = yyaml_doc_get_root(doc);
    bool result = yyaml_write(root, &output, &output_len, NULL, &err);
    ASSERT_TRUE(result);
    ASSERT_TRUE(output != NULL);
    
    // Write to temporary file
    const char* tmp_file = "/tmp/test_complex_roundtrip.yaml";
    FILE* tmp_fp = fopen(tmp_file, "w");
    ASSERT_TRUE(tmp_fp != NULL);
    fwrite(output, 1, output_len, tmp_fp);
    fclose(tmp_fp);
    
    yyaml_free_string(output);
    
    // Read back and verify
    char* read_back_content = read_file(tmp_file);
    ASSERT_TRUE(read_back_content != NULL);
    
    yyaml_doc* read_back_doc = yyaml_read(read_back_content, strlen(read_back_content), NULL, &err);
    ASSERT_TRUE(read_back_doc != NULL);
    
    // Verify structure is preserved
    const yyaml_node *original_root = yyaml_doc_get_root(doc);
    const yyaml_node *read_back_root = yyaml_doc_get_root(read_back_doc);
    
    ASSERT_TRUE(original_root != NULL);
    ASSERT_TRUE(read_back_root != NULL);
    ASSERT_EQ(original_root->type, read_back_root->type);
    
    // Clean up
    yyaml_doc_free(read_back_doc);
    free(read_back_content);
    free(yaml_content);
    
    // Remove temporary file
    remove(tmp_file);
    if (doc) yyaml_doc_free(doc);
}

// Test error handling with invalid files
UTEST(yyaml_file_tests, test_error_handling) {
    yyaml_doc *doc = NULL;
    yyaml_err err = {0};

    // Test with non-existent file
    char* yaml_content = read_file("../tests/data/nonexistent.yaml");
    ASSERT_TRUE(yaml_content == NULL);

    // Test with empty content
    doc = yyaml_read("", 0, NULL, &err);
    ASSERT_TRUE(doc != NULL);

    const yyaml_node *root = yyaml_doc_get_root(doc);
    ASSERT_TRUE(root != NULL);
    ASSERT_EQ(YYAML_NULL, root->type);

    if (doc) yyaml_doc_free(doc);
}

// Test reading all YAML files in data directory (similar to Go test)
UTEST(yyaml_file_tests, test_read_all_data_files) {
    // Get data directory path
#ifdef YYAML_TEST_DATA_DIR
    const char* data_dir = YYAML_TEST_DATA_DIR;
#else
    // Fallback: build path relative to this file
    char data_dir[4096];
    build_full_path(data_dir, sizeof(data_dir), "../tests/data");
#endif
    
    printf("Scanning data directory: %s\n", data_dir);
    
    // Use our platform-independent function to list files
    size_t file_count = 0;
    char** files = list_files_in_directory(data_dir, ".yaml", &file_count);
    
    if (files == NULL || file_count == 0) {
        printf("No YAML files found in %s\n", data_dir);
        if (files) free_file_list(files, file_count);
        return;
    }
    
    // Process each file
    for (size_t i = 0; i < file_count; i++) {
        printf("Testing file: %s\n", files[i]);
        
        // Build full path
        char file_path[4096];
        if (snprintf(file_path, sizeof(file_path), "%s/%s", data_dir, files[i]) < 0) {
            continue;
        }
        
        // Read YAML file
        char* yaml_content = read_file(file_path);
        ASSERT_TRUE(yaml_content != NULL);
        
        // Parse YAML using yyaml_read
        yyaml_doc *doc = NULL;
        yyaml_err err = {0};
        doc = yyaml_read(yaml_content, strlen(yaml_content), NULL, &err);
        
        // Basic validation - just ensure it parsed without error
        ASSERT_TRUE(doc != NULL);
        
        const yyaml_node *root = yyaml_doc_get_root(doc);
        if (strlen(yaml_content) > 0) {
            ASSERT_TRUE(root != NULL);
        }
        
        // Log success for debugging
        printf("Successfully parsed %s\n", files[i]);
        
        // Clean up
        if (doc) yyaml_doc_free(doc);
        free(yaml_content);
    }
    
    // Clean up file list
    free_file_list(files, file_count);
}

// Test roundtrip for all YAML files in data directory
UTEST(yyaml_file_tests, test_roundtrip_all_data_files) {
    // Get data directory path
#ifdef YYAML_TEST_DATA_DIR
    const char* data_dir = YYAML_TEST_DATA_DIR;
#else
    // Fallback: build path relative to this file
    char data_dir[4096];
    build_full_path(data_dir, sizeof(data_dir), "../tests/data");
#endif
    
    printf("Scanning data directory for roundtrip tests: %s\n", data_dir);
    
    // Use our platform-independent function to list files
    size_t file_count = 0;
    char** files = list_files_in_directory(data_dir, ".yaml", &file_count);
    
    ASSERT_TRUE(files != NULL);
    ASSERT_TRUE(file_count > 0);
    
    // Process each file
    for (size_t i = 0; i < file_count; i++) {
        printf("Testing roundtrip for file: %s\n", files[i]);
        
        // Build full path
        char file_path[4096];
        ASSERT_TRUE(snprintf(file_path, sizeof(file_path), "%s/%s", data_dir, files[i]) >= 0);
        
        // Read YAML file
        char* yaml_content = read_file(file_path);
        ASSERT_TRUE(yaml_content != NULL);
        
        // First parse using yyaml_read
        yyaml_doc *doc = NULL;
        yyaml_err err = {0};
        doc = yyaml_read(yaml_content, strlen(yaml_content), NULL, &err);
        ASSERT_TRUE(doc != NULL);
        
        const yyaml_node *root = yyaml_doc_get_root(doc);
        if (strlen(yaml_content) > 0) {
            ASSERT_TRUE(root != NULL);
        }
        
        // Write to string using yyaml_write
        char* output = NULL;
        size_t output_len = 0;
        bool result = yyaml_write(root, &output, &output_len, NULL, &err);
        ASSERT_TRUE(result);
        ASSERT_TRUE(output != NULL);
        
        // Parse again to verify
        yyaml_doc* doc2 = yyaml_read(output, output_len, NULL, &err);
        ASSERT_TRUE(doc2 != NULL);
        
        // Log success for debugging
        printf("Successfully roundtripped %s\n", files[i]);
        
        // Clean up
        yyaml_doc_free(doc2);
        yyaml_free_string(output);
        yyaml_doc_free(doc);
        free(yaml_content);
    }
    
    // Clean up file list
    free_file_list(files, file_count);
}
