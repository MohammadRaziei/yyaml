#pragma once

#include "yyaml.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace yyaml {

/** Alias to the C yyaml node type enumeration. */
using type = ::yyaml_type;
/** Alias to the C yyaml read options structure. */
using read_opts = ::yyaml_read_opts;
/** Alias to the C yyaml write options structure. */
using write_opts = ::yyaml_write_opts;

class document;
class node_iterator;
class const_node_iterator;

/**
 * @brief Exception type thrown by the C++ yyaml wrapper.
 *
 * Captures position, line, and column when raised from the underlying C API so
 * callers can report detailed parsing or serialization errors.
 */
struct yyaml_error : public std::runtime_error {
    std::size_t pos = 0;
    std::size_t line = 0;
    std::size_t column = 0;

    explicit yyaml_error(const std::string &msg)
        : std::runtime_error(msg) {}

    explicit yyaml_error(const ::yyaml_err &err)
        : std::runtime_error(err.msg), pos(err.pos), line(err.line), column(err.column) {}
};

/**
 * @brief Lightweight view over a YAML node owned by a yyaml::document.
 *
 * Instances are cheap to copy and behave like non-owning references to nodes
 * stored in the document tree. Operations that read or traverse nodes validate
 * their type and throw yyaml_error when misused.
 */
class node {
public:
    node() = default;
    ~node() {}

    /** @return The node type or YYAML_NULL if unbound. */
    type get_type() const { return _node ? static_cast<type>(_node->type) : YYAML_NULL; }

    /** @return True when the node points to data inside a document. */
    bool is_valid() const { return _node != nullptr; }
    explicit operator bool() const { return is_valid(); }

    bool is_null() const { return _node && _node->type == YYAML_NULL; }
    bool is_bool() const { return _node && _node->type == YYAML_BOOL; }
    bool is_int() const { return _node && _node->type == YYAML_INT; }
    bool is_double() const { return _node && _node->type == YYAML_DOUBLE; }
    bool is_number() const { return is_int() || is_double(); }
    bool is_string() const { return _node && _node->type == YYAML_STRING; }
    bool is_sequence() const { return _node && _node->type == YYAML_SEQUENCE; }
    bool is_mapping() const { return _node && _node->type == YYAML_MAPPING; }

    bool is_scalar() const { return _node && yyaml_is_scalar(_node); }
    bool is_container() const { return _node && yyaml_is_container(_node); }

    /** @name Conversions */
    ///@{
    std::nullptr_t as_null() const;
    bool as_bool() const;
    std::int64_t as_int() const;
    double as_double() const;
    double as_number() const;
    std::string as_string() const;
    std::string to_string() const;
    /** @return Index of the node within its owning document. */
    uint32_t index() const;
    ///@}

    /** @name Child access */
    ///@{
    node at(const std::string &key) const;
    node at(std::size_t index) const;
    node operator[](const std::string &key) const;
    node operator[](std::size_t index) const;
    ///@}

    /** @return Number of children for sequences/maps or zero otherwise. */
    std::size_t size() const;
    /** @return True when the node is empty or unbound. */
    bool empty() const;

    /**
     * @brief Iterate mapping entries without allocating intermediate pairs.
     *
     * The provided callback receives `(std::string key, node value)` for each
     * member in insertion order. Throws if the node is not a mapping.
     */
    template <typename Func>
    void foreach(Func &&func) const;

    /** @return A forward iterator over child nodes. */
    node_iterator iter();
    /** @return A forward iterator over child nodes for const contexts. */
    const_node_iterator iter() const;

private:
    const ::yyaml_node *_node = nullptr;

    explicit node(const ::yyaml_node *node) : _node(node) {}

    friend class document;
    friend class node_iterator;
    friend class const_node_iterator;

    void require_bound() const;
    void require_scalar() const;
    std::string read_string() const;
};

/**
 * @brief Forward-only iterator over the children of a yyaml::node.
 *
 * The iterator yields `node*` values; it returns `nullptr` after the final
 * child, making it convenient for pointer-based loops:
 *
 * @code
 * for (auto it = parent.iter(); node *child = it.next(); ) {
 *     // use child
 * }
 * @endcode
 */
class node_iterator {
public:
    node_iterator() = default;
    explicit node_iterator(const node &parent);

    /** @return Pointer to the next child node or nullptr after the last. */
    node *next();

private:
    static constexpr uint32_t none = std::numeric_limits<uint32_t>::max();

    const ::yyaml_doc *_doc = nullptr;
    uint32_t _next_idx = none;
    node _current;
};

