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

static void format_value(const yyaml_doc *doc, const yyaml_node *node,
                         char *buf, size_t buf_size, const char *missing) {
    const char *scalars;
    if (!buf || buf_size == 0) return;
    if (!node) {
        snprintf(buf, buf_size, "%s", missing);
        return;
    }

    scalars = yyaml_doc_get_scalar_buf(doc);
    switch (node->type) {
    case YYAML_NULL:
        snprintf(buf, buf_size, "null");
        break;
    case YYAML_BOOL:
        snprintf(buf, buf_size, "%s", node->val.boolean ? "true" : "false");
        break;
    case YYAML_INT:
        snprintf(buf, buf_size, "%lld", (long long)node->val.integer);
        break;
    case YYAML_DOUBLE:
        snprintf(buf, buf_size, "%g", node->val.real);
        break;
    case YYAML_STRING:
        if (scalars && node->val.str.len > 0) {
            size_t copy_len = node->val.str.len;
            if (copy_len >= buf_size) {
                copy_len = buf_size - 1;
            }
            memcpy(buf, scalars + node->val.str.ofs, copy_len);
            buf[copy_len] = '\0';
        } else {
            buf[0] = '\0';
        }
        break;
    default:
        buf[0] = '\0';
        break;
    }
}

static void print_item_summary(const yyaml_doc *doc, const yyaml_node *item) {
    char id_buf[64];
    char name_buf[128];
    char qty_buf[64];

    if (!item || item->type != YYAML_MAPPING) return;

    format_value(doc, yyaml_map_get(doc, item, "id"), id_buf, sizeof(id_buf), "?");
    format_value(doc, yyaml_map_get(doc, item, "name"), name_buf, sizeof(name_buf), "<unnamed>");
    format_value(doc, yyaml_map_get(doc, item, "quantity"), qty_buf, sizeof(qty_buf), "0");

    printf("- Item %s: %s (qty: %s)\n", id_buf, name_buf, qty_buf);
}

int main(int argc, char **argv) {
    char default_path[1024];
    const char *path;

    if (argc > 1) {
        path = argv[1];
    } else {
        if (!yyaml_example_build_data_path(__FILE__, "/../data/inventory.yml",
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

    const yyaml_node *root = yyaml_doc_get_root(doc);
    if (!root || root->type != YYAML_MAPPING) {
        fprintf(stderr, "Expected a mapping at the root of %s\n", path);
        yyaml_doc_free(doc);
        free(data);
        return 1;
    }

    {
        char city_buf[128];
        char temp_buf[64];
        char active_buf[16];
        const yyaml_node *warehouse = yyaml_map_get(doc, root, "warehouse");
        if (warehouse && warehouse->type == YYAML_MAPPING) {
            format_value(doc, yyaml_map_get(doc, warehouse, "city"),
                         city_buf, sizeof(city_buf), "unknown");
            format_value(doc, yyaml_map_get(doc, warehouse, "temperature"),
                         temp_buf, sizeof(temp_buf), "?");
            format_value(doc, yyaml_map_get(doc, warehouse, "active"),
                         active_buf, sizeof(active_buf), "?");
            printf("Warehouse %s | temperature: %s | active: %s\n",
                   city_buf, temp_buf, active_buf);
        }
    }

    {
        const yyaml_node *items = yyaml_map_get(doc, root, "items");
        if (items && items->type == YYAML_SEQUENCE) {
            size_t count = yyaml_seq_len(items);
            size_t i;
            printf("Total items: %zu\n", count);
            for (i = 0; i < count; i++) {
                const yyaml_node *entry = yyaml_seq_get(doc, items, i);
                print_item_summary(doc, entry);
            }
        }
    }

    yyaml_doc_free(doc);
    free(data);
    return 0;
}
