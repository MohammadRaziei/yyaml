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

struct error : public std::runtime_error {
    std::size_t pos = 0;
    std::size_t line = 0;
    std::size_t column = 0;

    explicit error(const std::string &msg)
        : std::runtime_error(msg) {}

    explicit error(const ::yyaml_err &err)
        : std::runtime_error(err.msg), pos(err.pos), line(err.line), column(err.column) {}
};

class node {
public:
    node() = default;

    type get_type() const { return node_ ? static_cast<type>(node_->type) : YYAML_NULL; }

    bool is_valid() const { return node_ != nullptr; }
    explicit operator bool() const { return is_valid(); }

    bool isNull() const { return node_ && node_->type == YYAML_NULL; }
    bool isBool() const { return node_ && node_->type == YYAML_BOOL; }
    bool isInt() const { return node_ && node_->type == YYAML_INT; }
    bool isDouble() const { return node_ && node_->type == YYAML_DOUBLE; }
    bool isNumber() const { return isInt() || isDouble(); }
    bool isString() const { return node_ && node_->type == YYAML_STRING; }
    bool isSequence() const { return node_ && node_->type == YYAML_SEQUENCE; }
    bool isMapping() const { return node_ && node_->type == YYAML_MAPPING; }
    bool isScalar() const { return node_ && yyaml_is_scalar(node_); }

    std::nullptr_t asNull() const;
    bool asBool() const;
    std::int64_t asInt() const;
    double asDouble() const;
    double asNumber() const;
    std::string asString() const;
    std::string toString() const;

    node operator[](const std::string &key) const;
    node at(std::size_t index) const;

    std::size_t size() const;

    template <typename Func>
    void forEachMember(Func &&func) const;

    const ::yyaml_node *raw() const { return node_; }

private:
    const ::yyaml_doc *doc_ = nullptr;
    const ::yyaml_node *node_ = nullptr;

    node(const ::yyaml_doc *doc, const ::yyaml_node *node) : doc_(doc), node_(node) {}

    friend class document;

    void require_bound() const;
    void require_scalar() const;
    std::string read_string() const;
};

class document {
public:
    document() = default;

    explicit document(::yyaml_doc *doc) : handle_(doc) {}

    document(document &&) noexcept = default;
    document &operator=(document &&) noexcept = default;

    document(const document &) = delete;
    document &operator=(const document &) = delete;

    static document parse(std::string_view yaml, const read_opts *opts = nullptr) {
        ::yyaml_err err = {0};
        ::yyaml_doc *doc = yyaml_read(yaml.data(), yaml.size(), opts, &err);
        if (!doc) {
            throw error(err);
        }
        return document(doc);
    }

    static document parse_file(const std::string &path, const read_opts *opts = nullptr) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            throw error("unable to open YAML file: " + path);
        }
        std::string data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        return parse(data, opts);
    }

    node root() const {
        if (!handle_) {
            return {};
        }
        return node(get(), yyaml_doc_get_root(get()));
    }

    const ::yyaml_doc *get() const { return handle_.get(); }

    bool valid() const { return static_cast<bool>(handle_); }

    std::string dump(const write_opts *opts = nullptr) const;

private:
    struct deleter {
        void operator()(::yyaml_doc *doc) const {
            if (doc) {
                yyaml_doc_free(doc);
            }
        }
    };

    std::unique_ptr<::yyaml_doc, deleter> handle_{nullptr};

    friend class node;
};

inline node node::operator[](const std::string &key) const {
    if (!doc_ || !node_ || !isMapping()) {
        return {};
    }
    const ::yyaml_node *child = yyaml_map_get(doc_, node_, key.c_str());
    return node(doc_, child);
}

inline node node::at(std::size_t index) const {
    if (!doc_ || !node_ || !isSequence()) {
        return {};
    }
    const ::yyaml_node *child = yyaml_seq_get(doc_, node_, index);
    return node(doc_, child);
}

inline std::size_t node::size() const {
    if (!node_) {
        return 0;
    }
    if (isSequence()) {
        return yyaml_seq_len(node_);
    }
    if (isMapping()) {
        return yyaml_map_len(node_);
    }
    return 0;
}

template <typename Func>
inline void node::forEachMember(Func &&func) const {
    require_bound();
    if (!isMapping()) {
        throw error("yyaml::node is not a mapping");
    }

    const char *buf = yyaml_doc_get_scalar_buf(doc_);
    if (!buf) {
        throw error("yyaml scalar buffer is null");
    }
    const auto none = std::numeric_limits<uint32_t>::max();
    const ::yyaml_node *child = yyaml_doc_get(doc_, node_->child);

    while (child) {
        std::string key;
        key.assign(buf + static_cast<std::size_t>(child->extra),
                   static_cast<std::size_t>(child->flags));

        func(key, node(doc_, child));

        if (child->next == none) {
            break;
        }
        child = yyaml_doc_get(doc_, child->next);
    }
}

inline std::nullptr_t node::asNull() const {
    require_bound();
    if (!isNull()) {
        throw error("yyaml::node is not null");
    }
    return nullptr;
}

inline bool node::asBool() const {
    require_bound();
    if (!isBool()) {
        throw error("yyaml::node is not a bool");
    }
    return node_->val.boolean;
}

inline std::int64_t node::asInt() const {
    require_bound();
    if (!isInt()) {
        throw error("yyaml::node is not an integer");
    }
    return node_->val.integer;
}

inline double node::asDouble() const {
    require_bound();
    if (!isDouble()) {
        throw error("yyaml::node is not a double");
    }
    return node_->val.real;
}

inline double node::asNumber() const {
    require_scalar();
    if (isInt()) {
        return static_cast<double>(asInt());
    }
    if (isDouble()) {
        return asDouble();
    }
    throw error("yyaml::node is not numeric");
}

inline std::string node::asString() const {
    return read_string();
}

inline std::string node::toString() const {
    require_scalar();
    switch (node_->type) {
    case YYAML_NULL:
        return "null";
    case YYAML_BOOL:
        return node_->val.boolean ? "true" : "false";
    case YYAML_INT:
        return std::to_string(node_->val.integer);
    case YYAML_DOUBLE:
        return std::to_string(node_->val.real);
    case YYAML_STRING:
        return read_string();
    default:
        throw error("yyaml::node holds an unknown scalar type");
    }
}

inline void node::require_bound() const {
    if (!doc_ || !node_) {
        throw error("yyaml::node is not bound to a document");
    }
}

inline void node::require_scalar() const {
    require_bound();
    if (!isScalar()) {
        throw error("yyaml::node is not a scalar");
    }
}

inline std::string node::read_string() const {
    require_bound();
    if (!isString()) {
        throw error("yyaml::node is not a string");
    }
    const char *buf = yyaml_doc_get_scalar_buf(doc_);
    if (!buf) {
        throw error("yyaml scalar buffer is null");
    }
    return std::string(buf + node_->val.str.ofs, node_->val.str.len);
}

inline std::string document::dump(const write_opts *opts) const {
    if (!handle_) {
        throw error("yyaml::document is not initialized");
    }

    char *buffer = nullptr;
    size_t len = 0;
    ::yyaml_err err = {0};

    if (!yyaml_write(get(), &buffer, &len, opts, &err)) {
        throw error(err);
    }

    std::string out(buffer, len);
    yyaml_free_string(buffer);
    return out;
}

} // namespace yyaml

