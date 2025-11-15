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

using type = ::yyaml_type;
using read_opts = ::yyaml_read_opts;
using write_opts = ::yyaml_write_opts;

class document;

struct yyaml_error : public std::runtime_error {
    std::size_t pos = 0;
    std::size_t line = 0;
    std::size_t column = 0;

    explicit yyaml_error(const std::string &msg)
        : std::runtime_error(msg) {}

    explicit yyaml_error(const ::yyaml_err &err)
        : std::runtime_error(err.msg), pos(err.pos), line(err.line), column(err.column) {}
};

class node {
public:
    node() = default;
    ~node() {}

    type get_type() const { return _node ? static_cast<type>(_node->type) : YYAML_NULL; }

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

    std::nullptr_t as_null() const;
    bool as_bool() const;
    std::int64_t as_int() const;
    double as_double() const;
    double as_number() const;
    std::string as_string() const;
    std::string to_string() const;

    node operator[](const std::string &key) const;
    node at(std::size_t index) const;

    std::size_t size() const;

    template <typename Func>
    void for_each_member(Func &&func) const;

private:
    const ::yyaml_doc *_doc = nullptr;
    const ::yyaml_node *_node = nullptr;

    node(const ::yyaml_doc *doc, const ::yyaml_node *node) : _doc(doc), _node(node) {}

    friend class document;

    void require_bound() const;
    void require_scalar() const;
    std::string read_string() const;
};

class document {
public:
    document() = default;
    ~document() {
        if (_doc) {
            yyaml_doc_free(_doc);
        }
    }
    explicit document(::yyaml_doc *doc) : _doc(doc) {}

    document(document &&) noexcept = default;
    document &operator=(document &&) noexcept = default;

    document(const document &) = delete;
    document &operator=(const document &) = delete;

    static document parse(std::string_view yaml, const read_opts *opts = nullptr) {
        ::yyaml_err err = {0};
        ::yyaml_doc *doc = yyaml_read(yaml.data(), yaml.size(), opts, &err);
        if (!doc) {
            throw yyaml_error(err);
        }
        return document(doc);
    }

    static document parse_file(const std::string &path, const read_opts *opts = nullptr) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            throw yyaml_error("unable to open YAML file: " + path);
        }
        std::string data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        return parse(data, opts);
    }

    node root() const {
        if (!_doc) {
            return {};
        }
        return node(_doc, yyaml_doc_get_root(_doc));
    }


    bool valid() const { return static_cast<bool>(_doc); }

    std::string dump(const write_opts *opts = nullptr) const;

private:


    ::yyaml_doc* _doc {nullptr};

    friend class node;
};

inline node node::operator[](const std::string &key) const {
    if (!_doc || !_node || !is_mapping()) {
        return {};
    }
    const ::yyaml_node *child = yyaml_map_get(_doc, _node, key.c_str());
    return node(_doc, child);
}

inline node node::at(std::size_t index) const {
    if (!_doc || !_node || !is_sequence()) {
        return {};
    }
    const ::yyaml_node *child = yyaml_seq_get(_doc, _node, index);
    return node(_doc, child);
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

template <typename Func>
inline void node::for_each_member(Func &&func) const {
    require_bound();
    if (!is_mapping()) {
        throw yyaml_error("yyaml::node is not a mapping");
    }

    const char *buf = yyaml_doc_get_scalar_buf(_doc);
    if (!buf) {
        throw yyaml_error("yyaml scalar buffer is null");
    }
    const auto none = std::numeric_limits<uint32_t>::max();
    const ::yyaml_node *child = yyaml_doc_get(_doc, _node->child);

    while (child) {
        std::string key;
        key.assign(buf + static_cast<std::size_t>(child->extra),
                   static_cast<std::size_t>(child->flags));

        func(key, node(_doc, child));

        if (child->next == none) {
            break;
        }
        child = yyaml_doc_get(_doc, child->next);
    }
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
    require_scalar();
    switch (_node->type) {
    case YYAML_NULL:
        return "null";
    case YYAML_BOOL:
        return _node->val.boolean ? "true" : "false";
    case YYAML_INT:
        return std::to_string(_node->val.integer);
    case YYAML_DOUBLE:
        return std::to_string(_node->val.real);
    case YYAML_STRING:
        return read_string();
    default:
        throw yyaml_error("yyaml::node holds an unknown scalar type");
    }
}

inline void node::require_bound() const {
    if (!_doc || !_node) {
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
    const char *buf = yyaml_doc_get_scalar_buf(_doc);
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

    if (!yyaml_write(_doc, &buffer, &len, opts, &err)) {
        throw yyaml_error(err);
    }

    std::string out(buffer, len);
    yyaml_free_string(buffer);
    return out;
}

} // namespace yyaml

