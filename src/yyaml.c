#include "yyaml.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <math.h>

struct yyaml_doc {
    yyaml_node *root;           /**< Root node of the document */
    yyaml_node *first_node;     /**< First node in the linked list */
    yyaml_node *last_node;      /**< Last node in the linked list */
    size_t node_count;          /**< Total number of nodes */
    char *scalars;              /**< Shared scalar buffer */
    size_t scalar_len;          /**< Length of used scalar buffer */
    size_t scalar_cap;          /**< Capacity of scalar buffer */
};

/* ------------------------------ utilities -------------------------------- */

static void yyaml_set_error(yyaml_err *err, size_t pos, size_t line,
                            size_t col, const char *msg) {
    if (!err) return;
    err->pos = pos;
    err->line = line;
    err->column = col;
    snprintf(err->msg, sizeof(err->msg), "%s", msg ? msg : "parse error");
}

static size_t yyaml_next_capacity(size_t current, size_t need, size_t init) {
    size_t cap = current ? current : init;
    if (cap < init) cap = init;
    while (cap < need) {
        size_t grown = (cap << 1); /* 2x growth */
        if (grown <= cap) return need;   /* overflow guard */
        cap = grown;
    }
    return cap;
}

static bool yyaml_doc_reserve_str(yyaml_doc *doc, size_t need) {
    size_t cap;
    char *new_buf;
    if (doc->scalar_cap >= need) return true;
    cap = yyaml_next_capacity(doc->scalar_cap, need, YYAML_STR_CAP_INIT);
    new_buf = (char *)realloc(doc->scalars, cap);
    if (!new_buf) return false;
    doc->scalars = new_buf;
    doc->scalar_cap = cap;
    return true;
}

static yyaml_node *yyaml_doc_add_node(yyaml_doc *doc, yyaml_type type) {
    yyaml_node *node = (yyaml_node *)calloc(1, sizeof(yyaml_node));
    if (!node) return NULL;
    
    node->doc = doc;
    node->type = type;
    node->parent = NULL;
    node->next = NULL;
    node->child = NULL;
    node->extra = 0;
    
    // Initialize node values based on type
    switch (type) {
        case YYAML_BOOL:
            node->val.boolean = false;
            break;
        case YYAML_INT:
            node->val.integer = 0;
            break;
        case YYAML_DOUBLE:
            node->val.real = 0.0;
            break;
        case YYAML_STRING:
            node->val.str.ofs = 0;
            node->val.str.len = 0;
            break;
        default:
            break;
    }
    
    // Add to linked list
    if (!doc->first_node) {
        doc->first_node = node;
        doc->last_node = node;
    } else {
        doc->last_node->next = node;
        doc->last_node = node;
    }
    
    doc->node_count++;
    return node;
}

static bool yyaml_doc_store_string(yyaml_doc *doc, const char *str, size_t len,
                                   uint32_t *out_ofs) {
    size_t need = doc->scalar_len + len + 1;
    if (!yyaml_doc_reserve_str(doc, need)) return false;
    memcpy(doc->scalars + doc->scalar_len, str, len);
    if (out_ofs) *out_ofs = (uint32_t)doc->scalar_len;
    doc->scalar_len += len;
    doc->scalars[doc->scalar_len++] = '\0';
    return true;
}

/* ---------------------------- reading API -------------------------------- */

static const yyaml_read_opts yyaml_default_opts = {false, false, true, 64};

static bool yyaml_is_num_char(int c) {
    return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' ||
           c == 'e' || c == 'E';
}

static bool yyaml_ieq(const char *str, size_t len, const char *lit) {
    size_t i;
    if (!str || !lit) return false;
    for (i = 0; lit[i]; i++) {
        if (i >= len) return false;
        if (tolower((unsigned char)str[i]) != tolower((unsigned char)lit[i]))
            return false;
    }
    return i == len;
}

