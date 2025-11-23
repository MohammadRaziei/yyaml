/*
 * Sample 04 - Iterate through the YAML data set in examples/data, load each
 * document, and dump its content as a JSON-like structure. This highlights how
 * to traverse nodes generically.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#    include <windows.h>
#else
#    include <dirent.h>
#    include <sys/types.h>
#endif

#include "common.h"
#include "yyaml.h"

static void print_indent(size_t depth) {
    size_t i;
    for (i = 0; i < depth; i++) {
        putchar(' ');
    }
}

static void print_escaped_json(const char *str, size_t len) {
    size_t i;
    putchar('"');
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        switch (c) {
        case '\\':
            fputs("\\\\", stdout);
            break;
        case '\"':
            fputs("\\\"", stdout);
            break;
        case '\n':
            fputs("\\n", stdout);
            break;
        case '\r':
            fputs("\\r", stdout);
            break;
        case '\t':
            fputs("\\t", stdout);
            break;
        default:
            if (c < 0x20) {
                static const char hex[] = "0123456789ABCDEF";
                putchar('\\');
                putchar('u');
                putchar(hex[(c >> 4) & 0xF]);
                putchar(hex[c & 0xF]);
            } else {
                putchar((char)c);
            }
            break;
        }
    }
    putchar('"');
}

static void dump_node_json(const yyaml_doc *doc, const yyaml_node *node, size_t depth);

static void dump_sequence_json(const yyaml_doc *doc, const yyaml_node *seq, size_t depth) {
    const yyaml_node *child = yyaml_doc_get(doc, seq->child);
    puts("[");
    while (child) {
        const yyaml_node *next = yyaml_doc_get(doc, child->next);
        print_indent(depth + 2);
        dump_node_json(doc, child, depth + 2);
        if (next) {
            putchar(',');
        }
        putchar('\n');
        child = next;
    }
    print_indent(depth);
    putchar(']');
}

static void dump_mapping_json(const yyaml_doc *doc, const yyaml_node *map, size_t depth) {
    const yyaml_node *child = yyaml_doc_get(doc, map->child);
    const char *scalars = yyaml_doc_get_scalar_buf(doc);
    puts("{");
    while (child) {
        const yyaml_node *next = yyaml_doc_get(doc, child->next);
        print_indent(depth + 2);
        if (scalars && child->flags > 0) {
            print_escaped_json(scalars + child->extra, child->flags);
        } else {
            fputs("\"<key>\"", stdout);
        }
        fputs(": ", stdout);
        dump_node_json(doc, child, depth + 2);
        if (next) {
            putchar(',');
        }
        putchar('\n');
        child = next;
    }
    print_indent(depth);
    putchar('}');
}

static void dump_node_json(const yyaml_doc *doc, const yyaml_node *node, size_t depth) {
    const char *scalars;
    if (!node) {
        fputs("null", stdout);
        return;
    }
    scalars = yyaml_doc_get_scalar_buf(doc);
    switch (node->type) {
    case YYAML_NULL:
        fputs("null", stdout);
        break;
    case YYAML_BOOL:
        fputs(node->val.boolean ? "true" : "false", stdout);
        break;
    case YYAML_INT:
        printf("%lld", (long long)node->val.integer);
        break;
    case YYAML_DOUBLE:
        printf("%g", node->val.real);
        break;
    case YYAML_STRING:
        if (scalars && node->val.str.len > 0) {
            print_escaped_json(scalars + node->val.str.ofs, node->val.str.len);
        } else {
            fputs("\"\"", stdout);
        }
        break;
    case YYAML_SEQUENCE:
        dump_sequence_json(doc, node, depth);
        break;
    case YYAML_MAPPING:
        dump_mapping_json(doc, node, depth);
        break;
    default:
        fputs("null", stdout);
        break;
    }
}

static bool has_yaml_extension(const char *name) {
    size_t len;
    if (!name) return false;
    len = strlen(name);
    if (len >= 5 && strcmp(name + len - 5, ".yaml") == 0) return true;
    if (len >= 4 && strcmp(name + len - 4, ".yml") == 0) return true;
    return false;
}

static void process_file(const char *path, const char *display_name) {
    char *file_data = NULL;
    size_t file_len = 0;
    yyaml_err err = {0};
    yyaml_doc *doc;

    if (!yyaml_example_read_file(path, &file_data, &file_len)) {
        fprintf(stderr, "Failed to read %s: %s\n", path, strerror(errno));
        return;
    }

    doc = yyaml_read(file_data, file_len, NULL, &err);
    if (!doc) {
        fprintf(stderr, "Failed to parse %s: %s (line %zu, column %zu)\n",
                path, err.msg, err.line, err.column);
        free(file_data);
        return;
    }

    printf("=== %s (%s) ===\n", display_name, path);
    printf("JSON dump of %s:\n", display_name);
    dump_node_json(doc, yyaml_doc_get_root(doc), 0);
    printf("\n\n");

    yyaml_doc_free(doc);
    free(file_data);
}

int main(void) {
    char data_dir[1024];

    if (!yyaml_example_build_data_path(__FILE__, "/../data", data_dir, sizeof(data_dir))) {
        fprintf(stderr, "Failed to resolve the examples/data directory relative to %s\n",
                __FILE__);
        return EXIT_FAILURE;
    }

    printf("Enumerating YAML files under: %s\n", data_dir);

#if defined(_WIN32)
    {
        char search_pattern[1024];
        WIN32_FIND_DATAA find_data;
        HANDLE handle;
        if (snprintf(search_pattern, sizeof(search_pattern), "%s\\*.y*", data_dir) < 0) {
            return EXIT_FAILURE;
        }
        handle = FindFirstFileA(search_pattern, &find_data);
        if (handle == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "Failed to enumerate %s (error %lu)\n", data_dir, GetLastError());
            return EXIT_FAILURE;
        }
        do {
            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                has_yaml_extension(find_data.cFileName)) {
                char path[2048];
                if (snprintf(path, sizeof(path), "%s\\%s", data_dir, find_data.cFileName) < 0) {
                    continue;
                }
                process_file(path, find_data.cFileName);
            }
        } while (FindNextFileA(handle, &find_data));
        FindClose(handle);
    }
#else
    {
        DIR *dir = opendir(data_dir);
        struct dirent *entry;
        if (!dir) {
            fprintf(stderr, "Failed to open %s: %s\n", data_dir, strerror(errno));
            return EXIT_FAILURE;
        }
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            if (!has_yaml_extension(entry->d_name)) continue;
            {
                char path[2048];
                if (snprintf(path, sizeof(path), "%s/%s", data_dir, entry->d_name) < 0) {
                    continue;
                }
                process_file(path, entry->d_name);
            }
        }
        closedir(dir);
    }
#endif

    return EXIT_SUCCESS;
}
