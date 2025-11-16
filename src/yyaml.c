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
    yyaml_node *nodes;
    size_t node_count;
    size_t node_cap;
    char *scalars;
    size_t scalar_len;
    size_t scalar_cap;
    uint32_t root;
};

#define YYAML_INDEX_NONE UINT32_MAX

typedef struct {
    size_t indent;
    uint32_t container;
    uint32_t last_child;
    bool is_sequence;
} yyaml_level;

typedef struct {
    bool active;
    uint32_t node;
    bool prefer_sequence;
} yyaml_pending;

/* ------------------------------ utilities -------------------------------- */

static void yyaml_set_error(yyaml_err *err, size_t pos, size_t line,
                            size_t col, const char *msg) {
    if (!err) return;
    err->pos = pos;
    err->line = line;
    err->column = col;
    snprintf(err->msg, sizeof(err->msg), "%s", msg ? msg : "parse error");
}

static bool yyaml_doc_reserve_nodes(yyaml_doc *doc, size_t need) {
    size_t cap;
    yyaml_node *new_nodes;
    if (doc->node_cap >= need) return true;
    cap = doc->node_cap ? doc->node_cap : YYAML_NODE_CAP_INIT;
    while (cap < need) cap *= 2;
    new_nodes = (yyaml_node *)realloc(doc->nodes, cap * sizeof(yyaml_node));
    if (!new_nodes) return false;
    doc->nodes = new_nodes;
    doc->node_cap = cap;
    return true;
}

static bool yyaml_doc_reserve_str(yyaml_doc *doc, size_t need) {
    size_t cap;
    char *new_buf;
    if (doc->scalar_cap >= need) return true;
    cap = doc->scalar_cap ? doc->scalar_cap : YYAML_STR_CAP_INIT;
    while (cap < need) cap *= 2;
    new_buf = (char *)realloc(doc->scalars, cap);
    if (!new_buf) return false;
    doc->scalars = new_buf;
    doc->scalar_cap = cap;
    return true;
}