static bool yyaml_parse_bool(const char *str, size_t len, bool *out) {
    if (yyaml_ieq(str, len, "true") || yyaml_ieq(str, len, "yes") ||
        yyaml_ieq(str, len, "on")) {
        *out = true;
        return true;
    }
    if (yyaml_ieq(str, len, "false") || yyaml_ieq(str, len, "no") ||
        yyaml_ieq(str, len, "off")) {
        *out = false;
        return true;
    }
    return false;
}

static bool yyaml_parse_null(const char *str, size_t len) {
    if (yyaml_ieq(str, len, "null")) return true;
    if (len == 1 && (str[0] == '~' || yyaml_ieq(str, len, "n"))) return true;
    return false;
}

static bool yyaml_parse_inf_nan(const char *str, size_t len, double *out,
                                bool allow_inf_nan) {
    if (!allow_inf_nan) return false;
    bool negative = false;
    if (len > 0 && (str[0] == '-' || str[0] == '+')) {
        negative = (str[0] == '-');
        str++;
        len--;
    }
    if (len > 0 && str[0] == '.') {
        str++;
        len--;
    }
    if (yyaml_ieq(str, len, "nan")) {
        *out = NAN;
        return true;
    }
    if (yyaml_ieq(str, len, "inf")) {
        *out = negative ? -INFINITY : INFINITY;
        return true;
    }
    return false;
}

static bool yyaml_parse_scalar(const char *str, size_t len, yyaml_doc *doc,
                               yyaml_node *node, const yyaml_read_opts *opts,
                               yyaml_err *err, size_t pos, size_t line,
                               size_t col) {
    size_t i;
    bool boolean;
    double dbl;
    char *end;
    
    if (!len) {
        node->type = YYAML_NULL;
        return true;
    }
    
    // Check for quoted strings
    if ((str[0] == '"' && str[len - 1] == '"') ||
        (str[0] == '\'' && str[len - 1] == '\'')) {
        // Simple quoted string handling - just strip quotes for now
        const char *content = str + 1;
        size_t content_len = len - 2;
        uint32_t ofs;
        if (!yyaml_doc_store_string(doc, content, content_len, &ofs)) {
            yyaml_set_error(err, pos, line, col, "out of memory");
            return false;
        }
        node->type = YYAML_STRING;
        node->val.str.ofs = ofs;
        node->val.str.len = (uint32_t)content_len;
        return true;
    }
    
    // Check if it's a number candidate
    for (i = 0; i < len; i++) {
        if (!yyaml_is_num_char((unsigned char)str[i])) break;
    }
    
    if (i == len && len > 0) {
        /* number candidate */
        long long ival;
        errno = 0;
        ival = strtoll(str, &end, 10);
        if (end == str + len && errno != ERANGE) {
            node->type = YYAML_INT;
            node->val.integer = ival;
            return true;
        }
        errno = 0;
        dbl = strtod(str, &end);
        if (end == str + len && errno != ERANGE) {
            node->type = YYAML_DOUBLE;
            node->val.real = dbl;
            return true;
        }
    }
    
    // Check for inf/nan
    if (yyaml_parse_inf_nan(str, len, &dbl, opts && opts->allow_inf_nan)) {
        node->type = YYAML_DOUBLE;
        node->val.real = dbl;
        return true;
    }
    
    // Check for boolean
    if (yyaml_parse_bool(str, len, &boolean)) {
        node->type = YYAML_BOOL;
        node->val.boolean = boolean;
        return true;
    }
    
    // Check for null
    if (yyaml_parse_null(str, len)) {
        node->type = YYAML_NULL;
        return true;
    }
    
    // Default to string
    uint32_t ofs;
    if (!yyaml_doc_store_string(doc, str, len, &ofs)) {
        yyaml_set_error(err, pos, line, col, "out of memory");
        return false;
    }
    node->type = YYAML_STRING;
    node->val.str.ofs = ofs;
    node->val.str.len = (uint32_t)len;
    return true;
}