/**
 * @brief Const-qualified variant of node_iterator.
 *
 * Returned from `const node::iter()` to allow traversal from const contexts
 * while still yielding `const node*` pointers.
 */
class const_node_iterator {
public:
    const_node_iterator() = default;
    explicit const_node_iterator(const node &parent);

    /** @return Pointer to the next child node or nullptr after the last. */
    const node *next();

private:
    static constexpr uint32_t none = std::numeric_limits<uint32_t>::max();

    const ::yyaml_doc *_doc = nullptr;
    uint32_t _next_idx = none;
    node _current;
};

/**
 * @brief Owning handle for a parsed YAML document.
 *
 * Use parse() or parse_file() to construct, then access the root node via
 * root(). Documents are movable but not copyable, mirroring ownership of the
 * underlying yyaml_doc.
 */
class document {
public:
    document() = default;
    ~document() {
        yyaml_doc_free(_doc);
    }
    explicit document(::yyaml_doc *doc) : _doc(doc) {}

    document(document &&other) noexcept : _doc(other._doc) { other._doc = nullptr; }
    document &operator=(document &&other) noexcept {
        if (this != &other) {
            yyaml_doc_free(_doc);
            _doc = other._doc;
            other._doc = nullptr;
        }
        return *this;
    }

    document(const document &) = delete;
    document &operator=(const document &) = delete;

    /**
     * @brief Parse YAML from memory.
     *
     * @param yaml UTF-8 YAML text.
     * @param opts Optional parser configuration.
     * @throws yyaml_error on failure with position information populated.
     */
    static document parse(std::string_view yaml, const read_opts *opts = nullptr) {
        ::yyaml_err err = {0};
        ::yyaml_doc *doc = yyaml_read(yaml.data(), yaml.size(), opts, &err);
        if (!doc) {
            throw yyaml_error(err);
        }
        return document(doc);
    }

    /**
     * @brief Allocate an empty document for manual construction.
     *
     * Caller is responsible for using the builder helpers to populate nodes
     * and assign a root.
     */
    static document create() {
        ::yyaml_doc *doc = yyaml_doc_new();
        if (!doc) {
            throw yyaml_error("failed to allocate yyaml document");
        }
        return document(doc);
    }

    /**
     * @brief Parse a YAML document from disk.
     *
     * @param path Path to the YAML file.
     * @param opts Optional parser configuration.
     * @throws yyaml_error if the file cannot be opened or parsing fails.
     */
    static document parse_file(const std::string &path, const read_opts *opts = nullptr) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            throw yyaml_error("unable to open YAML file: " + path);
        }
        std::string data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        return parse(data, opts);
    }

    /** @return The root node or an invalid node when the document is empty. */
    node root() const {
        if (!_doc) {
            return {};
        }
        return node(yyaml_doc_get_root(_doc));
    }

    /** @name Builder helpers */
    ///@{
    node add_null();
    node add_bool(bool value);
    node add_int(std::int64_t value);
    node add_double(double value);
    node add_string(const std::string &value);
    node add_sequence();
    node add_mapping();
    void set_root(const node &root);
    void seq_append(const node &seq, const node &child);
    void map_append(const node &map, const std::string &key, const node &value);
    ///@}

    /** @return Whether the underlying document pointer is initialized. */
    bool valid() const { return static_cast<bool>(_doc); }
    /** Serialize the document to YAML text. */
    std::string dump(const write_opts *opts = nullptr) const;

private:
    ::yyaml_doc* _doc = nullptr;
    friend class node;

    void require_doc() const;
    node node_from_index(uint32_t idx) const;
    uint32_t index_of(const node &n) const;
};

inline node node::at(const std::string &key) const {
    require_bound();
    if (!is_mapping()) {
        throw yyaml_error("yyaml::node only work at mapping type");
    }
    const ::yyaml_node *child = yyaml_map_get(_node, key.c_str());
    return node(child);
}

inline node node::at(std::size_t index) const {
    require_bound();
    if (!is_sequence()) {
        throw yyaml_error("yyaml::node only work at sequence type");
    }
    const ::yyaml_node *child = yyaml_seq_get(_node, index);
    return node(child);
}

inline node node::operator[](const std::string &key) const {
    return at(key);
}

inline node node::operator[](std::size_t index) const {
    return at(index);
}

inline std::size_t node::size() const {
    if (!_node) {
        return 0;
    }
    if (is_sequence()) {
        return yyaml_seq_len(_node);
    }
    if (is_mapping()) {
        return yyaml_map_len(_node);
    }
    return 0;
}