static uint32_t yyaml_doc_add_node(yyaml_doc *doc, yyaml_type type) {
    uint32_t idx;
    if (!yyaml_doc_reserve_nodes(doc, doc->node_count + 1)) return YYAML_INDEX_NONE;
    idx = (uint32_t)doc->node_count++;
    doc->nodes[idx].doc = doc;
    doc->nodes[idx].type = type;
    doc->nodes[idx].flags = 0;
    doc->nodes[idx].parent = YYAML_INDEX_NONE;
    doc->nodes[idx].next = YYAML_INDEX_NONE;
    doc->nodes[idx].child = YYAML_INDEX_NONE;
    doc->nodes[idx].extra = 0;
    doc->nodes[idx].val.integer = 0;
    if (type == YYAML_BOOL) doc->nodes[idx].val.boolean = false;
    else if (type == YYAML_INT) doc->nodes[idx].val.integer = 0;
    else if (type == YYAML_DOUBLE) doc->nodes[idx].val.real = 0.0;
    else if (type == YYAML_STRING) {
        doc->nodes[idx].val.str.ofs = 0;
        doc->nodes[idx].val.str.len = 0;
    }
    return idx;
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

static bool yyaml_parse_scalar(const char *str, size_t len, yyaml_doc *doc,
                               yyaml_node *node, const yyaml_read_opts *opts,
                               yyaml_err *err, size_t pos, size_t line,
                               size_t col);

static void yyaml_doc_link_child(yyaml_doc *doc, yyaml_level *lvl,
                                 uint32_t child_idx) {
    yyaml_node *child = &doc->nodes[child_idx];
    yyaml_node *parent = &doc->nodes[lvl->container];
    child->parent = lvl->container;
    child->next = YYAML_INDEX_NONE;
    if (lvl->last_child == YYAML_INDEX_NONE) {
        parent->child = child_idx;
    } else {
        doc->nodes[lvl->last_child].next = child_idx;
    }
    lvl->last_child = child_idx;
    if (parent->type == YYAML_SEQUENCE || parent->type == YYAML_MAPPING) {
        parent->val.integer++;
    }
}

static bool yyaml_doc_link_last(yyaml_doc *doc, uint32_t parent_idx,
                                uint32_t child_idx) {
    yyaml_node *parent;
    yyaml_node *child;
    uint32_t last;
    if (!doc || parent_idx >= doc->node_count || child_idx >= doc->node_count) {
        return false;
    }
    parent = &doc->nodes[parent_idx];
    child = &doc->nodes[child_idx];
    child->parent = parent_idx;
    child->next = YYAML_INDEX_NONE;
    if (parent->child == YYAML_INDEX_NONE) {
        parent->child = child_idx;
        return true;
    }
    last = parent->child;
    while (doc->nodes[last].next != YYAML_INDEX_NONE) {
        last = doc->nodes[last].next;
    }
    doc->nodes[last].next = child_idx;
    return true;
}

static bool yyaml_is_flow_sequence(const char *str, size_t len,
                                   size_t *content_start,
                                   size_t *content_end) {
    size_t start = 0;
    size_t end = len;

    while (start < end && isspace((unsigned char)str[start])) start++;
    while (end > start && isspace((unsigned char)str[end - 1])) end--;

    if (end - start >= 2 && str[start] == '[' && str[end - 1] == ']') {
        if (content_start) *content_start = start + 1;
        if (content_end) *content_end = end - 1;
        return true;
    }

    return false;
}

static bool yyaml_is_anchor_only(const char *str, size_t len) {
    size_t i = 0;
    while (i < len && isspace((unsigned char)str[i])) i++;
    if (i >= len || str[i] != '&') return false;
    i++;
    while (i < len && !isspace((unsigned char)str[i])) i++;
    while (i < len && isspace((unsigned char)str[i])) i++;
    return i == len;
}

static bool yyaml_is_flow_mapping(const char *str, size_t len,
                                  size_t *content_start,
                                  size_t *content_end) {
    size_t start = 0;
    size_t end = len;

    while (start < end && isspace((unsigned char)str[start])) start++;
    while (end > start && isspace((unsigned char)str[end - 1])) end--;

    if (end - start >= 2 && str[start] == '{' && str[end - 1] == '}') {
        if (content_start) *content_start = start + 1;
        if (content_end) *content_end = end - 1;
        return true;
    }

    return false;
}

static bool yyaml_fill_flow_sequence(yyaml_doc *doc, uint32_t seq_idx,
                                     const char *data, size_t len,
                                     const yyaml_read_opts *cfg,
                                     yyaml_err *err, size_t line_start,
                                     size_t line, size_t column) {
    yyaml_level seq_level = {0};
    size_t pos = 0;

    seq_level.container = seq_idx;
    seq_level.last_child = YYAML_INDEX_NONE;
    seq_level.is_sequence = true;
    seq_level.indent = 0;

    while (pos < len) {
        size_t item_start = pos;
        size_t item_end;
        int bracket_depth = 0;
        bool in_single = false;
        bool in_double = false;

        while (pos < len) {
            char c = data[pos];
            if (c == '\'' && !in_double) {
                in_single = !in_single;
            } else if (c == '"' && !in_single) {
                in_double = !in_double;
            } else if (!in_single && !in_double) {
                if (c == '[') bracket_depth++;
                else if (c == ']') {
                    if (bracket_depth == 0) break;
                    bracket_depth--;
                } else if (c == ',' && bracket_depth == 0) {
                    break;
                }
            }
            pos++;
        }

        item_end = pos;

        while (item_start < item_end &&
               isspace((unsigned char)data[item_start])) {
            item_start++;
        }
        while (item_end > item_start &&
               isspace((unsigned char)data[item_end - 1])) {
            item_end--;
        }

        if (item_end > item_start) {
            size_t item_len = item_end - item_start;
            const char *item_ptr = data + item_start;
            uint32_t child_idx = yyaml_doc_add_node(doc, YYAML_NULL);
            if (child_idx == YYAML_INDEX_NONE) {
                yyaml_set_error(err, line_start, line, column,
                                 "out of memory");
                return false;
            }

            size_t inner_start = 0;
            size_t inner_end = 0;
            if (yyaml_is_flow_sequence(item_ptr, item_len, &inner_start,
                                       &inner_end)) {
                doc->nodes[child_idx].type = YYAML_SEQUENCE;
                if (!yyaml_fill_flow_sequence(doc, child_idx,
                                              item_ptr + inner_start,
                                              inner_end - inner_start, cfg,
                                              err, line_start, line,
                                              column)) {
                    return false;
                }
            } else {
                yyaml_node temp = {0};
                if (!yyaml_parse_scalar(item_ptr, item_len, doc, &temp, cfg,
                                        err, line_start, line, column)) {
                    return false;
                }
                doc->nodes[child_idx].type = temp.type;
                doc->nodes[child_idx].val = temp.val;
                if (temp.type == YYAML_STRING) {
                    doc->nodes[child_idx].val.str.ofs = temp.val.str.ofs;
                    doc->nodes[child_idx].val.str.len = temp.val.str.len;
                }
            }

            yyaml_doc_link_child(doc, &seq_level, child_idx);
        }

        if (pos < len && data[pos] == ',') {
            pos++;
        }
        while (pos < len && isspace((unsigned char)data[pos])) pos++;
    }

    return true;
}

static bool yyaml_fill_flow_mapping(yyaml_doc *doc, uint32_t map_idx,
                                    const char *data, size_t len,
                                    const yyaml_read_opts *cfg,
                                    yyaml_err *err, size_t line_start,
                                    size_t line, size_t column) {
    yyaml_level map_level = {0};
    size_t pos = 0;

    map_level.container = map_idx;
    map_level.last_child = YYAML_INDEX_NONE;
    map_level.is_sequence = false;
    map_level.indent = 0;

    while (pos < len) {
        size_t key_start;
        size_t key_end;
        size_t val_start;
        size_t val_end;
        size_t flow_start = 0;
        size_t flow_end = 0;
        bool in_s = false, in_d = false;
        int bracket_depth = 0, brace_depth = 0;
        size_t colon_pos = SIZE_MAX;

        while (pos < len && isspace((unsigned char)data[pos])) pos++;
        if (pos >= len) break;
        key_start = pos;

        while (pos < len) {
            char c = data[pos];
            if (c == '\'' && !in_d) in_s = !in_s;
            else if (c == '"' && !in_s) in_d = !in_d;
            else if (!in_s && !in_d) {
                if (c == '[')
                    bracket_depth++;
                else if (c == ']' && bracket_depth > 0)
                    bracket_depth--;
                else if (c == '{')
                    brace_depth++;
                else if (c == '}' && brace_depth > 0)
                    brace_depth--;
                else if (c == ':' && bracket_depth == 0 && brace_depth == 0) {
                    colon_pos = pos;
                    pos++;
                    break;
                }
            }
            pos++;
        }

        key_end = (colon_pos == SIZE_MAX) ? pos : colon_pos;
        while (key_start < key_end && isspace((unsigned char)data[key_start]))
            key_start++;
        while (key_end > key_start && isspace((unsigned char)data[key_end - 1]))
            key_end--;

        if (colon_pos == SIZE_MAX) {
            yyaml_set_error(err, line_start, line, column,
                             "unterminated mapping entry");
            return false;
        }

        val_start = pos;
        in_s = in_d = false;
        bracket_depth = brace_depth = 0;
        while (pos < len) {
            char c = data[pos];
            if (c == '\'' && !in_d) in_s = !in_s;
            else if (c == '"' && !in_s) in_d = !in_d;
            else if (!in_s && !in_d) {
                if (c == '[')
                    bracket_depth++;
                else if (c == ']' && bracket_depth > 0)
                    bracket_depth--;
                else if (c == '{')
                    brace_depth++;
                else if (c == '}' && brace_depth > 0)
                    brace_depth--;
                else if (c == ',' && bracket_depth == 0 && brace_depth == 0) {
                    break;
                }
            }
            pos++;
        }

        val_end = pos;
        while (val_start < val_end && isspace((unsigned char)data[val_start]))
            val_start++;
        while (val_end > val_start && isspace((unsigned char)data[val_end - 1]))
            val_end--;

        {
            uint32_t idx = yyaml_doc_add_node(doc, YYAML_NULL);
            uint32_t key_ofs = 0;
            size_t key_len = key_end - key_start;
            if (idx == YYAML_INDEX_NONE) {
                yyaml_set_error(err, line_start, line, column,
                                 "out of memory");
                return false;
            }
            if (!yyaml_doc_store_string(doc, data + key_start, key_len,
                                        &key_ofs)) {
                yyaml_set_error(err, line_start, line, column,
                                 "out of memory");
                return false;
            }
            doc->nodes[idx].flags = (uint32_t)key_len;
            doc->nodes[idx].extra = key_ofs;
            yyaml_doc_link_child(doc, &map_level, idx);

            if (val_start == val_end) {
                doc->nodes[idx].type = YYAML_NULL;
            } else if (yyaml_is_flow_sequence(data + val_start, val_end - val_start,
                                              &flow_start, &flow_end)) {
                doc->nodes[idx].type = YYAML_SEQUENCE;
                if (!yyaml_fill_flow_sequence(doc, idx,
                                              data + val_start + flow_start,
                                              flow_end - flow_start, cfg, err,
                                              line_start, line,
                                              val_start - line_start + 1))
                    return false;
            } else if (yyaml_is_flow_mapping(data + val_start, val_end - val_start,
                                             &flow_start, &flow_end)) {
                doc->nodes[idx].type = YYAML_MAPPING;
                if (!yyaml_fill_flow_mapping(doc, idx,
                                             data + val_start + flow_start,
                                             flow_end - flow_start, cfg, err,
                                             line_start, line,
                                             val_start - line_start + 1))
                    return false;
            } else {
                yyaml_node temp = {0};
                if (!yyaml_parse_scalar(data + val_start, val_end - val_start,
                                        doc, &temp, cfg, err, line_start, line,
                                        val_start - line_start + 1)) {
                    return false;
                }
                doc->nodes[idx].type = temp.type;
                doc->nodes[idx].val = temp.val;
                if (temp.type == YYAML_STRING) {
                    doc->nodes[idx].val.str.ofs = temp.val.str.ofs;
                    doc->nodes[idx].val.str.len = temp.val.str.len;
                }
            }
        }

        if (pos < len && data[pos] == ',') {
            pos++;
        }
        while (pos < len && isspace((unsigned char)data[pos])) pos++;
    }

    return true;
}

static bool yyaml_parse_block_scalar(const char *data, size_t len,
                                     size_t indent_level, size_t *pos,
                                     size_t *line, yyaml_doc *doc,
                                     yyaml_node *node, bool folded,
                                     yyaml_err *err) {
    size_t base_indent = indent_level + 1;
    size_t start_pos = *pos;
    size_t buf_cap = len - start_pos + 1;
    size_t buf_len = 0;
    char *buf = (char *)malloc(buf_cap);

    if (!buf) {
        yyaml_set_error(err, start_pos, *line, 1, "out of memory");
        return false;
    }

    while (*pos < len) {
        size_t cur_indent = 0;
        size_t line_start = *pos;
        while (*pos < len && data[*pos] == ' ') {
            cur_indent++;
            (*pos)++;
        }

        size_t content_start = *pos;
        while (*pos < len && data[*pos] != '\n' && data[*pos] != '\r') {
            (*pos)++;
        }
        size_t line_end = *pos;
        bool blank_line = (content_start == line_end);

        if (cur_indent < base_indent) {
            if (blank_line) {
                *pos = line_start;
                break;
            }
            *pos = line_start;
            break;
        }

        if (cur_indent < base_indent) cur_indent = base_indent;
        if (line_end > content_start) {
            size_t slice_start = line_start + base_indent;
            if (slice_start < content_start) slice_start = content_start;
            if (slice_start > line_end) slice_start = line_end;
            size_t slice_len = line_end - slice_start;
            memcpy(buf + buf_len, data + slice_start, slice_len);
            buf_len += slice_len;
        }

        if (*pos < len && data[*pos] == '\r' && *pos + 1 < len &&
            data[*pos + 1] == '\n') {
            (*pos)++;
        }
        if (*pos < len && data[*pos] == '\n') {
            (*pos)++;
            (*line)++;
        }

        if (buf_len < buf_cap) {
            buf[buf_len++] = folded && !blank_line ? ' ' : '\n';
        }
    }

    if (folded) {
        if (buf_len == 0) {
            buf[buf_len++] = '\n';
        } else if (buf[buf_len - 1] == ' ') {
            buf[buf_len - 1] = '\n';
        } else if (buf_len < buf_cap) {
            buf[buf_len++] = '\n';
        }
    }

    {
        uint32_t ofs;
        if (!yyaml_doc_store_string(doc, buf, buf_len, &ofs)) {
            free(buf);
            yyaml_set_error(err, start_pos, *line, 1, "out of memory");
            return false;
        }
        node->type = YYAML_STRING;
        node->val.str.ofs = ofs;
        node->val.str.len = (uint32_t)buf_len;
    }

    free(buf);
    return true;
}

/* ---------------------------- scalar parsing ------------------------------ */

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

static bool yyaml_parse_quoted(const char *str, size_t len, yyaml_doc *doc,
                               uint32_t *out_idx, uint32_t *out_len,
                               yyaml_err *err, size_t pos, size_t line,
                               size_t col) {
    char quote;
    size_t i = 0, j = 0;
    uint32_t ofs;
    char *buf;
    if (len < 2) {
        yyaml_set_error(err, pos, line, col, "unterminated quoted string");
        return false;
    }
    quote = str[0];
    if (str[len - 1] != quote) {
        yyaml_set_error(err, pos, line, col, "unterminated quoted string");
        return false;
    }
    if (!yyaml_doc_reserve_str(doc, doc->scalar_len + len + 1)) return false;
    ofs = (uint32_t)doc->scalar_len;
    buf = doc->scalars + doc->scalar_len;
    for (i = 1; i + 1 < len; i++) {
        char c = str[i];
        if (quote == '"' && c == '\\') {
            if (i + 1 >= len - 1) {
                yyaml_set_error(err, pos, line, col, "invalid escape sequence");
                return false;
            }
            c = str[++i];
            switch (c) {
                case '\"': buf[j++] = '"'; break;
                case '\\': buf[j++] = '\\'; break;
                case 'n': buf[j++] = '\n'; break;
                case 'r': buf[j++] = '\r'; break;
                case 't': buf[j++] = '\t'; break;
                case '0': buf[j++] = '\0'; break;
                default:
                    yyaml_set_error(err, pos, line, col, "unsupported escape");
                    return false;
            }
        } else {
            buf[j++] = c;
        }
    }
    buf[j++] = '\0';
    doc->scalar_len += j;
    *out_idx = ofs;
    if (out_len) *out_len = (uint32_t)(j - 1);
    return true;
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
    if ((str[0] == '"' && str[len - 1] == '"') ||
        (str[0] == '\'' && str[len - 1] == '\'')) {
        uint32_t ofs, slen;
        if (!yyaml_parse_quoted(str, len, doc, &ofs, &slen, err, pos, line, col))
            return false;
        node->type = YYAML_STRING;
        node->val.str.ofs = ofs;
        node->val.str.len = slen;
        return true;
    }
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
    if (yyaml_parse_inf_nan(str, len, &dbl, opts && opts->allow_inf_nan)) {
        node->type = YYAML_DOUBLE;
        node->val.real = dbl;
        return true;
    }
    if (yyaml_parse_bool(str, len, &boolean)) {
        node->type = YYAML_BOOL;
        node->val.boolean = boolean;
        return true;
    }
    if (yyaml_parse_null(str, len)) {
        node->type = YYAML_NULL;
        return true;
    }
    {
        uint32_t ofs;
        if (!yyaml_doc_store_string(doc, str, len, &ofs)) return false;
        node->type = YYAML_STRING;
        node->val.str.ofs = ofs;
        node->val.str.len = (uint32_t)len;
    }
    return true;
}

/* ------------------------------- parsing --------------------------------- */

static const yyaml_read_opts yyaml_default_opts = {false, false, true, 64};

YYAML_API yyaml_doc *yyaml_read(const char *data, size_t len,
                                const yyaml_read_opts *opts,
                                yyaml_err *err) {
    const yyaml_read_opts *cfg = opts ? opts : &yyaml_default_opts;
    yyaml_doc *doc;
    size_t pos = 0, line = 1, col = 1;
    yyaml_level stack[128];
    size_t stack_sz = 0;
    yyaml_pending pending = {0};
    size_t last_indent = 0;

    if (!data) {
        yyaml_set_error(err, 0, 1, 1, "input buffer is null");
        return NULL;
    }
    doc = (yyaml_doc *)calloc(1, sizeof(*doc));
    if (!doc) return NULL;
    doc->root = YYAML_INDEX_NONE;

    while (pos < len) {
        size_t line_start = pos;
        size_t indent = 0;
        bool seq_item = false;
        size_t content_start;
        size_t content_end;
        const char *line_ptr;
        bool has_colon = false;
        bool in_single = false, in_double = false;
        yyaml_level *parent_level = NULL;
        yyaml_node temp_node;
        char ch;

        /* fetch line */
        while (pos < len && (data[pos] == '\r' || data[pos] == '\n')) {
            if (data[pos] == '\n') { line++; col = 1; }
            else col++;
            pos++;
        }
        if (pos >= len) break;
        line_start = pos;
        indent = 0;
        while (pos < len) {
            ch = data[pos];
            if (ch == ' ') { indent++; pos++; col++; }
            else if (ch == '\t') {
                yyaml_set_error(err, pos, line, col, "tabs are not supported");
                goto fail;
            } else {
                break;
            }
        }
        if (pos >= len) break;
        if (data[pos] == '#') {
            /* skip comment */
            while (pos < len && data[pos] != '\n') pos++;
            continue;
        }
        if (data[pos] == '\r' || data[pos] == '\n') {
            /* blank line */
            while (pos < len && data[pos] != '\n') pos++;
            continue;
        }
        line_ptr = data + pos;
        if (*line_ptr == '-') {
            char next = (pos + 1 < len) ? data[pos + 1] : '\n';
            if (next == ' ' || next == '\t' || next == '\r' || next == '\n') {
                seq_item = true;
                pos++;
                col++;
                if (pos < len && data[pos] == ' ') {
                    pos++;
                    col++;
                }
            }
        }
        content_start = pos;
        while (pos < len && data[pos] != '\n' && data[pos] != '\r') {
            ch = data[pos];
            if (ch == '\'' && !in_double) in_single = !in_single;
            else if (ch == '"' && !in_single) in_double = !in_double;
            else if (ch == '#' && !in_single && !in_double) break;
            else if (ch == ':' && !in_single && !in_double) {
                size_t nxt = pos + 1;
                if (nxt >= len || data[nxt] == ' ' || data[nxt] == '\t' ||
                    data[nxt] == '\r' || data[nxt] == '\n') {
                    has_colon = true;
                }
            }
            pos++;
            col++;
        }
        content_end = pos;
        while (content_end > content_start &&
               isspace((unsigned char)data[content_end - 1])) {
            content_end--;
        }
        /* skip to next line */
        while (pos < len && data[pos] != '\n') pos++;
        if (pos < len && data[pos] == '\n') { pos++; line++; col = 1; }

        /* Determine parent */
        if (indent > last_indent || (pending.active && indent == last_indent)) {
            if (!pending.active) {
                yyaml_set_error(err, line_start, line, 1,
                                 "unexpected indentation");
                goto fail;
            }
            if (seq_item) pending.prefer_sequence = true;
            else pending.prefer_sequence = false;
            if ((cfg->max_nesting && stack_sz >= cfg->max_nesting) ||
                stack_sz >= sizeof(stack) / sizeof(stack[0])) {
                yyaml_set_error(err, line_start, line, indent,
                                 "nesting limit exceeded");
                goto fail;
            }
            /* convert pending node to container */
            yyaml_node *pnode = &doc->nodes[pending.node];
            pnode->type = pending.prefer_sequence ? YYAML_SEQUENCE : YYAML_MAPPING;
            pnode->val.integer = 0;
            stack[stack_sz].indent = indent;
            stack[stack_sz].container = pending.node;
            stack[stack_sz].last_child = YYAML_INDEX_NONE;
            stack[stack_sz].is_sequence = pending.prefer_sequence;
            stack_sz++;
            pending.active = false;
            last_indent = indent;
        } else {
            while (stack_sz && indent < stack[stack_sz - 1].indent) {
                stack_sz--;
            }
            if (stack_sz && indent != stack[stack_sz - 1].indent) {
                yyaml_set_error(err, line_start, line, 1,
                                 "misaligned indentation");
                goto fail;
            }
            last_indent = indent;
            pending.active = false;
        }
        if (stack_sz) parent_level = &stack[stack_sz - 1];
        else parent_level = NULL;

        /* Validate container expectations */
        while (!seq_item && parent_level && parent_level->is_sequence &&
               indent <= parent_level->indent) {
            stack_sz--;
            parent_level = stack_sz ? &stack[stack_sz - 1] : NULL;
        }

        if (seq_item) {
            if (!parent_level || !parent_level->is_sequence) {
                if (pending.active) {
                    /* promote pending to sequence */
                    yyaml_node *pnode = &doc->nodes[pending.node];
                    pnode->type = YYAML_SEQUENCE;
                    pnode->val.integer = 0;
                    if ((cfg->max_nesting && stack_sz >= cfg->max_nesting) ||
                        stack_sz >= sizeof(stack) / sizeof(stack[0])) {
                        yyaml_set_error(err, line_start, line, indent,
                                         "nesting limit exceeded");
                        goto fail;
                    }
                    stack[stack_sz].indent = indent;
                    stack[stack_sz].container = pending.node;
                    stack[stack_sz].last_child = YYAML_INDEX_NONE;
                    stack[stack_sz].is_sequence = true;
                    stack_sz++;
                    pending.active = false;
                    parent_level = &stack[stack_sz - 1];
                } else if (parent_level) {
                    yyaml_set_error(err, line_start, line, 1,
                                     "sequence item without sequence context");
                    goto fail;
                }
            }
        } else if (parent_level && parent_level->is_sequence) {
            yyaml_set_error(err, line_start, line, 1,
                             "expected sequence item");
            goto fail;
        }

        memset(&temp_node, 0, sizeof(temp_node));
        temp_node.type = YYAML_NULL;
        temp_node.child = YYAML_INDEX_NONE;
        temp_node.next = YYAML_INDEX_NONE;
        temp_node.parent = YYAML_INDEX_NONE;
        temp_node.flags = 0;
        temp_node.extra = 0;

        if (seq_item) {
            yyaml_level map_level = {0};
            bool map_from_sequence = false;
            size_t map_child_indent = indent;
            if (!parent_level) {
                /* root level sequence */
                uint32_t seq_idx;
                if (doc->root != YYAML_INDEX_NONE) {
                    yyaml_set_error(err, line_start, line, 1,
                                     "multiple root nodes");
                    goto fail;
                }
                seq_idx = yyaml_doc_add_node(doc, YYAML_SEQUENCE);
                if (seq_idx == YYAML_INDEX_NONE) goto fail_nomem;
                doc->root = seq_idx;
                if ((cfg->max_nesting && stack_sz >= cfg->max_nesting) ||
                    stack_sz >= sizeof(stack) / sizeof(stack[0])) {
                    yyaml_set_error(err, line_start, line, indent,
                                     "nesting limit exceeded");
                    goto fail;
                }
                stack[stack_sz].indent = indent;
                stack[stack_sz].container = seq_idx;
                stack[stack_sz].last_child = YYAML_INDEX_NONE;
                stack[stack_sz].is_sequence = true;
                stack_sz++;
                parent_level = &stack[stack_sz - 1];
            }
            if (has_colon) {
                size_t base = (size_t)(line_ptr - data);
                size_t offset = content_start > base ?
                                 (size_t)(content_start - base) : 0;
                if (offset > 0) map_child_indent += offset;
                else map_child_indent++;
                uint32_t map_idx = yyaml_doc_add_node(doc, YYAML_MAPPING);
                if (map_idx == YYAML_INDEX_NONE) goto fail_nomem;
                yyaml_doc_link_child(doc, parent_level, map_idx);
                map_level.indent = map_child_indent;
                map_level.container = map_idx;
                map_level.last_child = YYAML_INDEX_NONE;
                map_level.is_sequence = false;
                parent_level = &map_level;
                map_from_sequence = true;
                /* fall through to mapping handling below */
            } else if (content_start == content_end) {
                /* placeholder null, may become container */
                uint32_t idx = yyaml_doc_add_node(doc, YYAML_NULL);
                if (idx == YYAML_INDEX_NONE) goto fail_nomem;
                yyaml_doc_link_child(doc, parent_level, idx);
                pending.active = true;
                pending.node = idx;
                pending.prefer_sequence = false;
                continue;
            } else {
                size_t value_len = content_end - content_start;
                size_t flow_start = 0;
                size_t flow_end = 0;
                if (value_len == 1 &&
                    (data[content_start] == '|' || data[content_start] == '>')) {
                    yyaml_node block_node = {0};
                    bool folded = data[content_start] == '>';
                    if (!yyaml_parse_block_scalar(data, len, indent, &pos, &line,
                                                 doc, &block_node, folded, err))
                        goto fail;
                    uint32_t idx = yyaml_doc_add_node(doc, YYAML_STRING);
                    if (idx == YYAML_INDEX_NONE) goto fail_nomem;
                    doc->nodes[idx] = block_node;
                    doc->nodes[idx].doc = doc;
                    yyaml_doc_link_child(doc, parent_level, idx);
                    col = 1;
                    continue;
                } else if (yyaml_is_flow_sequence(data + content_start, value_len,
                                           &flow_start, &flow_end)) {
                    uint32_t idx = yyaml_doc_add_node(doc, YYAML_SEQUENCE);
                    if (idx == YYAML_INDEX_NONE) goto fail_nomem;
                    yyaml_doc_link_child(doc, parent_level, idx);
                    if (!yyaml_fill_flow_sequence(doc, idx,
                                                  data + content_start +
                                                      flow_start,
                                                  flow_end - flow_start,
                                                  cfg, err, line_start, line,
                                                  indent + 1))
                        goto fail;
                } else if (yyaml_is_flow_mapping(data + content_start, value_len,
                                                 &flow_start, &flow_end)) {
                    uint32_t idx = yyaml_doc_add_node(doc, YYAML_MAPPING);
                    if (idx == YYAML_INDEX_NONE) goto fail_nomem;
                    yyaml_doc_link_child(doc, parent_level, idx);
                    if (!yyaml_fill_flow_mapping(doc, idx,
                                                 data + content_start +
                                                     flow_start,
                                                 flow_end - flow_start, cfg,
                                                 err, line_start, line,
                                                 indent + 1))
                        goto fail;
                } else {
                    if (!yyaml_parse_scalar(data + content_start, value_len,
                                             doc, &temp_node, cfg, err,
                                             line_start, line, indent + 1))
                        goto fail;
                    if (temp_node.type == YYAML_STRING) {
                        uint32_t idx =
                            yyaml_doc_add_node(doc, YYAML_STRING);
                        if (idx == YYAML_INDEX_NONE) goto fail_nomem;
                        doc->nodes[idx] = temp_node;
                        doc->nodes[idx].doc = doc;
                        yyaml_doc_link_child(doc, parent_level, idx);
                    } else {
                        uint32_t idx =
                            yyaml_doc_add_node(doc, temp_node.type);
                        if (idx == YYAML_INDEX_NONE) goto fail_nomem;
                        doc->nodes[idx] = temp_node;
                        doc->nodes[idx].doc = doc;
                        yyaml_doc_link_child(doc, parent_level, idx);
                    }
                }
                continue;
            }

            if (map_from_sequence) {
                size_t flow_start = 0;
                size_t flow_end = 0;
                const char *colon = NULL;
                size_t j;
                bool in_s = false, in_d = false;
                for (j = content_start; j < content_end; j++) {
                    char c = data[j];
                    if (c == '\'' && !in_d) in_s = !in_s;
                    else if (c == '"' && !in_s) in_d = !in_d;
                    else if (c == ':' && !in_s && !in_d) {
                        size_t nxt = j + 1;
                        if (nxt >= content_end || data[nxt] == ' ' ||
                            data[nxt] == '\t') {
                            colon = &data[j];
                            break;
                        }
                    }
                }
                if (!colon) {
                    yyaml_set_error(err, line_start, line, indent + 1,
                                     "unterminated mapping entry");
                    goto fail;
                }
                size_t key_start = content_start;
                size_t key_end = (size_t)(colon - data);
                while (key_start < key_end &&
                       isspace((unsigned char)data[key_start])) key_start++;
                while (key_end > key_start &&
                       isspace((unsigned char)data[key_end - 1])) key_end--;
                size_t val_start = (size_t)(colon - data + 1);
                while (val_start < content_end &&
                       isspace((unsigned char)data[val_start]))
                    val_start++;
                size_t val_len = content_end - val_start;
                if (yyaml_is_anchor_only(data + val_start, val_len)) val_len = 0;
                size_t key_len = key_end - key_start;
                const char *key_ptr = data + key_start;
                uint32_t idx = yyaml_doc_add_node(doc, YYAML_NULL);
                uint32_t key_ofs = 0;
                if (idx == YYAML_INDEX_NONE) goto fail_nomem;
                if (!yyaml_doc_store_string(doc, key_ptr, key_len, &key_ofs))
                    goto fail_nomem;
                doc->nodes[idx].flags = (uint32_t)key_len;
                doc->nodes[idx].extra = key_ofs;
                yyaml_doc_link_child(doc, &map_level, idx);
                if (val_len == 0) {
                    pending.active = true;
                    pending.node = idx;
                    pending.prefer_sequence = false;
                } else if (val_len == 1 &&
                           (data[val_start] == '|' || data[val_start] == '>')) {
                    yyaml_node block_node = {0};
                    bool folded = data[val_start] == '>';
                    if (!yyaml_parse_block_scalar(data, len, map_child_indent, &pos,
                                                 &line, doc, &block_node, folded,
                                                 err))
                        goto fail;
                    doc->nodes[idx].type = block_node.type;
                    doc->nodes[idx].val = block_node.val;
                    col = 1;
                } else if (yyaml_is_flow_sequence(data + val_start, val_len,
                                                  &flow_start, &flow_end)) {
                    doc->nodes[idx].type = YYAML_SEQUENCE;
                    if (!yyaml_fill_flow_sequence(doc, idx,
                                                  data + val_start +
                                                      flow_start,
                                                  flow_end - flow_start,
                                                  cfg, err, line_start, line,
                                                  val_start - line_start + 1))
                        goto fail;
                } else if (yyaml_is_flow_mapping(data + val_start, val_len,
                                                &flow_start, &flow_end)) {
                    doc->nodes[idx].type = YYAML_MAPPING;
                    if (!yyaml_fill_flow_mapping(doc, idx,
                                                data + val_start + flow_start,
                                                flow_end - flow_start, cfg, err,
                                                line_start, line,
                                                val_start - line_start + 1))
                        goto fail;
                } else {
                    if (!yyaml_parse_scalar(data + val_start, val_len, doc,
                                             &temp_node, cfg, err, line_start,
                                             line, val_start - line_start + 1))
                        goto fail;
                    doc->nodes[idx].type = temp_node.type;
                    doc->nodes[idx].val = temp_node.val;
                    if (temp_node.type == YYAML_STRING) {
                        doc->nodes[idx].val.str.ofs = temp_node.val.str.ofs;
                        doc->nodes[idx].val.str.len = temp_node.val.str.len;
                    }
                }
                if ((cfg->max_nesting && stack_sz >= cfg->max_nesting) ||
                    stack_sz >= sizeof(stack) / sizeof(stack[0])) {
                    yyaml_set_error(err, line_start, line, indent,
                                     "nesting limit exceeded");
                    goto fail;
                }
                stack[stack_sz] = map_level;
                stack_sz++;
                last_indent = map_child_indent;
                continue;
            }
            continue;
        }

        if (has_colon) {
            const char *colon = NULL;
            size_t j;
            bool in_s = false, in_d = false;
            for (j = content_start; j < content_end; j++) {
                char c = data[j];
                if (c == '\'' && !in_d) in_s = !in_s;
                else if (c == '"' && !in_s) in_d = !in_d;
                else if (c == ':' && !in_s && !in_d) {
                    size_t nxt = j + 1;
                    if (nxt >= content_end ||
                        data[nxt] == ' ' || data[nxt] == '\t') {
                        colon = &data[j];
                        break;
                    }
                }
            }
            if (!colon) {
                yyaml_set_error(err, line_start, line, indent + 1,
                                 "unterminated mapping entry");
                goto fail;
            }
            /* extract key */
            size_t key_start = content_start;
            size_t key_end = (size_t)(colon - data);
            while (key_start < key_end &&
                   isspace((unsigned char)data[key_start])) key_start++;
            while (key_end > key_start &&
                   isspace((unsigned char)data[key_end - 1])) key_end--;
            size_t val_start = (size_t)(colon - data + 1);
            while (val_start < content_end && isspace((unsigned char)data[val_start]))
                val_start++;
            size_t val_len = content_end - val_start;
            if (yyaml_is_anchor_only(data + val_start, val_len)) val_len = 0;
            size_t key_len = key_end - key_start;
            const char *key_ptr = data + key_start;
            /* parent preparation */
            if (!parent_level) {
                if (doc->root == YYAML_INDEX_NONE) {
                    uint32_t map_idx = yyaml_doc_add_node(doc, YYAML_MAPPING);
                    if (map_idx == YYAML_INDEX_NONE) goto fail_nomem;
                    doc->root = map_idx;
                    if ((cfg->max_nesting && stack_sz >= cfg->max_nesting) ||
                        stack_sz >= sizeof(stack) / sizeof(stack[0])) {
                        yyaml_set_error(err, line_start, line, indent,
                                         "nesting limit exceeded");
                        goto fail;
                    }
                    stack[stack_sz].indent = indent;
                    stack[stack_sz].container = map_idx;
                    stack[stack_sz].last_child = YYAML_INDEX_NONE;
                    stack[stack_sz].is_sequence = false;
                    stack_sz++;
                    parent_level = &stack[stack_sz - 1];
                } else {
                    yyaml_set_error(err, line_start, line, indent + 1,
                                     "multiple root nodes");
                    goto fail;
                }
            }
            if (parent_level->is_sequence) {
                yyaml_set_error(err, line_start, line, indent + 1,
                                 "mapping entry inside sequence without item");
                goto fail;
            }
            if (!cfg->allow_duplicate_keys) {
                uint32_t dup = doc->nodes[parent_level->container].child;
                while (dup != YYAML_INDEX_NONE) {
                    yyaml_node *dnode = &doc->nodes[dup];
                    if (dnode->flags == key_len && doc->scalars &&
                        memcmp(doc->scalars + dnode->extra, key_ptr, key_len) == 0) {
                        yyaml_set_error(err, line_start, line, indent + 1,
                                         "duplicate mapping key");
                        goto fail;
                    }
                    dup = dnode->next;
                }
            }
            /* create value node */
            uint32_t idx = yyaml_doc_add_node(doc, YYAML_NULL);
            uint32_t key_ofs = 0;
            if (idx == YYAML_INDEX_NONE) goto fail_nomem;
            if (!yyaml_doc_store_string(doc, key_ptr, key_len, &key_ofs))
                goto fail_nomem;
            doc->nodes[idx].flags = (uint32_t)key_len;
            doc->nodes[idx].extra = key_ofs;
            if (val_len == 0) {
                /* awaiting nested block */
                yyaml_doc_link_child(doc, parent_level, idx);
                pending.active = true;
                pending.node = idx;
                pending.prefer_sequence = false;
            } else {
                size_t flow_start = 0;
                size_t flow_end = 0;
                yyaml_doc_link_child(doc, parent_level, idx);
                if (val_len == 1 &&
                    (data[val_start] == '|' || data[val_start] == '>')) {
                    yyaml_node block_node = {0};
                    bool folded = data[val_start] == '>';
                    if (!yyaml_parse_block_scalar(data, len, indent, &pos, &line,
                                                 doc, &block_node, folded, err))
                        goto fail;
                    doc->nodes[idx].type = block_node.type;
                    doc->nodes[idx].val = block_node.val;
                    col = 1;
                } else if (yyaml_is_flow_sequence(data + val_start, val_len,
                                           &flow_start, &flow_end)) {
                    doc->nodes[idx].type = YYAML_SEQUENCE;
                    if (!yyaml_fill_flow_sequence(doc, idx,
                                                  data + val_start +
                                                      flow_start,
                                                  flow_end - flow_start,
                                                  cfg, err, line_start, line,
                                                  val_start - line_start + 1))
                        goto fail;
                } else if (yyaml_is_flow_mapping(data + val_start, val_len,
                                                &flow_start, &flow_end)) {
                    doc->nodes[idx].type = YYAML_MAPPING;
                    if (!yyaml_fill_flow_mapping(doc, idx,
                                                data + val_start + flow_start,
                                                flow_end - flow_start, cfg, err,
                                                line_start, line,
                                                val_start - line_start + 1))
                        goto fail;
                } else {
                    if (!yyaml_parse_scalar(data + val_start, val_len, doc,
                                             &temp_node, cfg, err, line_start,
                                             line, val_start - line_start + 1))
                        goto fail;
                    doc->nodes[idx].type = temp_node.type;
                    doc->nodes[idx].val = temp_node.val;
                    if (temp_node.type == YYAML_STRING) {
                        doc->nodes[idx].val.str.ofs = temp_node.val.str.ofs;
                        doc->nodes[idx].val.str.len = temp_node.val.str.len;
                    }
                }
            }
            continue;
        }

        /* plain scalar at top level */
        if (parent_level) {
            yyaml_set_error(err, line_start, line, indent + 1,
                             "unexpected scalar inside container");
            goto fail;
        }
        if (doc->root != YYAML_INDEX_NONE) {
            if (cfg->allow_trailing_content) break;
            yyaml_set_error(err, line_start, line, indent + 1,
                             "multiple root nodes");
            goto fail;
        }
        if (!yyaml_parse_scalar(data + content_start,
                                 content_end - content_start, doc,
                                 &temp_node, cfg, err, line_start, line,
                                 indent + 1))
            goto fail;
        doc->root = yyaml_doc_add_node(doc, temp_node.type);
        if (doc->root == YYAML_INDEX_NONE) goto fail_nomem;
        doc->nodes[doc->root] = temp_node;
        doc->nodes[doc->root].doc = doc;
    }

    if (doc->root == YYAML_INDEX_NONE) {
        doc->root = yyaml_doc_add_node(doc, YYAML_NULL);
        if (doc->root == YYAML_INDEX_NONE) goto fail_nomem;
    }
    return doc;

fail_nomem:
    yyaml_set_error(err, pos, line, col, "out of memory");
fail:
    yyaml_doc_free(doc);
    return NULL;
}

YYAML_API yyaml_doc *yyaml_doc_new(void) {
    yyaml_doc *doc = (yyaml_doc *)calloc(1, sizeof(*doc));
    if (!doc) return NULL;
    doc->root = YYAML_INDEX_NONE;
    return doc;
}

/* ------------------------------ traversal -------------------------------- */

YYAML_API void yyaml_doc_free(yyaml_doc *doc) {
    if (!doc) return;
    free(doc->nodes);
    free(doc->scalars);
    free(doc);
}

YYAML_API const yyaml_node *yyaml_doc_get_root(const yyaml_doc *doc) {
    if (!doc || doc->root == YYAML_INDEX_NONE) return NULL;
    return &doc->nodes[doc->root];
}

YYAML_API const yyaml_node *yyaml_doc_get(const yyaml_doc *doc, uint32_t idx) {
    if (!doc || idx == YYAML_INDEX_NONE || idx >= doc->node_count) return NULL;
    return &doc->nodes[idx];
}

YYAML_API uint32_t yyaml_node_index(const yyaml_doc *doc, const yyaml_node *node) {
    if (!doc || !node || node->doc != doc || !doc->nodes) return YYAML_INDEX_NONE;
    ptrdiff_t offset = node - doc->nodes;
    if (offset < 0 || (size_t)offset >= doc->node_count) return YYAML_INDEX_NONE;
    return (uint32_t)offset;
}

YYAML_API const char *yyaml_doc_get_scalar_buf(const yyaml_doc *doc) {
    return doc ? doc->scalars : NULL;
}

YYAML_API size_t yyaml_doc_node_count(const yyaml_doc *doc) {
    return doc ? doc->node_count : 0;
}

YYAML_API const yyaml_node *yyaml_map_get(const yyaml_node *map,
                                          const char *key) {
    const yyaml_doc *doc;
    const yyaml_node *node;
    uint32_t idx;
    size_t key_len;
    const char *buf;
    if (!map || map->type != YYAML_MAPPING || !key) return NULL;
    doc = map->doc;
    if (!doc) return NULL;
    buf = doc->scalars;
    key_len = strlen(key);
    idx = map->child;
    node = NULL;
    while (idx != YYAML_INDEX_NONE) {
        const yyaml_node *cur = &doc->nodes[idx];
        if (cur->flags == key_len && buf &&
            memcmp(buf + cur->extra, key, key_len) == 0) {
            node = cur;
        }
        idx = cur->next;
    }
    return node;
}

YYAML_API const yyaml_node *yyaml_seq_get(const yyaml_node *seq,
                                          size_t index) {
    uint32_t idx;
    const yyaml_doc *doc;
    if (!seq || seq->type != YYAML_SEQUENCE) return NULL;
    doc = seq->doc;
    if (!doc) return NULL;
    idx = seq->child;
    while (idx != YYAML_INDEX_NONE && index--) {
        idx = doc->nodes[idx].next;
    }
    if (idx == YYAML_INDEX_NONE) return NULL;
    return &doc->nodes[idx];
}

YYAML_API size_t yyaml_seq_len(const yyaml_node *seq) {
    if (!seq || seq->type != YYAML_SEQUENCE) return 0;
    return (size_t)seq->val.integer;
}

YYAML_API size_t yyaml_map_len(const yyaml_node *map) {
    if (!map || map->type != YYAML_MAPPING) return 0;
    return (size_t)map->val.integer;
}

/* --------------------------- building API ------------------------------- */

YYAML_API bool yyaml_doc_set_root(yyaml_doc *doc, uint32_t idx) {
    if (!doc || idx == YYAML_INDEX_NONE || idx >= doc->node_count) return false;
    doc->root = idx;
    return true;
}

YYAML_API uint32_t yyaml_doc_add_null(yyaml_doc *doc) {
    if (!doc) return YYAML_INDEX_NONE;
    return yyaml_doc_add_node(doc, YYAML_NULL);
}

YYAML_API uint32_t yyaml_doc_add_bool(yyaml_doc *doc, bool value) {
    uint32_t idx;
    if (!doc) return YYAML_INDEX_NONE;
    idx = yyaml_doc_add_node(doc, YYAML_BOOL);
    if (idx != YYAML_INDEX_NONE) {
        doc->nodes[idx].val.boolean = value;
    }
    return idx;
}

YYAML_API uint32_t yyaml_doc_add_int(yyaml_doc *doc, int64_t value) {
    uint32_t idx;
    if (!doc) return YYAML_INDEX_NONE;
    idx = yyaml_doc_add_node(doc, YYAML_INT);
    if (idx != YYAML_INDEX_NONE) {
        doc->nodes[idx].val.integer = value;
    }
    return idx;
}

YYAML_API uint32_t yyaml_doc_add_double(yyaml_doc *doc, double value) {
    uint32_t idx;
    if (!doc) return YYAML_INDEX_NONE;
    idx = yyaml_doc_add_node(doc, YYAML_DOUBLE);
    if (idx != YYAML_INDEX_NONE) {
        doc->nodes[idx].val.real = value;
    }
    return idx;
}

YYAML_API uint32_t yyaml_doc_add_string(yyaml_doc *doc, const char *str,
                                        size_t len) {
    uint32_t idx;
    uint32_t ofs = 0;
    if (!doc || !str) return YYAML_INDEX_NONE;
    idx = yyaml_doc_add_node(doc, YYAML_STRING);
    if (idx == YYAML_INDEX_NONE) return YYAML_INDEX_NONE;
    if (!yyaml_doc_store_string(doc, str, len, &ofs)) return YYAML_INDEX_NONE;
    doc->nodes[idx].val.str.ofs = ofs;
    doc->nodes[idx].val.str.len = (uint32_t)len;
    return idx;
}

YYAML_API uint32_t yyaml_doc_add_sequence(yyaml_doc *doc) {
    uint32_t idx;
    if (!doc) return YYAML_INDEX_NONE;
    idx = yyaml_doc_add_node(doc, YYAML_SEQUENCE);
    if (idx != YYAML_INDEX_NONE) {
        doc->nodes[idx].val.integer = 0;
    }
    return idx;
}

YYAML_API uint32_t yyaml_doc_add_mapping(yyaml_doc *doc) {
    uint32_t idx;
    if (!doc) return YYAML_INDEX_NONE;
    idx = yyaml_doc_add_node(doc, YYAML_MAPPING);
    if (idx != YYAML_INDEX_NONE) {
        doc->nodes[idx].val.integer = 0;
    }
    return idx;
}

YYAML_API bool yyaml_doc_seq_append(yyaml_doc *doc, uint32_t seq_idx,
                                    uint32_t child_idx) {
    yyaml_node *seq;
    if (!doc || seq_idx == YYAML_INDEX_NONE || child_idx == YYAML_INDEX_NONE) {
        return false;
    }
    if (seq_idx >= doc->node_count || child_idx >= doc->node_count) {
        return false;
    }
    seq = &doc->nodes[seq_idx];
    if (seq->type != YYAML_SEQUENCE) return false;
    if (!yyaml_doc_link_last(doc, seq_idx, child_idx)) return false;
    seq->val.integer++;
    return true;
}

YYAML_API bool yyaml_doc_map_append(yyaml_doc *doc, uint32_t map_idx,
                                    const char *key, size_t key_len,
                                    uint32_t val_idx) {
    yyaml_node *map;
    yyaml_node *val;
    uint32_t key_ofs = 0;
    if (!doc || !key || map_idx == YYAML_INDEX_NONE || val_idx == YYAML_INDEX_NONE) {
        return false;
    }
    if (map_idx >= doc->node_count || val_idx >= doc->node_count) return false;
    map = &doc->nodes[map_idx];
    if (map->type != YYAML_MAPPING) return false;
    if (!yyaml_doc_store_string(doc, key, key_len, &key_ofs)) return false;
    if (!yyaml_doc_link_last(doc, map_idx, val_idx)) return false;
    val = &doc->nodes[val_idx];
    val->flags = (uint32_t)key_len;
    val->extra = key_ofs;
    map->val.integer++;
    return true;
}

/* ------------------------------ writing ---------------------------------- */

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} yyaml_writer;

