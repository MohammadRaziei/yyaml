#ifndef YYAML_H
#define YYAML_H

/**
 * @file yyaml.h
 * @brief C API for the yyaml parser and writer.
 *
 * yyaml implements a fast, DOM-style YAML parser for a pragmatic subset of
 * YAML 1.2 that mirrors JSON features together with basic indentation-based
 * collections. It is designed to be completely standalone, requiring only the
 * C standard library. The API follows the general style of yyjson so existing
 * yyjson users can get started quickly.
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

/**
 * @brief Node kinds that can appear in a YAML document.
 */
typedef enum yyaml_type {
    YYAML_NULL = 0, /**< YAML null */
    YYAML_BOOL,     /**< Boolean value */
    YYAML_INT,      /**< 64-bit signed integer */
    YYAML_DOUBLE,   /**< Floating-point number */
    YYAML_STRING,   /**< UTF-8 string */
    YYAML_SEQUENCE, /**< YAML sequence (array) */
    YYAML_MAPPING   /**< YAML mapping (object) */
} yyaml_type;

typedef struct yyaml_doc yyaml_doc;

/**
 * @brief A single node stored inside a yyaml_doc.
 */
typedef struct yyaml_node {
    const yyaml_doc *doc; /**< owning document */
    uint32_t type;        /**< yyaml_type */
    uint32_t flags;       /**< reserved for future use */
    uint32_t parent;      /**< index of parent node, UINT32_MAX if none */
    uint32_t next;        /**< next sibling index, UINT32_MAX if none */
    uint32_t child;       /**< index of first child (for sequence/mapping) */
    uint32_t extra;       /**< used for mapping: index of key scalar */
    union {
        bool boolean; /**< YYAML_BOOL */
        int64_t integer; /**< YYAML_INT */
        double real; /**< YYAML_DOUBLE */
        struct {
            uint32_t ofs; /**< offset into shared scalar buffer */
            uint32_t len; /**< length in bytes */
        } str; /**< YYAML_STRING */
    } val; /**< Node payload */
} yyaml_node;


/**
 * @brief Parser configuration parameters.
 */
typedef struct yyaml_read_opts {
    bool allow_duplicate_keys;   /**< keep last value when duplicates appear */
    bool allow_trailing_content; /**< ignore trailing non-empty content */
    bool allow_inf_nan;          /**< parse inf/nan literals */
    size_t max_nesting;          /**< maximum indentation nesting depth */
} yyaml_read_opts;

/**
 * @brief Error details returned by the parser and writer.
 */
typedef struct yyaml_err {
    size_t pos;      /**< byte offset */
    size_t line;     /**< 1-based line number */
    size_t column;   /**< 1-based column number */
    char msg[96];    /**< error message */
} yyaml_err;

/**
 * @brief Serialization options for yyaml_write.
 */
typedef struct yyaml_write_opts {
    size_t indent;        /**< spaces per indentation level, default 2 */
    bool final_newline;   /**< append trailing newline (default true) */
} yyaml_write_opts;

/* ---------------------------- reading API -------------------------------- */

/**
 * @brief Parse YAML text into a document tree.
 *
 * @param data UTF-8 YAML buffer.
 * @param len Buffer length in bytes.
 * @param opts Optional parser configuration, may be NULL for defaults.
 * @param err Output error details on failure, may be NULL to ignore.
 * @return Allocated document on success or NULL on failure.
 */
YYAML_API yyaml_doc *yyaml_read(const char *data, size_t len,
                                const yyaml_read_opts *opts,
                                yyaml_err *err);

/** @brief Allocate an empty document for manual construction. */
YYAML_API yyaml_doc *yyaml_doc_new(void);

/** @brief Free a document returned by yyaml_read. */
YYAML_API void yyaml_doc_free(yyaml_doc *doc);

/** @brief Retrieve the root node of a document. */
YYAML_API const yyaml_node *yyaml_doc_get_root(const yyaml_doc *doc);

/** @brief Fetch a node by index within the document pool. */
YYAML_API const yyaml_node *yyaml_doc_get(const yyaml_doc *doc, uint32_t idx);

