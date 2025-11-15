/*
 * Sample 01 - Load the app_config.yaml file and print each field in a
 * human-readable way. The goal is to demonstrate how to navigate mappings
 * and sequences after parsing a YAML document with yyaml.
 */

#include <errno.h>
#include <stdint.h>
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

    if (!yyaml_example_build_data_path(__FILE__, "/../data/app_config.yaml", path,
                                       sizeof(path))) {
        fprintf(stderr, "Failed to resolve the app_config.yaml path relative to %s\n",
                __FILE__);
        return EXIT_FAILURE;
    }

    printf("Reading configuration from: %s\n", path);

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
        char name[128];
        copy_scalar_string(doc, yyaml_map_get(root, "name"), name, sizeof(name));
        printf("data[\"name\"]: %s\n", name[0] ? name : "<missing>");
    }

    {
        const yyaml_node *version = yyaml_map_get(root, "version");
        if (version && version->type == YYAML_INT) {
            printf("data[\"version\"]: %lld\n", (long long)version->val.integer);
        } else if (version && version->type == YYAML_DOUBLE) {
            printf("data[\"version\"]: %.2f\n", version->val.real);
        } else {
            puts("data[\"version\"]: <missing>");
        }
    }

    {
        const yyaml_node *debug = yyaml_map_get(root, "debug");
        if (debug && debug->type == YYAML_BOOL) {
            printf("data[\"debug\"]: %s\n", debug->val.boolean ? "true" : "false");
        } else {
            puts("data[\"debug\"]: <missing>");
        }
    }

    {
        const yyaml_node *features = yyaml_map_get(root, "features");
        char feature0[128];
        char feature1[128];
        char feature2[128];
        copy_scalar_string(doc,
                           (features && features->type == YYAML_SEQUENCE)
                               ? yyaml_seq_get(features, 0)
                               : NULL,
                           feature0, sizeof(feature0));
        copy_scalar_string(doc,
                           (features && features->type == YYAML_SEQUENCE)
                               ? yyaml_seq_get(features, 1)
                               : NULL,
                           feature1, sizeof(feature1));
        copy_scalar_string(doc,
                           (features && features->type == YYAML_SEQUENCE)
                               ? yyaml_seq_get(features, 2)
                               : NULL,
                           feature2, sizeof(feature2));
        printf("data[\"features\"][0]: %s\n", feature0[0] ? feature0 : "<missing>");
        printf("data[\"features\"][1]: %s\n", feature1[0] ? feature1 : "<missing>");
        printf("data[\"features\"][2]: %s\n", feature2[0] ? feature2 : "<missing>");
    }

    {
        const yyaml_node *database = yyaml_map_get(root, "database");
        char host[128];
        const yyaml_node *port;
        const yyaml_node *tags;
        char tag0[128];
        char tag1[128];

        if (!database || database->type != YYAML_MAPPING) {
            puts("data[\"database\"]: <missing mapping>");
        } else {
            copy_scalar_string(doc, yyaml_map_get(database, "host"), host, sizeof(host));
            printf("data[\"database\"][\"host\"]: %s\n", host[0] ? host : "<missing>");

            port = yyaml_map_get(database, "port");
            if (port && port->type == YYAML_INT) {
                printf("data[\"database\"][\"port\"]: %lld\n", (long long)port->val.integer);
            } else if (port && port->type == YYAML_DOUBLE) {
                printf("data[\"database\"][\"port\"]: %.2f\n", port->val.real);
            } else {
                puts("data[\"database\"][\"port\"]: <missing>");
            }

            tags = yyaml_map_get(database, "tags");
            copy_scalar_string(doc,
                               (tags && tags->type == YYAML_SEQUENCE)
                                   ? yyaml_seq_get(tags, 0)
                                   : NULL,
                               tag0, sizeof(tag0));
            copy_scalar_string(doc,
                               (tags && tags->type == YYAML_SEQUENCE)
                                   ? yyaml_seq_get(tags, 1)
                                   : NULL,
                               tag1, sizeof(tag1));
            printf("data[\"database\"][\"tags\"][0]: %s\n", tag0[0] ? tag0 : "<missing>");
            printf("data[\"database\"][\"tags\"][1]: %s\n", tag1[0] ? tag1 : "<missing>");
        }
    }

    yyaml_doc_free(doc);
    free(data);
    return EXIT_SUCCESS;
}