static bool yyaml_writer_reserve(yyaml_writer *wr, size_t need) {
    size_t cap;
    char *new_buf;
    if (wr->cap >= need) return true;
    cap = wr->cap ? wr->cap : 128;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) {
            cap = need;
            break;
        }
        cap *= 2;
    }
    new_buf = (char *)realloc(wr->buf, cap);
    if (!new_buf) return false;
    wr->buf = new_buf;
    wr->cap = cap;
    return true;
}

static bool yyaml_writer_ensure(yyaml_writer *wr, size_t extra) {
    size_t need;
    if (extra > SIZE_MAX - wr->len - 1) return false;
    need = wr->len + extra + 1; /* reserve space for null terminator */
    return yyaml_writer_reserve(wr, need);
}

static bool yyaml_writer_putc(yyaml_writer *wr, char c) {
    if (!yyaml_writer_ensure(wr, 1)) return false;
    wr->buf[wr->len++] = c;
    return true;
}

static bool yyaml_writer_write(yyaml_writer *wr, const char *str, size_t len) {
    if (!len) return true;
    if (!yyaml_writer_ensure(wr, len)) return false;
    memcpy(wr->buf + wr->len, str, len);
    wr->len += len;
    return true;
}

static bool yyaml_writer_indent(yyaml_writer *wr, size_t indent, size_t depth) {
    size_t total = indent * depth;
    size_t i;
    if (!total) return true;
    if (!yyaml_writer_ensure(wr, total)) return false;
    for (i = 0; i < total; i++) {
        wr->buf[wr->len++] = ' ';
    }
    return true;
}

