#ifndef YYAML_H
#define YYAML_H

/*
 * yyaml - A lightweight, zero-dependency YAML reader inspired by yyjson.
 *
 * This library implements a fast, DOM-style YAML parser for a pragmatic
 * subset of YAML 1.2 that mirrors JSON features together with basic
 * indentation-based collections. It is designed to be completely
 * standalone, requiring only the C standard library. The API follows the
 * general style of yyjson so existing yyjson users can get started quickly.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef YYAML_API
#    if defined(_WIN32) && defined(YYAML_SHARED)
#        ifdef YYAML_BUILD
#            define YYAML_API __declspec(dllexport)
#        else
#            define YYAML_API __declspec(dllimport)
#        endif
#    else
#        define YYAML_API
#    endif
#endif

#ifndef YYAML_INLINE
#    if defined(__GNUC__) || defined(__clang__)
#        define YYAML_INLINE static inline __attribute__((unused))
#    else
#        define YYAML_INLINE static inline
#    endif
#endif

/* -------------------------- configuration flags -------------------------- */

/* Configure the initial capacity of the node pool. */
#ifndef YYAML_NODE_CAP_INIT
#    define YYAML_NODE_CAP_INIT 64
#endif

/* Configure the initial capacity of the scalar string buffer. */
#ifndef YYAML_STR_CAP_INIT
#    define YYAML_STR_CAP_INIT 256
#endif

/* ----------------------------- public types ------------------------------ */

typedef enum yyaml_type {
    YYAML_NULL = 0,
    YYAML_BOOL,
    YYAML_INT,
    YYAML_DOUBLE,
    YYAML_STRING,
    YYAML_SEQUENCE,
    YYAML_MAPPING
} yyaml_type;

typedef struct yyaml_doc yyaml_doc;

typedef struct yyaml_node {
    uint32_t type;        /* yyaml_type */
    uint32_t flags;       /* reserved for future use */
    uint32_t parent;      /* index of parent node, UINT32_MAX if none */
    uint32_t next;        /* next sibling index, UINT32_MAX if none */
    uint32_t child;       /* index of first child (for sequence/mapping) */
    uint32_t extra;       /* used for mapping: index of key scalar */
    union {
        bool boolean;
        int64_t integer;
        double real;
        struct {
            uint32_t ofs; /* offset into shared scalar buffer */
            uint32_t len; /* length in bytes */
        } str;
    } val;
} yyaml_node;


typedef struct yyaml_read_opts {
    bool allow_duplicate_keys;   /* keep last value when duplicates appear */
    bool allow_trailing_content; /* ignore trailing non-empty content */
    bool allow_inf_nan;          /* parse inf/nan literals */
    size_t max_nesting;          /* fail when indentation nesting exceeds */
} yyaml_read_opts;

typedef struct yyaml_err {
    size_t pos;      /* byte offset */
    size_t line;     /* 1-based line number */
    size_t column;   /* 1-based column number */
    char msg[96];    /* error message */
} yyaml_err;

typedef struct yyaml_write_opts {
    size_t indent;        /* spaces per indentation level, default 2 */
    bool final_newline;   /* set true to append trailing newline (default true) */
} yyaml_write_opts;

/* ---------------------------- reading API -------------------------------- */

YYAML_API yyaml_doc *yyaml_read(const char *data, size_t len,
                                const yyaml_read_opts *opts,
                                yyaml_err *err);

YYAML_API void yyaml_doc_free(yyaml_doc *doc);

YYAML_API const yyaml_node *yyaml_doc_get_root(const yyaml_doc *doc);

YYAML_API const yyaml_node *yyaml_doc_get(const yyaml_doc *doc, uint32_t idx);

YYAML_API const char *yyaml_doc_get_scalar_buf(const yyaml_doc *doc);

YYAML_API size_t yyaml_doc_node_count(const yyaml_doc *doc);

/* -------------------------- convenience helpers -------------------------- */

YYAML_INLINE bool yyaml_is_scalar(const yyaml_node *node) {
    return node && (node->type <= YYAML_STRING);
}

YYAML_INLINE bool yyaml_is_container(const yyaml_node *node) {
    return node && (node->type == YYAML_SEQUENCE || node->type == YYAML_MAPPING);
}

YYAML_INLINE bool yyaml_str_eq(const yyaml_doc *doc, const yyaml_node *node,
                               const char *str) {
    const char *buf;
    size_t i;
    if (!node || node->type != YYAML_STRING || !str) return false;
    buf = yyaml_doc_get_scalar_buf(doc);
    if (!buf) return false;
    if (node->val.str.len != (uint32_t)strlen(str)) return false;
    for (i = 0; i < node->val.str.len; i++) {
        if (buf[node->val.str.ofs + i] != str[i]) return false;
    }
    return true;
}

YYAML_API const yyaml_node *yyaml_map_get(const yyaml_doc *doc,
                                          const yyaml_node *map,
                                          const char *key);

YYAML_API const yyaml_node *yyaml_seq_get(const yyaml_doc *doc,
                                          const yyaml_node *seq,
                                          size_t index);

YYAML_API size_t yyaml_seq_len(const yyaml_node *seq);

YYAML_API size_t yyaml_map_len(const yyaml_node *map);

/* --------------------------- writing API --------------------------------- */

YYAML_API bool yyaml_write(const yyaml_doc *doc, char **out, size_t *out_len,
                           const yyaml_write_opts *opts, yyaml_err *err);


YYAML_API void yyaml_free_string(char *str);

#ifdef __cplusplus
}
#endif

#endif /* YYAML_H */
