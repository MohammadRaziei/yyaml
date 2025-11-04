#include <stdio.h>
#include <string.h>
#include "yyaml.h"

int main() {
    const char *tests[] = {
        "42",
        "-123", 
        "hello",
        "- item",
        "- a\n- b",
        "key: value",
        NULL
    };
    
    for (int i = 0; tests[i]; i++) {
        yyaml_err err = {0};
        yyaml_doc *doc = yyaml_read(tests[i], strlen(tests[i]), NULL, &err);
        printf("Test %d: '%s' -> ", i, tests[i]);
        if (doc) {
            const yyaml_node *root = yyaml_doc_get_root(doc);
            if (root) {
                printf("SUCCESS (type: %d)\n", root->type);
            } else {
                printf("SUCCESS (no root)\n");
            }
            yyaml_doc_free(doc);
        } else {
            printf("FAILED: %s (pos: %zu)\n", err.msg, err.pos);
        }
    }
    return 0;
}