static bool yyaml_writer_write_double(yyaml_writer *wr, double val) {
    char tmp[64];
    int len;
    if (isnan(val)) {
        return yyaml_writer_write(wr, "nan", 3);
    }
    if (isinf(val)) {
        if (val < 0) return yyaml_writer_write(wr, "-inf", 4);
        return yyaml_writer_write(wr, "inf", 3);
    }
    len = snprintf(tmp, sizeof(tmp), "%.17g", val);
    if (len < 0) return false;

    // bool has_decimal = false;
    // for (int i = 0; i < len; ++i) {
    //     char c = tmp[i];
    //     if (c == '.' || c == 'e' || c == 'E') {
    //         has_decimal = true;
    //         break;
    //     }
    // }

    // if (!has_decimal && len + 2 < (int)sizeof(tmp)) {
    //     tmp[len++] = '.';
    //     tmp[len++] = '0';
    //     tmp[len] = '\0';
    // }

    return yyaml_writer_write(wr, tmp, (size_t)len);
}

static bool yyaml_writer_write_int(yyaml_writer *wr, int64_t val) {
    char tmp[32];
    int len = snprintf(tmp, sizeof(tmp), "%lld", (long long)val);
    if (len < 0) return false;
    return yyaml_writer_write(wr, tmp, (size_t)len);
}