YYAML_API yyaml_doc *yyaml_read(const char *data, size_t len,
                                const yyaml_read_opts *opts,
                                yyaml_err *err) {
    const yyaml_read_opts *cfg = opts ? opts : &yyaml_default_opts;
    yyaml_doc *doc;
    size_t pos = 0, line = 1, col = 1;

    if (!data) {
        yyaml_set_error(err, 0, 1, 1, "input buffer is null");
        return NULL;
    }
    
    doc = yyaml_doc_new();
    if (!doc) return NULL;
    doc->root = NULL;

    // Skip leading whitespace
    while (pos < len && isspace((unsigned char)data[pos])) {
        if (data[pos] == '\n') { line++; col = 1; }
        else col++;
        pos++;
    }

    // If we're at the end, create a null root
    if (pos >= len) {
        yyaml_node *root = yyaml_doc_add_null(doc);
        if (!root) {
            yyaml_doc_free(doc);
            yyaml_set_error(err, pos, line, col, "out of memory");
            return NULL;
        }
        doc->root = root;
        return doc;
    }

    // Parse the first line as a scalar
    size_t line_start = pos;
    size_t line_end = pos;
    
    // Find the end of the line
    while (line_end < len && data[line_end] != '\n' && data[line_end] != '\r') {
        line_end++;
    }
    
    // Trim trailing whitespace
    size_t content_end = line_end;
    while (content_end > line_start && isspace((unsigned char)data[content_end - 1])) {
        content_end--;
    }
    
    size_t content_len = content_end - line_start;
    
    if (content_len == 0) {
        // Empty content, create null root
        yyaml_node *root = yyaml_doc_add_null(doc);
        if (!root) {
            yyaml_doc_free(doc);
            yyaml_set_error(err, pos, line, col, "out of memory");
            return NULL;
        }
        doc->root = root;
        return doc;
    }

    // Parse the scalar value
    yyaml_node temp_node = {0};
    if (!yyaml_parse_scalar(data + line_start, content_len, doc, &temp_node, cfg, 
                           err, line_start, line, col)) {
        yyaml_doc_free(doc);
        return NULL;
    }

    // Create the actual node
    yyaml_node *root = yyaml_doc_add_node(doc, temp_node.type);
    if (!root) {
        yyaml_doc_free(doc);
        yyaml_set_error(err, pos, line, col, "out of memory");
        return NULL;
    }
    
    // Copy the parsed values
    root->val = temp_node.val;
    if (temp_node.type == YYAML_STRING) {
        root->val.str.ofs = temp_node.val.str.ofs;
        root->val.str.len = temp_node.val.str.len;
    }
    
    doc->root = root;
    return doc;
}

YYAML_API yyaml_doc *yyaml_doc_new(void) {
    yyaml_doc *doc = (yyaml_doc *)calloc(1, sizeof(*doc));
    if (!doc) return NULL;
    doc->root = NULL;
    doc->first_node = NULL;
    doc->last_node = NULL;
    doc->node_count = 0;
    doc->scalars = NULL;
    doc->scalar_len = 0;
    doc->scalar_cap = 0;
    return doc;
}

YYAML_API void yyaml_doc_free(yyaml_doc *doc) {
    if (!doc) return;
    
    // Free all nodes in the linked list
    yyaml_node *current = doc->first_node;
    while (current) {
        yyaml_node *next = current->next;
        free(current);
        current = next;
    }
    
    free(doc->scalars);
    free(doc);
}

YYAML_API const yyaml_node *yyaml_doc_get_root(const yyaml_doc *doc) {
    return doc ? doc->root : NULL;
}

YYAML_API const yyaml_node *yyaml_doc_get(const yyaml_doc *doc, uint32_t idx) {
    if (!doc || idx >= doc->node_count) return NULL;
    
    // Traverse the linked list to find the node at the given index
    const yyaml_node *current = doc->first_node;
    for (uint32_t i = 0; i < idx && current; i++) {
        current = current->next;
    }
    return current;
}

YYAML_API uint32_t yyaml_node_index(const yyaml_doc *doc, const yyaml_node *node) {
    if (!doc || !node || node->doc != doc) return UINT32_MAX;
    
    // Traverse the linked list to find the index of the node
    const yyaml_node *current = doc->first_node;
    uint32_t index = 0;
    while (current) {
        if (current == node) return index;
        current = current->next;
        index++;
    }
    return UINT32_MAX;
}