inline bool node::empty() const {
    if (!_node) {
        return true;
    }
    switch (_node->type) {
    case YYAML_NULL:
        return true;
    case YYAML_SEQUENCE:
        return yyaml_seq_len(_node) == 0;
    case YYAML_MAPPING:
        return yyaml_map_len(_node) == 0;
    case YYAML_STRING:
        return _node->val.str.len == 0;
    default:
        return false;
    }
}

template <typename Func>
inline void node::foreach(Func &&func) const {
    require_bound();
    if (!is_mapping()) {
        throw yyaml_error("yyaml::node is not a mapping");
    }

    const ::yyaml_doc *doc = _node->doc;
    const char *buf = yyaml_doc_get_scalar_buf(doc);
    if (!buf) {
        throw yyaml_error("yyaml scalar buffer is null");
    }
    const auto none = std::numeric_limits<uint32_t>::max();
    const ::yyaml_node *child = yyaml_doc_get(doc, _node->child);

    while (child) {
        std::string key;
        key.assign(buf + static_cast<std::size_t>(child->extra),
                   static_cast<std::size_t>(child->flags));

        func(key, node(child));

        if (child->next == none) {
            break;
        }
        child = yyaml_doc_get(doc, child->next);
    }
}

inline node_iterator node::iter() {
    return node_iterator(*this);
}

inline const_node_iterator node::iter() const {
    return const_node_iterator(*this);
}

inline node_iterator::node_iterator(const node &parent) {
    if (!parent._node || !yyaml_is_container(parent._node)) {
        return;
    }
    _doc = parent._node->doc;
    _next_idx = parent._node->child;
}

inline node *node_iterator::next() {
    if (!_doc || _next_idx == none) {
        return nullptr;
    }

    const ::yyaml_node *raw = yyaml_doc_get(_doc, _next_idx);
    _current = node(raw);
    _next_idx = raw->next == none ? none : raw->next;
    return &_current;
}

inline const_node_iterator::const_node_iterator(const node &parent) {
    if (!parent._node || !yyaml_is_container(parent._node)) {
        return;
    }
    _doc = parent._node->doc;
    _next_idx = parent._node->child;
}

inline const node *const_node_iterator::next() {
    if (!_doc || _next_idx == none) {
        return nullptr;
    }

    const ::yyaml_node *raw = yyaml_doc_get(_doc, _next_idx);
    _current = node(raw);
    _next_idx = raw->next == none ? none : raw->next;
    return &_current;
}

inline std::nullptr_t node::as_null() const {
    require_bound();
    if (!is_null()) {
        throw yyaml_error("yyaml::node is not null");
    }
    return nullptr;
}

inline bool node::as_bool() const {
    require_bound();
    if (!is_bool()) {
        throw yyaml_error("yyaml::node is not a bool");
    }
    return _node->val.boolean;
}

inline std::int64_t node::as_int() const {
    require_bound();
    if (!is_int()) {
        throw yyaml_error("yyaml::node is not an integer");
    }
    return _node->val.integer;
}

inline double node::as_double() const {
    require_bound();
    if (!is_double()) {
        throw yyaml_error("yyaml::node is not a double");
    }
    return _node->val.real;
}

inline double node::as_number() const {
    require_scalar();
    if (is_int()) {
        return static_cast<double>(as_int());
    }
    if (is_double()) {
        return as_double();
    }
    throw yyaml_error("yyaml::node is not numeric");
}

inline std::string node::as_string() const {
    return read_string();
}

inline std::string node::to_string() const {
    require_bound();

    char *buffer = nullptr;
    size_t len = 0;
    ::yyaml_err err = {0};

    ::yyaml_write_opts opts{};
    opts.indent = 2;
    opts.final_newline = false;

    if (!yyaml_write(_node, &buffer, &len, &opts, &err)) {
        throw yyaml_error(err);
    }

    std::string out(buffer, len);
    yyaml_free_string(buffer);
    return out;
}

inline uint32_t node::index() const {
    require_bound();
    const uint32_t idx = yyaml_node_index(_node->doc, _node);
    if (idx == std::numeric_limits<uint32_t>::max()) {
        throw yyaml_error("yyaml::node index lookup failed");
    }
    return idx;
}

inline void document::require_doc() const {
    if (!_doc) {
        throw yyaml_error("yyaml::document is not initialized");
    }
}

inline node document::node_from_index(uint32_t idx) const {
    require_doc();
    const ::yyaml_node *raw = yyaml_doc_get(_doc, idx);
    if (!raw) {
        throw yyaml_error("invalid yyaml node index");
    }
    return node(raw);
}