static bool yyaml_writer_is_plain_scalar(const char *str, size_t len) {
    size_t i;
    if (len == 0) return false;
    if (!(isalpha((unsigned char)str[0]) || str[0] == '_')) return false;
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        if (!(isalnum(c) || c == '_' || c == '-')) return false;
    }
    return true;
}

static bool yyaml_writer_write_string_literal(yyaml_writer *wr, const char *str,
                                              size_t len) {
    size_t i;
    if (yyaml_writer_is_plain_scalar(str, len)) {
        return yyaml_writer_write(wr, str, len);
    }
    if (!yyaml_writer_putc(wr, '"')) return false;
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        switch (c) {
        case '\\':
            if (!yyaml_writer_write(wr, "\\\\", 2)) return false;
            break;
        case '\"':
            if (!yyaml_writer_write(wr, "\\\"", 2)) return false;
            break;
        case '\n':
            if (!yyaml_writer_write(wr, "\\n", 2)) return false;
            break;
        case '\r':
            if (!yyaml_writer_write(wr, "\\r", 2)) return false;
            break;
        case '\t':
            if (!yyaml_writer_write(wr, "\\t", 2)) return false;
            break;
        default:
            if (c < 0x20) {
                char esc[5];
                esc[0] = '\\';
                esc[1] = 'x';
                esc[2] = "0123456789ABCDEF"[c >> 4];
                esc[3] = "0123456789ABCDEF"[c & 0xF];
                esc[4] = '\0';
                if (!yyaml_writer_write(wr, esc, 4)) return false;
            } else {
                if (!yyaml_writer_putc(wr, (char)c)) return false;
            }
            break;
        }
    }
    return yyaml_writer_putc(wr, '"');
}