/** @brief Compute the index of a node within its owning document. */
YYAML_API uint32_t yyaml_node_index(const yyaml_doc *doc, const yyaml_node *node);

/** @brief Access the shared scalar buffer backing string nodes. */
YYAML_API const char *yyaml_doc_get_scalar_buf(const yyaml_doc *doc);

/** @brief Total number of nodes allocated within a document. */
YYAML_API size_t yyaml_doc_node_count(const yyaml_doc *doc);

/* -------------------------- convenience helpers -------------------------- */

/** @brief True when the node is a scalar type (null, bool, int, double, string). */
YYAML_INLINE bool yyaml_is_scalar(const yyaml_node *node) {
    return node && (node->type <= YYAML_STRING);
}

/** @brief True when the node is a sequence or mapping. */
YYAML_INLINE bool yyaml_is_container(const yyaml_node *node) {
    return node && (node->type == YYAML_SEQUENCE || node->type == YYAML_MAPPING);
}

/** @brief Compare a string node against a C string literal. */
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

/** @brief Look up a mapping value by key. */
YYAML_API const yyaml_node *yyaml_map_get(const yyaml_node *map,
                                          const char *key);

/** @brief Retrieve a sequence element by index. */
YYAML_API const yyaml_node *yyaml_seq_get(const yyaml_node *seq,
                                          size_t index);

/** @brief Number of elements inside a sequence. */
YYAML_API size_t yyaml_seq_len(const yyaml_node *seq);

/** @brief Number of members inside a mapping. */
YYAML_API size_t yyaml_map_len(const yyaml_node *map);

/* --------------------------- building API ------------------------------- */

/** @brief Set the document root node by index. */
YYAML_API bool yyaml_doc_set_root(yyaml_doc *doc, uint32_t idx);

/** @brief Create a null node and return its index, or UINT32_MAX on failure. */
YYAML_API uint32_t yyaml_doc_add_null(yyaml_doc *doc);

/** @brief Create a boolean node and return its index, or UINT32_MAX on failure. */
YYAML_API uint32_t yyaml_doc_add_bool(yyaml_doc *doc, bool value);

/** @brief Create an integer node and return its index, or UINT32_MAX on failure. */
YYAML_API uint32_t yyaml_doc_add_int(yyaml_doc *doc, int64_t value);

/** @brief Create a double node and return its index, or UINT32_MAX on failure. */
YYAML_API uint32_t yyaml_doc_add_double(yyaml_doc *doc, double value);

/** @brief Create a string node and return its index, or UINT32_MAX on failure. */
YYAML_API uint32_t yyaml_doc_add_string(yyaml_doc *doc, const char *str,
                                        size_t len);

/** @brief Create a sequence node and return its index, or UINT32_MAX on failure. */
YYAML_API uint32_t yyaml_doc_add_sequence(yyaml_doc *doc);

/** @brief Create a mapping node and return its index, or UINT32_MAX on failure. */
YYAML_API uint32_t yyaml_doc_add_mapping(yyaml_doc *doc);

/** @brief Append a child to a sequence. */
YYAML_API bool yyaml_doc_seq_append(yyaml_doc *doc, uint32_t seq_idx,
                                    uint32_t child_idx);

/** @brief Append a key/value pair to a mapping. */
YYAML_API bool yyaml_doc_map_append(yyaml_doc *doc, uint32_t map_idx,
                                    const char *key, size_t key_len,
                                    uint32_t val_idx);

/* --------------------------- writing API --------------------------------- */

/**
 * @brief Serialize a node tree to YAML text.
 *
 * @param root Root node to serialize.
 * @param out Output buffer pointer set to allocated YAML text on success.
 * @param out_len Length of the returned buffer.
 * @param opts Optional writer configuration, may be NULL.
 * @param err Output error details on failure, may be NULL.
 * @return true on success, false on failure.
 */
YYAML_API bool yyaml_write(const yyaml_node *root, char **out, size_t *out_len,
                           const yyaml_write_opts *opts, yyaml_err *err);

/** @brief Free buffers returned by yyaml_write. */
YYAML_API void yyaml_free_string(char *str);

#ifdef __cplusplus
}
#endif

#endif /* YYAML_H */