YYAML_API const char *yyaml_doc_get_scalar_buf(const yyaml_doc *doc) {
    return doc ? doc->scalars : NULL;
}

YYAML_API size_t yyaml_doc_node_count(const yyaml_doc *doc) {
    return doc ? doc->node_count : 0;
}

/* --------------------------- building API ------------------------------- */

YYAML_API bool yyaml_doc_set_root(yyaml_doc *doc, yyaml_node *node) {
    if (!doc || !node) return false;
    doc->root = node;
    return true;
}

YYAML_API yyaml_node *yyaml_doc_add_null(yyaml_doc *doc) {
    if (!doc) return NULL;
    return yyaml_doc_add_node(doc, YYAML_NULL);
}

YYAML_API yyaml_node *yyaml_doc_add_bool(yyaml_doc *doc, bool value) {
    yyaml_node *node = yyaml_doc_add_node(doc, YYAML_BOOL);
    if (node) {
        node->val.boolean = value;
    }
    return node;
}

YYAML_API yyaml_node *yyaml_doc_add_int(yyaml_doc *doc, int64_t value) {
    yyaml_node *node = yyaml_doc_add_node(doc, YYAML_INT);
    if (node) {
        node->val.integer = value;
    }
    return node;
}

YYAML_API yyaml_node *yyaml_doc_add_double(yyaml_doc *doc, double value) {
    yyaml_node *node = yyaml_doc_add_node(doc, YYAML_DOUBLE);
    if (node) {
        node->val.real = value;
    }
    return node;
}

YYAML_API yyaml_node *yyaml_doc_add_string(yyaml_doc *doc, const char *str,
                                        size_t len) {
    yyaml_node *node = yyaml_doc_add_node(doc, YYAML_STRING);
    if (!node) return NULL;
    
    uint32_t ofs = 0;
    if (!yyaml_doc_store_string(doc, str, len, &ofs)) {
        // Remove the node from the linked list if string storage fails
        // For now, we'll just return NULL and let the node be orphaned
        // In a complete implementation, we'd need to remove it properly
        return NULL;
    }
    
    node->val.str.ofs = ofs;
    node->val.str.len = (uint32_t)len;
    return node;
}

YYAML_API yyaml_node *yyaml_doc_add_sequence(yyaml_doc *doc) {
    yyaml_node *node = yyaml_doc_add_node(doc, YYAML_SEQUENCE);
    if (node) {
        node->val.integer = 0; // Use integer field to store child count
    }
    return node;
}

YYAML_API yyaml_node *yyaml_doc_add_mapping(yyaml_doc *doc) {
    yyaml_node *node = yyaml_doc_add_node(doc, YYAML_MAPPING);
    if (node) {
        node->val.integer = 0; // Use integer field to store child count
    }
    return node;
}

YYAML_API bool yyaml_doc_seq_append(yyaml_doc *doc, yyaml_node *seq,
                                    yyaml_node *child) {
    if (!doc || !seq || !child || seq->type != YYAML_SEQUENCE) {
        return false;
    }
    
    // Add child to the end of the sequence's child list
    if (!seq->child) {
        seq->child = child;
    } else {
        yyaml_node *last = seq->child;
        while (last->next) {
            last = last->next;
        }
        last->next = child;
    }
    
    child->parent = seq;
    seq->val.integer++; // Increment child count
    return true;
}

YYAML_API bool yyaml_doc_map_append(yyaml_doc *doc, yyaml_node *map,
                                    const char *key, size_t key_len,
                                    yyaml_node *val) {
    if (!doc || !map || !key || !val || map->type != YYAML_MAPPING) {
        return false;
    }
    
    // Store the key string
    uint32_t key_ofs = 0;
    if (!yyaml_doc_store_string(doc, key, key_len, &key_ofs)) {
        return false;
    }
    
    // Set up the value node with key information
    val->flags = (uint32_t)key_len;
    val->extra = key_ofs;
    
    // Add value to the end of the mapping's child list
    if (!map->child) {
        map->child = val;
    } else {
        yyaml_node *last = map->child;
        while (last->next) {
            last = last->next;
        }
        last->next = val;
    }
    
    val->parent = map;
    map->val.integer++; // Increment child count
    return true;
}