static bool yyaml_writer_write_string_node(const yyaml_doc *doc,
                                           const yyaml_node *node,
                                           yyaml_writer *wr) {
    const char *buf = yyaml_doc_get_scalar_buf(doc);
    if (!buf) buf = "";
    return yyaml_writer_write_string_literal(
        wr, buf + node->val.str.ofs, node->val.str.len);
}

static bool yyaml_writer_write_key(const yyaml_doc *doc, const yyaml_node *node,
                                   yyaml_writer *wr) {
    const char *buf = yyaml_doc_get_scalar_buf(doc);
    if (!buf) buf = "";
    return yyaml_writer_write_string_literal(wr, buf + node->extra,
                                             node->flags);
}

static bool yyaml_write_node_internal(const yyaml_doc *doc,
                                      const yyaml_node *node,
                                      size_t depth, size_t indent,
                                      yyaml_writer *wr, yyaml_err *err);

static bool yyaml_writer_write_mapping(const yyaml_doc *doc,
                                       const yyaml_node *node, size_t depth,
                                       size_t indent, yyaml_writer *wr,
                                       yyaml_err *err, bool inline_first);

static bool yyaml_writer_write_sequence(const yyaml_doc *doc,
                                        const yyaml_node *node, size_t depth,
                                        size_t indent, yyaml_writer *wr,
                                        yyaml_err *err, bool inline_first) {
    uint32_t idx;
    bool first = true;
    if (!node || node->type != YYAML_SEQUENCE) return false;
    if (node->child == YYAML_INDEX_NONE) {
        return yyaml_writer_write(wr, "[]", 2);
    }
    idx = node->child;
    while (idx != YYAML_INDEX_NONE) {
        const yyaml_node *child = &doc->nodes[idx];
        if (!first) {
            if (!yyaml_writer_putc(wr, '\n')) return false;
        }
        if (!(inline_first && first)) {
            if (!yyaml_writer_indent(wr, indent, depth)) return false;
        }
        if (!yyaml_writer_write(wr, "- ", 2)) return false;
        if (child->type == YYAML_SEQUENCE) {
            if (!yyaml_writer_write_sequence(doc, child, depth + 1, indent, wr,
                                             err, true))
                return false;
        } else if (child->type == YYAML_MAPPING) {
            if (!yyaml_writer_write_mapping(doc, child, depth + 1, indent, wr,
                                            err, true))
                return false;
        } else {
            if (!yyaml_write_node_internal(doc, child, depth + 1, indent, wr,
                                           err))
                return false;
        }
        first = false;
        idx = child->next;
    }
    return true;
}

