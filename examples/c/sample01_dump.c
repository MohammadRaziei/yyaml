/*
 * This code is for Mohammad Raziei (https://github.com/mohammadraziei/yyaml).
 * Written on 2025-11-06 and released under the MIT license.
 * If you use it, please star the repository and report issues via GitHub.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "yyaml.h"

static void print_indent(size_t depth) {
    size_t i;
    for (i = 0; i < depth; i++) {
        putchar(' ');
    }
}

static void print_escaped(const char *str, size_t len) {
    size_t i;
    putchar('"');
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        switch (c) {
        case '\\':
            fputs("\\\\", stdout);
            break;
        case '"':
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

static void print_node(const yyaml_doc *doc, const yyaml_node *node, size_t depth);

static void print_sequence(const yyaml_doc *doc, const yyaml_node *seq, size_t depth) {
    const yyaml_node *child;
    printf("[\n");
    child = yyaml_doc_get(doc, seq->child);
    while (child) {
        const yyaml_node *next;
        print_indent(depth + 2);
        print_node(doc, child, depth + 2);
        next = yyaml_doc_get(doc, child->next);
        if (next) {
            putchar(',');
        }
        putchar('\n');
        child = next;
    }
    print_indent(depth);
    putchar(']');
}

static void print_mapping(const yyaml_doc *doc, const yyaml_node *map, size_t depth) {
    const yyaml_node *child;
    const char *scalars = yyaml_doc_get_scalar_buf(doc);
    printf("{\n");
    child = yyaml_doc_get(doc, map->child);
    while (child) {
        const yyaml_node *next = yyaml_doc_get(doc, child->next);
        print_indent(depth + 2);
        if (scalars && child->flags > 0) {
            print_escaped(scalars + child->extra, child->flags);
        } else {
            fputs("\"<key>\"", stdout);
        }
        fputs(": ", stdout);
        print_node(doc, child, depth + 2);
        if (next) {
            putchar(',');
        }
        putchar('\n');
        child = next;
    }
    print_indent(depth);
    putchar('}');
}

static void print_node(const yyaml_doc *doc, const yyaml_node *node, size_t depth) {
    const char *scalars;
    if (!node) {
        fputs("<null>", stdout);
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
            print_escaped(scalars + node->val.str.ofs, node->val.str.len);
        } else {
            fputs("\"\"", stdout);
        }
        break;
    case YYAML_SEQUENCE:
        print_sequence(doc, node, depth);
        break;
    case YYAML_MAPPING:
        print_mapping(doc, node, depth);
        break;
    default:
        fputs("<unknown>", stdout);
        break;
    }
}

int main(int argc, char **argv) {
    char default_path[1024];
    const char *path;

    if (argc > 1) {
        path = argv[1];
    } else {
        if (!yyaml_example_build_data_path(__FILE__, "/../data/app_config.yaml",
                                           default_path, sizeof(default_path))) {
            fprintf(stderr, "failed to resolve default YAML path based on %s\n", __FILE__);
            return EXIT_FAILURE;
        }
        path = default_path;
    }
    char *data = NULL;
    size_t len = 0;
    yyaml_err err = {0};
    yyaml_doc *doc;

    if (!yyaml_example_read_file(path, &data, &len)) {
        fprintf(stderr, "Failed to read %s: %s\n", path, strerror(errno));
        return 1;
    }

    doc = yyaml_read(data, len, NULL, &err);
    if (!doc) {
        fprintf(stderr, "Failed to parse YAML: %s at line %zu, column %zu\n",
                err.msg, err.line, err.column);
        free(data);
        return 1;
    }

    print_node(doc, yyaml_doc_get_root(doc), 0);
    putchar('\n');

    yyaml_doc_free(doc);
    free(data);
    return 0;
}