inline uint32_t document::index_of(const node &n) const {
    if (!n._node || n._node->doc != _doc) {
        throw yyaml_error("yyaml::node belongs to a different document");
    }
    const uint32_t idx = yyaml_node_index(_doc, n._node);
    if (idx == std::numeric_limits<uint32_t>::max()) {
        throw yyaml_error("yyaml::node index lookup failed");
    }
    return idx;
}

inline node document::add_null() {
    require_doc();
    const uint32_t idx = yyaml_doc_add_null(_doc);
    if (idx == std::numeric_limits<uint32_t>::max()) {
        throw yyaml_error("failed to allocate null node");
    }
    return node_from_index(idx);
}

inline node document::add_bool(bool value) {
    require_doc();
    const uint32_t idx = yyaml_doc_add_bool(_doc, value);
    if (idx == std::numeric_limits<uint32_t>::max()) {
        throw yyaml_error("failed to allocate bool node");
    }
    return node_from_index(idx);
}

inline node document::add_int(std::int64_t value) {
    require_doc();
    const uint32_t idx = yyaml_doc_add_int(_doc, value);
    if (idx == std::numeric_limits<uint32_t>::max()) {
        throw yyaml_error("failed to allocate int node");
    }
    return node_from_index(idx);
}

inline node document::add_double(double value) {
    require_doc();
    const uint32_t idx = yyaml_doc_add_double(_doc, value);
    if (idx == std::numeric_limits<uint32_t>::max()) {
        throw yyaml_error("failed to allocate double node");
    }
    return node_from_index(idx);
}

inline node document::add_string(const std::string &value) {
    require_doc();
    const uint32_t idx = yyaml_doc_add_string(_doc, value.data(), value.size());
    if (idx == std::numeric_limits<uint32_t>::max()) {
        throw yyaml_error("failed to allocate string node");
    }
    return node_from_index(idx);
}

inline node document::add_sequence() {
    require_doc();
    const uint32_t idx = yyaml_doc_add_sequence(_doc);
    if (idx == std::numeric_limits<uint32_t>::max()) {
        throw yyaml_error("failed to allocate sequence node");
    }
    return node_from_index(idx);
}

inline node document::add_mapping() {
    require_doc();
    const uint32_t idx = yyaml_doc_add_mapping(_doc);
    if (idx == std::numeric_limits<uint32_t>::max()) {
        throw yyaml_error("failed to allocate mapping node");
    }
    return node_from_index(idx);
}

inline void document::set_root(const node &root) {
    const uint32_t idx = index_of(root);
    if (!yyaml_doc_set_root(_doc, idx)) {
        throw yyaml_error("failed to set document root");
    }
}

inline void document::seq_append(const node &seq, const node &child) {
    if (!seq.is_sequence()) {
        throw yyaml_error("yyaml::node is not a sequence");
    }
    const uint32_t seq_idx = index_of(seq);
    const uint32_t child_idx = index_of(child);
    if (!yyaml_doc_seq_append(_doc, seq_idx, child_idx)) {
        throw yyaml_error("failed to append child to sequence");
    }
}

inline void document::map_append(const node &map, const std::string &key, const node &value) {
    if (!map.is_mapping()) {
        throw yyaml_error("yyaml::node is not a mapping");
    }
    const uint32_t map_idx = index_of(map);
    const uint32_t val_idx = index_of(value);
    if (!yyaml_doc_map_append(_doc, map_idx, key.data(), key.size(), val_idx)) {
        throw yyaml_error("failed to append mapping entry");
    }
}

inline void node::require_bound() const {
    if (!_node || !_node->doc) {
        throw yyaml_error("yyaml::node is not bound to a document");
    }
}

inline void node::require_scalar() const {
    require_bound();
    if (!is_scalar()) {
        throw yyaml_error("yyaml::node is not a scalar");
    }
}

inline std::string node::read_string() const {
    require_bound();
    if (!is_string()) {
        throw yyaml_error("yyaml::node is not a string");
    }
    const char *buf = yyaml_doc_get_scalar_buf(_node->doc);
    if (!buf) {
        throw yyaml_error("yyaml scalar buffer is null");
    }
    return std::string(buf + _node->val.str.ofs, _node->val.str.len);
}

inline std::string document::dump(const write_opts *opts) const {
    if (!_doc) {
        throw yyaml_error("yyaml::document is not initialized");
    }

    char *buffer = nullptr;
    size_t len = 0;
    ::yyaml_err err = {0};
    const ::yyaml_node *root = yyaml_doc_get_root(_doc);

    if (!yyaml_write(root, &buffer, &len, opts, &err)) {
        throw yyaml_error(err);
    }

    std::string out(buffer, len);
    yyaml_free_string(buffer);
    return out;
}

} // namespace yyaml