static bool yyaml_writer_write_mapping(const yyaml_doc *doc,
                                       const yyaml_node *node, size_t depth,
                                       size_t indent, yyaml_writer *wr,
                                       yyaml_err *err, bool inline_first) {
    uint32_t idx;
    bool first = true;
    if (!node || node->type != YYAML_MAPPING) return false;
    if (node->child == YYAML_INDEX_NONE) {
        return yyaml_writer_write(wr, "{}", 2);
    }
    idx = node->child;
    while (idx != YYAML_INDEX_NONE) {
        const yyaml_node *child = &doc->nodes[idx];
        if (!first) {
            if (!yyaml_writer_putc(wr, '\n')) return false;
        }
        if (!(inline_first && first)) {
            if (!yyaml_writer_indent(wr, indent, depth)) return false;
        }
        if (!yyaml_writer_write_key(doc, child, wr)) return false;
        if (child->type == YYAML_SEQUENCE || child->type == YYAML_MAPPING) {
            if (!yyaml_writer_write(wr, ":\n", 2)) return false;
            if (!yyaml_write_node_internal(doc, child, depth + 1, indent, wr,
                                           err))
                return false;
        } else {
            if (!yyaml_writer_write(wr, ": ", 2)) return false;
            if (!yyaml_write_node_internal(doc, child, depth + 1, indent, wr,
                                           err))
                return false;
        }
        first = false;
        idx = child->next;
    }
    return true;
}

