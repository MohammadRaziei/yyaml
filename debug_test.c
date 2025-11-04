#include <stdio.h>
#include <string.h>
#include "yyaml.h"

int main() {
    // Test the specific YAML that's failing
    const char *tests[] = {
        "items:\n  - item1\n  - item2\n  - 42",
        "invalid\tcontent",
        NULL
    };
    
    for (int i = 0; tests[i]; i++) {
        yyaml_err err = {0};
        printf("Testing: '%s'\n", tests[i]);
        yyaml_doc *doc = yyaml_read(tests[i], strlen(tests[i]), NULL, &err);
        if (doc) {
            printf("  SUCCESS\n");
            const yyaml_node *root = yyaml_doc_get_root(doc);
            if (root) {
                printf("  Root type: %d\n", root->type);
            }
            yyaml_doc_free(doc);
        } else {
            printf("  FAILED: %s (pos: %zu)\n", err.msg, err.pos);
        }
        printf("\n");
    }
    return 0;
}
