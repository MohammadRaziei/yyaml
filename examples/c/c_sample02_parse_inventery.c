/*
 * Sample 02 - Parse the inventory.yml document and enumerate its content.
 * This example focuses on sequences inside mappings and illustrates how to
 * turn the parsed tree into a human-friendly report without relying on
 * command-line arguments.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "yyaml.h"

static void copy_scalar_string(const yyaml_doc *doc,
                               const yyaml_node *node,
                               char *buffer,
                               size_t buffer_size) {
    const char *scalars;
    size_t len;
    if (!buffer || buffer_size == 0) return;
    buffer[0] = '\0';
    if (!node || node->type != YYAML_STRING) return;
    scalars = yyaml_doc_get_scalar_buf(doc);
    if (!scalars) return;
    len = node->val.str.len;
    if (len >= buffer_size) len = buffer_size - 1;
    memcpy(buffer, scalars + node->val.str.ofs, len);
    buffer[len] = '\0';
}

int main(void) {
    char path[1024];
    char *data = NULL;
    size_t len = 0;
    yyaml_err err = {0};
    yyaml_doc *doc = NULL;
    const yyaml_node *root;

    if (!yyaml_example_build_data_path(__FILE__, "/../data/inventory.yml", path,
                                       sizeof(path))) {
        fprintf(stderr, "Failed to resolve the inventory.yml path relative to %s\n",
                __FILE__);
        return EXIT_FAILURE;
    }

    printf("Reading inventory from: %s\n", path);

    if (!yyaml_example_read_file(path, &data, &len)) {
        fprintf(stderr, "Failed to read %s: %s\n", path, strerror(errno));
        return EXIT_FAILURE;
    }

    doc = yyaml_read(data, len, NULL, &err);
    if (!doc) {
        fprintf(stderr, "Failed to parse %s: %s (line %zu, column %zu)\n",
                path, err.msg, err.line, err.column);
        free(data);
        return EXIT_FAILURE;
    }

    root = yyaml_doc_get_root(doc);
    if (!root || root->type != YYAML_MAPPING) {
        fprintf(stderr, "Unexpected root type in %s. Expected a mapping.\n", path);
        yyaml_doc_free(doc);
        free(data);
        return EXIT_FAILURE;
    }

    {
        const yyaml_node *warehouse = yyaml_map_get(root, "warehouse");
        char city[128];
        const yyaml_node *temperature;
        const yyaml_node *active;

        if (!warehouse || warehouse->type != YYAML_MAPPING) {
            puts("data[\"warehouse\"]: <missing mapping>");
        } else {
            copy_scalar_string(doc, yyaml_map_get(warehouse, "city"), city, sizeof(city));
            printf("data[\"warehouse\"][\"city\"]: %s\n", city[0] ? city : "<missing>");

            temperature = yyaml_map_get(warehouse, "temperature");
            if (temperature && temperature->type == YYAML_DOUBLE) {
                printf("data[\"warehouse\"][\"temperature\"]: %.1f\n", temperature->val.real);
            } else if (temperature && temperature->type == YYAML_INT) {
                printf("data[\"warehouse\"][\"temperature\"]: %lld\n", (long long)temperature->val.integer);
            } else {
                puts("data[\"warehouse\"][\"temperature\"]: <missing>");
            }

            active = yyaml_map_get(warehouse, "active");
            if (active && active->type == YYAML_BOOL) {
                printf("data[\"warehouse\"][\"active\"]: %s\n",
                       active->val.boolean ? "true" : "false");
            } else {
                puts("data[\"warehouse\"][\"active\"]: <missing>");
            }
        }
    }

    {
        const yyaml_node *items = yyaml_map_get(root, "items");
        const yyaml_node *item0 =
            (items && items->type == YYAML_SEQUENCE) ? yyaml_seq_get(items, 0) : NULL;
        const yyaml_node *item1 =
            (items && items->type == YYAML_SEQUENCE) ? yyaml_seq_get(items, 1) : NULL;
        char id0[32];
        char id1[32];
        char name0[128];
        char name1[128];
        const yyaml_node *quantity0;
        const yyaml_node *quantity1;

        if (!item0 || item0->type != YYAML_MAPPING) {
            puts("data[\"items\"][0]: <missing mapping>");
        } else {
            copy_scalar_string(doc, yyaml_map_get(item0, "id"), id0, sizeof(id0));
            copy_scalar_string(doc, yyaml_map_get(item0, "name"), name0, sizeof(name0));
            quantity0 = yyaml_map_get(item0, "quantity");
            printf("data[\"items\"][0][\"id\"]: %s\n", id0[0] ? id0 : "<missing>");
            printf("data[\"items\"][0][\"name\"]: %s\n", name0[0] ? name0 : "<missing>");
            if (quantity0 && quantity0->type == YYAML_INT) {
                printf("data[\"items\"][0][\"quantity\"]: %lld\n",
                       (long long)quantity0->val.integer);
            } else if (quantity0 && quantity0->type == YYAML_DOUBLE) {
                printf("data[\"items\"][0][\"quantity\"]: %.2f\n", quantity0->val.real);
            } else {
                puts("data[\"items\"][0][\"quantity\"]: <missing>");
            }
        }

        if (!item1 || item1->type != YYAML_MAPPING) {
            puts("data[\"items\"][1]: <missing mapping>");
        } else {
            copy_scalar_string(doc, yyaml_map_get(item1, "id"), id1, sizeof(id1));
            copy_scalar_string(doc, yyaml_map_get(item1, "name"), name1, sizeof(name1));
            quantity1 = yyaml_map_get(item1, "quantity");
            printf("data[\"items\"][1][\"id\"]: %s\n", id1[0] ? id1 : "<missing>");
            printf("data[\"items\"][1][\"name\"]: %s\n", name1[0] ? name1 : "<missing>");
            if (quantity1 && quantity1->type == YYAML_INT) {
                printf("data[\"items\"][1][\"quantity\"]: %lld\n",
                       (long long)quantity1->val.integer);
            } else if (quantity1 && quantity1->type == YYAML_DOUBLE) {
                printf("data[\"items\"][1][\"quantity\"]: %.2f\n", quantity1->val.real);
            } else {
                puts("data[\"items\"][1][\"quantity\"]: <missing>");
            }
        }
    }

    yyaml_doc_free(doc);
    free(data);
    return EXIT_SUCCESS;
}
