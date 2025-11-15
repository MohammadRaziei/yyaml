/*
 * Sample 03 - Generate a complex YAML payload, serialize it to a temporary
 * file, reload it, and verify that the round-trip preserved the expected
 * values. This demonstrates how to combine yyaml_read with yyaml_write while
 * manually inspecting every element.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "yyaml.h"

static const char *k_input_yaml =
    "deployment:\n"
    "  name: core-services\n"
    "  enabled: true\n"
    "  replicas: 3\n"
    "  containers:\n"
    "    - name: api\n"
    "      image: registry.example.com/api:v1\n"
    "      env:\n"
    "        DEBUG: yes\n"
    "        TIMEOUT: 30\n"
    "    - name: worker\n"
    "      image: registry.example.com/worker:v2\n"
    "      env:\n"
    "        DEBUG: no\n"
    "        TIMEOUT: 120\n"
    "  volumes:\n"
    "    config:\n"
    "      mountPath: /etc/config\n"
    "      readOnly: true\n";

static void copy_scalar_string(const yyaml_doc *doc,
                               const yyaml_node *node,
                               char *buffer,
                               size_t buffer_size) {
    const char *scalars;
    size_t len;
    if (!buffer || buffer_size == 0) return;
    buffer[0] = '\0';
    if (!node) return;

    switch (node->type) {
    case YYAML_STRING:
        scalars = yyaml_doc_get_scalar_buf(doc);
        if (!scalars) return;
        len = node->val.str.len;
        if (len >= buffer_size) len = buffer_size - 1;
        memcpy(buffer, scalars + node->val.str.ofs, len);
        buffer[len] = '\0';
        return;
    case YYAML_BOOL:
        strncpy(buffer, node->val.boolean ? "true" : "false", buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return;
    case YYAML_INT:
        snprintf(buffer, buffer_size, "%lld", (long long)node->val.integer);
        return;
    default:
        return;
    }
}

static const yyaml_node *get_sequence_item(const yyaml_doc *doc,
                                           const yyaml_node *sequence,
                                           size_t index) {
    if (!sequence || sequence->type != YYAML_SEQUENCE) {
        return NULL;
    }
    return yyaml_seq_get(sequence, index);
}

static void describe_deployment(const yyaml_doc *doc, const char *label) {
    const yyaml_node *root = yyaml_doc_get_root(doc);
    const yyaml_node *deployment =
        (root && root->type == YYAML_MAPPING) ? yyaml_map_get(root, "deployment") : NULL;
    char name[128];
    const yyaml_node *enabled;
    const yyaml_node *replicas;
    const yyaml_node *containers;
    const yyaml_node *volumes;

    if (!deployment || deployment->type != YYAML_MAPPING) {
        printf("%s deployment: <missing mapping>\n", label);
        return;
    }

    copy_scalar_string(doc, yyaml_map_get(deployment, "name"), name, sizeof(name));
    enabled = yyaml_map_get(deployment, "enabled");
    replicas = yyaml_map_get(deployment, "replicas");
    containers = yyaml_map_get(deployment, "containers");
    volumes = yyaml_map_get(deployment, "volumes");

    printf("%s deployment[\"name\"]: %s\n", label, name[0] ? name : "<missing>");
    if (enabled && enabled->type == YYAML_BOOL) {
        printf("%s deployment[\"enabled\"]: %s\n", label,
               enabled->val.boolean ? "true" : "false");
    } else {
        printf("%s deployment[\"enabled\"]: <missing>\n", label);
    }
    if (replicas && replicas->type == YYAML_INT) {
        printf("%s deployment[\"replicas\"]: %lld\n", label, (long long)replicas->val.integer);
    } else {
        printf("%s deployment[\"replicas\"]: <missing>\n", label);
    }

    {
        const yyaml_node *first = get_sequence_item(doc, containers, 0);
        const yyaml_node *second = get_sequence_item(doc, containers, 1);
        char first_name[64];
        char first_image[128];
        char second_name[64];
        char second_image[128];
        const yyaml_node *first_env = first && first->type == YYAML_MAPPING
                                          ? yyaml_map_get(first, "env")
                                          : NULL;
        const yyaml_node *second_env = second && second->type == YYAML_MAPPING
                                           ? yyaml_map_get(second, "env")
                                           : NULL;
        char first_debug[32];
        char first_timeout[32];
        char second_debug[32];
        char second_timeout[32];

        if (!first || first->type != YYAML_MAPPING) {
            printf("%s deployment[\"containers\"][0]: <missing mapping>\n", label);
        } else {
            copy_scalar_string(doc, yyaml_map_get(first, "name"), first_name, sizeof(first_name));
            copy_scalar_string(doc, yyaml_map_get(first, "image"), first_image,
                               sizeof(first_image));
            copy_scalar_string(doc,
                               (first_env && first_env->type == YYAML_MAPPING)
                                   ? yyaml_map_get(first_env, "DEBUG")
                                   : NULL,
                               first_debug, sizeof(first_debug));
            copy_scalar_string(doc,
                               (first_env && first_env->type == YYAML_MAPPING)
                                   ? yyaml_map_get(first_env, "TIMEOUT")
                                   : NULL,
                               first_timeout, sizeof(first_timeout));
            printf("%s deployment[\"containers\"][0][\"name\"]: %s\n", label,
                   first_name[0] ? first_name : "<missing>");
            printf("%s deployment[\"containers\"][0][\"image\"]: %s\n", label,
                   first_image[0] ? first_image : "<missing>");
            printf("%s deployment[\"containers\"][0][\"env\"][\"DEBUG\"]: %s\n", label,
                   first_debug[0] ? first_debug : "<missing>");
            printf("%s deployment[\"containers\"][0][\"env\"][\"TIMEOUT\"]: %s\n", label,
                   first_timeout[0] ? first_timeout : "<missing>");
        }

        if (!second || second->type != YYAML_MAPPING) {
            printf("%s deployment[\"containers\"][1]: <missing mapping>\n", label);
        } else {
            copy_scalar_string(doc, yyaml_map_get(second, "name"), second_name,
                               sizeof(second_name));
            copy_scalar_string(doc, yyaml_map_get(second, "image"), second_image,
                               sizeof(second_image));
            copy_scalar_string(doc,
                               (second_env && second_env->type == YYAML_MAPPING)
                                   ? yyaml_map_get(second_env, "DEBUG")
                                   : NULL,
                               second_debug, sizeof(second_debug));
            copy_scalar_string(doc,
                               (second_env && second_env->type == YYAML_MAPPING)
                                   ? yyaml_map_get(second_env, "TIMEOUT")
                                   : NULL,
                               second_timeout, sizeof(second_timeout));
            printf("%s deployment[\"containers\"][1][\"name\"]: %s\n", label,
                   second_name[0] ? second_name : "<missing>");
            printf("%s deployment[\"containers\"][1][\"image\"]: %s\n", label,
                   second_image[0] ? second_image : "<missing>");
            printf("%s deployment[\"containers\"][1][\"env\"][\"DEBUG\"]: %s\n", label,
                   second_debug[0] ? second_debug : "<missing>");
            printf("%s deployment[\"containers\"][1][\"env\"][\"TIMEOUT\"]: %s\n", label,
                   second_timeout[0] ? second_timeout : "<missing>");
        }
    }

    {
        const yyaml_node *config =
            (volumes && volumes->type == YYAML_MAPPING) ? yyaml_map_get(volumes, "config")
                                                        : NULL;
        const yyaml_node *mount_path =
            (config && config->type == YYAML_MAPPING) ? yyaml_map_get(config, "mountPath")
                                                      : NULL;
        const yyaml_node *read_only =
            (config && config->type == YYAML_MAPPING) ? yyaml_map_get(config, "readOnly")
                                                      : NULL;
        char mount_value[128];

        copy_scalar_string(doc, mount_path, mount_value, sizeof(mount_value));
        printf("%s deployment[\"volumes\"][\"config\"][\"mountPath\"]: %s\n", label,
               mount_value[0] ? mount_value : "<missing>");
        if (read_only && read_only->type == YYAML_BOOL) {
            printf("%s deployment[\"volumes\"][\"config\"][\"readOnly\"]: %s\n", label,
                   read_only->val.boolean ? "true" : "false");
        } else {
            printf("%s deployment[\"volumes\"][\"config\"][\"readOnly\"]: <missing>\n", label);
        }
    }
}

static void print_check(const char *label, bool ok) {
    printf("%s %s\n", label, ok ? "[OK]" : "[MISMATCH]");
}

static void verify_expected(const yyaml_doc *doc) {
    const yyaml_node *root = yyaml_doc_get_root(doc);
    const yyaml_node *deployment =
        (root && root->type == YYAML_MAPPING) ? yyaml_map_get(root, "deployment") : NULL;
    const yyaml_node *containers;
    const yyaml_node *volumes;
    char text[128];
    const yyaml_node *node;

    if (!deployment || deployment->type != YYAML_MAPPING) {
        print_check("deployment mapping present", false);
        return;
    }

    copy_scalar_string(doc, yyaml_map_get(deployment, "name"), text, sizeof(text));
    print_check("deployment[\"name\"] == core-services", strcmp(text, "core-services") == 0);

    node = yyaml_map_get(deployment, "enabled");
    print_check("deployment[\"enabled\"] == true",
                node && node->type == YYAML_BOOL && node->val.boolean);

    node = yyaml_map_get(deployment, "replicas");
    print_check("deployment[\"replicas\"] == 3",
                node && node->type == YYAML_INT && node->val.integer == 3);

    containers = yyaml_map_get(deployment, "containers");
    {
        const yyaml_node *first = get_sequence_item(doc, containers, 0);
        const yyaml_node *second = get_sequence_item(doc, containers, 1);
        const yyaml_node *env;

        copy_scalar_string(doc,
                           (first && first->type == YYAML_MAPPING)
                               ? yyaml_map_get(first, "name")
                               : NULL,
                           text, sizeof(text));
        print_check("containers[0].name == api", strcmp(text, "api") == 0);
        copy_scalar_string(doc,
                           (first && first->type == YYAML_MAPPING)
                               ? yyaml_map_get(first, "image")
                               : NULL,
                           text, sizeof(text));
        print_check("containers[0].image == registry.example.com/api:v1",
                    strcmp(text, "registry.example.com/api:v1") == 0);
        env = (first && first->type == YYAML_MAPPING) ? yyaml_map_get(first, "env") : NULL;
        node = (env && env->type == YYAML_MAPPING) ? yyaml_map_get(env, "DEBUG") : NULL;
        print_check("containers[0].env.DEBUG == true",
                    node && node->type == YYAML_BOOL && node->val.boolean);
        node = (env && env->type == YYAML_MAPPING) ? yyaml_map_get(env, "TIMEOUT") : NULL;
        print_check("containers[0].env.TIMEOUT == 30",
                    node && node->type == YYAML_INT && node->val.integer == 30);

        copy_scalar_string(doc,
                           (second && second->type == YYAML_MAPPING)
                               ? yyaml_map_get(second, "name")
                               : NULL,
                           text, sizeof(text));
        print_check("containers[1].name == worker", strcmp(text, "worker") == 0);
        copy_scalar_string(doc,
                           (second && second->type == YYAML_MAPPING)
                               ? yyaml_map_get(second, "image")
                               : NULL,
                           text, sizeof(text));
        print_check("containers[1].image == registry.example.com/worker:v2",
                    strcmp(text, "registry.example.com/worker:v2") == 0);
        env = (second && second->type == YYAML_MAPPING) ? yyaml_map_get(second, "env") : NULL;
        node = (env && env->type == YYAML_MAPPING) ? yyaml_map_get(env, "DEBUG") : NULL;
        print_check("containers[1].env.DEBUG == false",
                    node && node->type == YYAML_BOOL && !node->val.boolean);
        node = (env && env->type == YYAML_MAPPING) ? yyaml_map_get(env, "TIMEOUT") : NULL;
        print_check("containers[1].env.TIMEOUT == 120",
                    node && node->type == YYAML_INT && node->val.integer == 120);
    }

    volumes = yyaml_map_get(deployment, "volumes");
    {
        const yyaml_node *config =
            (volumes && volumes->type == YYAML_MAPPING) ? yyaml_map_get(volumes, "config")
                                                        : NULL;
        const yyaml_node *mount_path =
            (config && config->type == YYAML_MAPPING) ? yyaml_map_get(config, "mountPath")
                                                      : NULL;
        const yyaml_node *read_only =
            (config && config->type == YYAML_MAPPING) ? yyaml_map_get(config, "readOnly")
                                                      : NULL;
        copy_scalar_string(doc, mount_path, text, sizeof(text));
        print_check("volumes.config.mountPath == /etc/config", strcmp(text, "/etc/config") == 0);
        print_check("volumes.config.readOnly == true",
                    read_only && read_only->type == YYAML_BOOL && read_only->val.boolean);
    }
}

int main(void) {
    yyaml_err err = {0};
    yyaml_doc *doc = NULL;
    char *serialized = NULL;
    size_t serialized_len = 0;
    char temp_path[4096];
    bool ok;

    doc = yyaml_read(k_input_yaml, strlen(k_input_yaml), NULL, &err);
    if (!doc) {
        fprintf(stderr, "Failed to parse in-memory YAML: %s (line %zu, column %zu)\n", err.msg,
                err.line, err.column);
        return EXIT_FAILURE;
    }

    puts("--- Parsed deployment specification ---");
    describe_deployment(doc, "parsed");

    const yyaml_node *root = yyaml_doc_get_root(doc);
    ok = yyaml_write(root, &serialized, &serialized_len, NULL, &err);
    if (!ok) {
        fprintf(stderr, "Failed to serialize YAML: %s\n", err.msg);
        yyaml_doc_free(doc);
        return EXIT_FAILURE;
    }

    if (!yyaml_example_create_temp_yaml(temp_path, sizeof(temp_path))) {
        fprintf(stderr, "Failed to create a temporary file for serialization.\n");
        yyaml_free_string(serialized);
        yyaml_doc_free(doc);
        return EXIT_FAILURE;
    }

    printf("Writing serialized YAML to %s (%zu bytes)\n", temp_path, serialized_len);
    if (!yyaml_example_write_file(temp_path, serialized, serialized_len)) {
        fprintf(stderr, "Unable to write serialized YAML to %s: %s\n", temp_path,
                strerror(errno));
        yyaml_free_string(serialized);
        yyaml_doc_free(doc);
        return EXIT_FAILURE;
    }

    yyaml_doc_free(doc);
    doc = NULL;

    {
        char *file_data = NULL;
        size_t file_len = 0;
        yyaml_doc *roundtrip_doc;

        if (!yyaml_example_read_file(temp_path, &file_data, &file_len)) {
            fprintf(stderr, "Unable to reopen %s: %s\n", temp_path, strerror(errno));
            yyaml_free_string(serialized);
            return EXIT_FAILURE;
        }

        roundtrip_doc = yyaml_read(file_data, file_len, NULL, &err);
        if (!roundtrip_doc) {
            fprintf(stderr, "Failed to parse serialized YAML: %s (line %zu, column %zu)\n", err.msg,
                    err.line, err.column);
            free(file_data);
            yyaml_free_string(serialized);
            return EXIT_FAILURE;
        }

        puts("--- Round-trip verification ---");
        describe_deployment(roundtrip_doc, "roundtrip");
        verify_expected(roundtrip_doc);

        yyaml_doc_free(roundtrip_doc);
        free(file_data);
    }

    yyaml_free_string(serialized);

    puts("Round-trip completed successfully.");
    printf("Temporary YAML preserved at: %s\n", temp_path);

    return EXIT_SUCCESS;
}