/* -------------------------- convenience helpers -------------------------- */

YYAML_API const yyaml_node *yyaml_map_get(const yyaml_node *map,
                                          const char *key) {
    const yyaml_doc *doc;
    const yyaml_node *node;
    size_t key_len;
    const char *buf;
    
    if (!map || map->type != YYAML_MAPPING || !key) return NULL;
    doc = map->doc;
    if (!doc) return NULL;
    
    buf = doc->scalars;
    key_len = strlen(key);
    node = map->child;
    
    while (node) {
        if (node->flags == key_len && buf &&
            memcmp(buf + node->extra, key, key_len) == 0) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

YYAML_API const yyaml_node *yyaml_seq_get(const yyaml_node *seq,
                                          size_t index) {
    const yyaml_node *child;
    if (!seq || seq->type != YYAML_SEQUENCE) return NULL;
    
    child = seq->child;
    while (child && index--) {
        child = child->next;
    }
    return child;
}

YYAML_API size_t yyaml_seq_len(const yyaml_node *seq) {
    if (!seq || seq->type != YYAML_SEQUENCE) return 0;
    return (size_t)seq->val.integer;
}

YYAML_API size_t yyaml_map_len(const yyaml_node *map) {
    if (!map || map->type != YYAML_MAPPING) return 0;
    return (size_t)map->val.integer;
}

/* --------------------------- writing API --------------------------------- */

static bool yyaml_write_node(const yyaml_node *node, char **out, size_t *out_len,
                            const yyaml_write_opts *opts, yyaml_err *err) {
    if (!node) {
        *out = strdup("null\n");
        if (!*out) {
            yyaml_set_error(err, 0, 0, 0, "out of memory");
            return false;
        }
        if (out_len) *out_len = strlen(*out);
        return true;
    }
    
    const yyaml_doc *doc = node->doc;
    const char *scalar_buf = doc ? yyaml_doc_get_scalar_buf(doc) : NULL;
    
    switch (node->type) {
        case YYAML_NULL:
            *out = strdup("null\n");
            break;
        case YYAML_BOOL:
            *out = strdup(node->val.boolean ? "true\n" : "false\n");
            break;
        case YYAML_INT: {
            char buf[64];
            snprintf(buf, sizeof(buf), "%ld\n", node->val.integer);
            *out = strdup(buf);
            break;
        }
        case YYAML_DOUBLE: {
            char buf[64];
            snprintf(buf, sizeof(buf), "%g\n", node->val.real);
            *out = strdup(buf);
            break;
        }
        case YYAML_STRING:
            if (scalar_buf && node->val.str.len > 0) {
                *out = malloc(node->val.str.len + 2);
                if (!*out) {
                    yyaml_set_error(err, 0, 0, 0, "out of memory");
                    return false;
                }
                memcpy(*out, scalar_buf + node->val.str.ofs, node->val.str.len);
                (*out)[node->val.str.len] = '\n';
                (*out)[node->val.str.len + 1] = '\0';
                if (out_len) *out_len = node->val.str.len + 1;
                return true;
            } else {
                *out = strdup("\n");
            }
            break;
        default:
            *out = strdup("null\n");
            break;
    }
    
    if (!*out) {
        yyaml_set_error(err, 0, 0, 0, "out of memory");
        return false;
    }
    
    if (out_len) *out_len = strlen(*out);
    return true;
}

YYAML_API bool yyaml_write(const yyaml_node *root, char **out, size_t *out_len,
                           const yyaml_write_opts *opts, yyaml_err *err) {
    if (!out) {
        yyaml_set_error(err, 0, 0, 0, "invalid output buffer");
        return false;
    }
    
    return yyaml_write_node(root, out, out_len, opts, err);
}

YYAML_API void yyaml_free_string(char *str) {
    free(str);
}