static bool yyaml_write_node_internal(const yyaml_doc *doc,
                                      const yyaml_node *node,
                                      size_t depth, size_t indent,
                                      yyaml_writer *wr, yyaml_err *err) {
    (void)err;
    if (!node) return yyaml_writer_write(wr, "null", 4);
    switch (node->type) {
    case YYAML_NULL:
        return yyaml_writer_write(wr, "null", 4);
    case YYAML_BOOL:
        return yyaml_writer_write(wr, node->val.boolean ? "true" : "false",
                                  node->val.boolean ? 4 : 5);
    case YYAML_INT:
        return yyaml_writer_write_int(wr, node->val.integer);
    case YYAML_DOUBLE:
        return yyaml_writer_write_double(wr, node->val.real);
    case YYAML_STRING:
        return yyaml_writer_write_string_node(doc, node, wr);
    case YYAML_SEQUENCE:
        return yyaml_writer_write_sequence(doc, node, depth, indent, wr, err,
                                           false);
    case YYAML_MAPPING:
        return yyaml_writer_write_mapping(doc, node, depth, indent, wr, err,
                                          false);
    default:
        return false;
    }
}

YYAML_API bool yyaml_write(const yyaml_node *root, char **out, size_t *out_len,
                           const yyaml_write_opts *opts, yyaml_err *err) {
    yyaml_writer wr = {0};
    const yyaml_doc *doc = root ? root->doc : NULL;
    size_t indent = 2;
    bool final_newline = true;
    if (!out) {
        yyaml_set_error(err, 0, 0, 0, "invalid output buffer");
        return false;
    }
    *out = NULL;
    if (out_len) *out_len = 0;
    if (opts) {
        if (opts->indent) indent = opts->indent;
        final_newline = opts->final_newline;
    }
    if (root && !doc) {
        yyaml_set_error(err, 0, 0, 0, "node is not bound to a document");
        return false;
    }
    if (!root) {
        if (!yyaml_writer_write(&wr, "null", 4)) goto nomem;
    } else {
        if (!yyaml_write_node_internal(doc, root, 0, indent, &wr, err))
            goto nomem;
    }
    if (final_newline) {
        if (!yyaml_writer_putc(&wr, '\n')) goto nomem;
    }
    if (!yyaml_writer_ensure(&wr, 0)) goto nomem;
    wr.buf[wr.len] = '\0';
    *out = wr.buf;
    if (out_len) *out_len = wr.len;
    return true;
nomem:
    yyaml_set_error(err, 0, 0, 0, "out of memory");
    free(wr.buf);
    return false;
}

YYAML_API void yyaml_free_string(char *str) {
    free(str);
}
