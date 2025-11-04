#include <stdio.h>
#include <string.h>
#include "yyaml.h"

void test_yaml(const char *name, const char *yaml) {
    yyaml_err err = {0};
    printf("Testing %s:\n", name);
    printf("YAML: '%s'\n", yaml);
    yyaml_doc *doc = yyaml_read(yaml, strlen(yaml), NULL, &err);
    if (doc) {
        printf("  SUCCESS\n");
        const yyaml_node *root = yyaml_doc_get_root(doc);
        if (root) {
            printf("  Root type: %d\n", root->type);
            if (root->type == YYAML_MAPPING) {
                printf("  Mapping with %zu items\n", yyaml_map_len(root));
            } else if (root->type == YYAML_SEQUENCE) {
                printf("  Sequence with %zu items\n", yyaml_seq_len(root));
            }
        }
        yyaml_doc_free(doc);
    } else {
        printf("  FAILED: %s (pos: %zu)\n", err.msg, err.pos);
    }
    printf("\n");
}

int main() {
    // Test the specific YAML that's failing in our tests
    test_yaml("Sequence test", "items:\n  - item1\n  - item2\n  - 42");
    test_yaml("Invalid YAML test", "invalid\tcontent");
    test_yaml("Write sequence test", "items:\n  - a\n  - b\n  - c");
    return 0;
}
