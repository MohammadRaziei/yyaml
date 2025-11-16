# cython: language_level=3
"""Cython bindings for the yyaml C library.

This module exposes a small, pythonic wrapper around the C structures while
keeping allocations inside the yyaml library. The public surface mirrors a
subset of the C++ helpers defined in ``yyaml.hpp``.
"""
from libc.stdint cimport uint32_t, int64_t
from libc.string cimport strlen
from libc.stddef cimport size_t

cdef extern from "yyaml.h":
    ctypedef bint yyaml_bool

    ctypedef enum yyaml_type:
        YYAML_NULL
        YYAML_BOOL
        YYAML_INT
        YYAML_DOUBLE
        YYAML_STRING
        YYAML_SEQUENCE
        YYAML_MAPPING

    ctypedef struct yyaml_read_opts:
        yyaml_bool allow_duplicate_keys
        yyaml_bool allow_trailing_content
        yyaml_bool allow_inf_nan
        size_t max_nesting

    ctypedef struct yyaml_write_opts:
        size_t indent
        yyaml_bool final_newline

    ctypedef struct yyaml_err:
        size_t pos
        size_t line
        size_t column
        char msg[96]

    ctypedef struct yyaml_doc

    ctypedef struct yyaml_string_val:
        uint32_t ofs
        uint32_t len

    ctypedef union yyaml_node_val:
        yyaml_bool boolean
        int64_t integer
        double real
        yyaml_string_val str

    ctypedef struct yyaml_node:
        const yyaml_doc *doc
        uint32_t type
        uint32_t flags
        uint32_t parent
        uint32_t next
        uint32_t child
        uint32_t extra
        yyaml_node_val val

    yyaml_doc *yyaml_read(const char *data, size_t len,
                          const yyaml_read_opts *opts,
                          yyaml_err *err)
    yyaml_doc *yyaml_doc_new()
    void yyaml_doc_free(yyaml_doc *doc)
    const yyaml_node *yyaml_doc_get_root(const yyaml_doc *doc)
    const yyaml_node *yyaml_doc_get(const yyaml_doc *doc, uint32_t idx)
    const char *yyaml_doc_get_scalar_buf(const yyaml_doc *doc)
    size_t yyaml_doc_node_count(const yyaml_doc *doc)

    bint yyaml_is_scalar(const yyaml_node *node)
    bint yyaml_is_container(const yyaml_node *node)
    const yyaml_node *yyaml_map_get(const yyaml_node *map, const char *key)
    const yyaml_node *yyaml_seq_get(const yyaml_node *seq, size_t index)
    size_t yyaml_seq_len(const yyaml_node *seq)
    size_t yyaml_map_len(const yyaml_node *map)

    bint yyaml_doc_set_root(yyaml_doc *doc, uint32_t idx)
    uint32_t yyaml_doc_add_null(yyaml_doc *doc)
    uint32_t yyaml_doc_add_bool(yyaml_doc *doc, bint value)
    uint32_t yyaml_doc_add_int(yyaml_doc *doc, int64_t value)
    uint32_t yyaml_doc_add_double(yyaml_doc *doc, double value)
    uint32_t yyaml_doc_add_string(yyaml_doc *doc, const char *str, size_t len)
    uint32_t yyaml_doc_add_sequence(yyaml_doc *doc)
    uint32_t yyaml_doc_add_mapping(yyaml_doc *doc)
    bint yyaml_doc_seq_append(yyaml_doc *doc, uint32_t seq_idx, uint32_t child_idx)
    bint yyaml_doc_map_append(yyaml_doc *doc, uint32_t map_idx,
                              const char *key, size_t key_len,
                              uint32_t val_idx)

    bint yyaml_write(const yyaml_node *root, char **out, size_t *out_len,
                     const yyaml_write_opts *opts, yyaml_err *err)
    void yyaml_free_string(char *str)

# yyaml uses UINT32_MAX as the end-of-list sentinel for child nodes.
DEF _NONE_IDX = 0xFFFFFFFF


cdef inline str _quote_string(str text):
    escaped = text.replace("\\", "\\\\").replace("\"", "\\\"")
    escaped = escaped.replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t")
    return f"\"{escaped}\""


cdef inline str _format_error(const yyaml_err *err):
    """Build a human-readable error message from a yyaml_err structure."""
    if err is NULL:
        return "yyaml error"
    msg_len = strlen(<const char *>err.msg)
    text = (<const char *>err.msg)[:msg_len].decode("utf-8", "replace")
    return f"{text} (line {err.line}, column {err.column})"


cdef uint32_t _build_node(object obj, yyaml_doc *doc):
    cdef uint32_t idx
    cdef uint32_t child
    cdef bytes data
    if obj is None:
        return yyaml_doc_add_null(doc)
    if obj is True:
        return yyaml_doc_add_bool(doc, 1)
    if obj is False:
        return yyaml_doc_add_bool(doc, 0)
    if isinstance(obj, int):
        return yyaml_doc_add_int(doc, <int64_t>obj)
    if isinstance(obj, float):
        return yyaml_doc_add_double(doc, obj)
    if isinstance(obj, str):
        data = obj.encode("utf-8")
        return yyaml_doc_add_string(doc, data, <size_t>len(data))
    if isinstance(obj, (list, tuple)):
        idx = yyaml_doc_add_sequence(doc)
        if idx == _NONE_IDX:
            return _NONE_IDX
        for item in obj:
            child = _build_node(item, doc)
            if child == _NONE_IDX:
                return _NONE_IDX
            if not yyaml_doc_seq_append(doc, idx, child):
                return _NONE_IDX
        return idx
    if isinstance(obj, dict):
        idx = yyaml_doc_add_mapping(doc)
        if idx == _NONE_IDX:
            return _NONE_IDX
        for key, value in obj.items():
            key_text = str(key)
            key_bytes = key_text.encode("utf-8")
            child = _build_node(value, doc)
            if child == _NONE_IDX:
                return _NONE_IDX
            if not yyaml_doc_map_append(doc, idx, key_bytes,
                                        <size_t>len(key_bytes), child):
                return _NONE_IDX
        return idx
    # Fallback: store the string representation
    return _build_node(str(obj), doc)


cdef class Node:
    cdef const yyaml_node *_node
    cdef object _owner  # keep document alive

    def __cinit__(self):
        self._owner = None
        self._node = NULL

    def __bool__(self):
        return self._node is not NULL and self._node.doc is not NULL

    property type:
        """Return the numeric yyaml_type value for this node."""
        def __get__(self):
            if self._node is NULL:
                return YYAML_NULL
            return self._node.type

    property is_scalar:
        def __get__(self):
            return self._node is not NULL and yyaml_is_scalar(self._node)

    property is_container:
        def __get__(self):
            return self._node is not NULL and yyaml_is_container(self._node)

    property is_null:
        def __get__(self):
            return self._node is not NULL and self._node.type == YYAML_NULL

    property is_bool:
        def __get__(self):
            return self._node is not NULL and self._node.type == YYAML_BOOL

    property is_int:
        def __get__(self):
            return self._node is not NULL and self._node.type == YYAML_INT

    property is_double:
        def __get__(self):
            return self._node is not NULL and self._node.type == YYAML_DOUBLE

    property is_string:
        def __get__(self):
            return self._node is not NULL and self._node.type == YYAML_STRING

    property is_sequence:
        def __get__(self):
            return self._node is not NULL and self._node.type == YYAML_SEQUENCE

    property is_mapping:
        def __get__(self):
            return self._node is not NULL and self._node.type == YYAML_MAPPING

    def __getitem__(self, key):
        cdef const yyaml_node *child
        if self._node is NULL or self._node.doc is NULL:
            raise ValueError("node is not bound to a document")
        if self._node.type == YYAML_MAPPING:
            encoded = key.encode("utf-8")
            child = yyaml_map_get(self._node, <const char *>encoded)
            if child is NULL:
                raise KeyError(key)
            return _wrap_node(self._owner, child)
        if self._node.type == YYAML_SEQUENCE:
            child = yyaml_seq_get(self._node, <size_t>key)
            if child is NULL:
                raise IndexError(key)
            return _wrap_node(self._owner, child)
        raise TypeError("node is not subscriptable")

    def __len__(self):
        if self._node is NULL:
            return 0
        if self._node.type == YYAML_SEQUENCE:
            return yyaml_seq_len(self._node)
        if self._node.type == YYAML_MAPPING:
            return yyaml_map_len(self._node)
        return 0

    def __iter__(self):
        if not self.is_container:
            raise TypeError("node is not iterable")
        return NodeIterator(self)

    def to_dict(self):
        """Convert this node tree into native Python objects."""
        cdef const char *buf
        cdef const yyaml_node *child
        cdef size_t i
        cdef size_t size
        if self._node is NULL:
            return None
        cdef uint32_t t = self._node.type
        if t == YYAML_NULL:
            return None
        if t == YYAML_BOOL:
            return bool(self._node.val.boolean)
        if t == YYAML_INT:
            return int(self._node.val.integer)
        if t == YYAML_DOUBLE:
            return float(self._node.val.real)
        if t == YYAML_STRING:
            buf = yyaml_doc_get_scalar_buf(self._node.doc)
            if buf is NULL:
                raise ValueError("yyaml scalar buffer is null")
            return (<const char *>buf + self._node.val.str.ofs)[:self._node.val.str.len].decode("utf-8")
        if t == YYAML_SEQUENCE:
            size = yyaml_seq_len(self._node)
            out = []
            for i in range(size):
                child = yyaml_seq_get(self._node, i)
                if child is NULL:
                    out.append(None)
                else:
                    out.append(_wrap_node(self._owner, child).to_dict())
            return out
        if t == YYAML_MAPPING:
            buf = yyaml_doc_get_scalar_buf(self._node.doc)
            if buf is NULL:
                raise ValueError("yyaml scalar buffer is null")
            result = {}
            child = yyaml_doc_get(self._node.doc, self._node.child)
            while child is not NULL:
                key = (<const char *>buf + child.extra)[:child.flags].decode("utf-8")
                result[key] = _wrap_node(self._owner, child).to_dict()
                if child.next == _NONE_IDX:
                    break
                child = yyaml_doc_get(self._node.doc, child.next)
            return result
        raise TypeError("unknown yyaml node type")


cdef class NodeIterator:
    cdef Node _parent
    cdef const yyaml_node *_next

    def __cinit__(self, Node parent):
        self._parent = parent
        if parent._node is not NULL and yyaml_is_container(parent._node):
            self._next = yyaml_doc_get(parent._node.doc, parent._node.child)
        else:
            self._next = NULL

    def __iter__(self):
        return self

    def __next__(self):
        if self._next is NULL:
            raise StopIteration
        current = _wrap_node(self._parent._owner, self._next)
        if self._next.next == _NONE_IDX:
            self._next = NULL
        else:
            self._next = yyaml_doc_get(self._parent._node.doc, self._next.next)
        return current


cdef Node _wrap_node(object owner, const yyaml_node *ptr):
    cdef Node node = Node.__new__(Node)
    node._owner = owner
    node._node = ptr
    return node


cdef class Document:
    cdef yyaml_doc *_doc

    def __cinit__(self):
        self._doc = NULL

    def __dealloc__(self):
        if self._doc is not NULL:
            yyaml_doc_free(self._doc)
            self._doc = NULL

    @property
    def valid(self):
        return self._doc is not NULL

    @property
    def root(self):
        """Return the root node wrapped as :class:`Node`."""
        if self._doc is NULL:
            return None
        cdef const yyaml_node *raw = yyaml_doc_get_root(self._doc)
        if raw is NULL:
            return None
        return _wrap_node(self, raw)

    def to_dict(self):
        root_node = self.root
        if root_node is None:
            return None
        return root_node.to_dict()

    def dump(self, opts=None):
        """Serialize the document to YAML text."""
        if self._doc is NULL:
            raise ValueError("document is not initialized")

        cdef yyaml_write_opts c_opts
        cdef yyaml_write_opts *opts_ptr = NULL
        if opts is not None:
            c_opts.indent = <size_t>opts.get("indent", 2)
            c_opts.final_newline = bool(opts.get("final_newline", True))
            opts_ptr = &c_opts

        cdef char *buffer = NULL
        cdef size_t length = 0
        cdef yyaml_err err
        if not yyaml_write(yyaml_doc_get_root(self._doc), &buffer, &length, opts_ptr, &err):
            raise ValueError(_format_error(&err))
        try:
            return (<const char *>buffer)[:length].decode("utf-8")
        finally:
            if buffer is not NULL:
                yyaml_free_string(buffer)

    @staticmethod
    def parse(text, opts=None):
        """Parse YAML text into a :class:`Document`."""
        cdef bytes encoded = text.encode("utf-8")
        cdef const char *data = encoded
        cdef size_t size = <size_t>len(encoded)

        cdef yyaml_read_opts c_opts
        cdef yyaml_read_opts *opts_ptr = NULL
        if opts is not None:
            c_opts.allow_duplicate_keys = bool(opts.get("allow_duplicate_keys", False))
            c_opts.allow_trailing_content = bool(opts.get("allow_trailing_content", False))
            c_opts.allow_inf_nan = bool(opts.get("allow_inf_nan", True))
            c_opts.max_nesting = <size_t>opts.get("max_nesting", 0)
            opts_ptr = &c_opts

        cdef yyaml_err err
        cdef yyaml_doc *doc = yyaml_read(data, size, opts_ptr, &err)
        if doc is NULL:
            raise ValueError(_format_error(&err))

        cdef Document wrapper = Document.__new__(Document)
        wrapper._doc = doc
        return wrapper

    @staticmethod
    def parse_file(path, opts=None):
        with open(path, "rb") as handle:
            content = handle.read().decode("utf-8")
        return Document.parse(content, opts=opts)

    @staticmethod
    def from_dict(obj, *, opts=None):
        """Create a :class:`Document` from a native Python object."""
        cdef yyaml_doc *doc = yyaml_doc_new()
        if doc is NULL:
            raise MemoryError("failed to allocate document")

        cdef uint32_t root_idx = _build_node(obj, doc)
        if root_idx == _NONE_IDX or not yyaml_doc_set_root(doc, root_idx):
            yyaml_doc_free(doc)
            raise ValueError("failed to build document from input")

        cdef Document wrapper = Document.__new__(Document)
        wrapper._doc = doc
        return wrapper


# Convenience functions similar to the :mod:`json` API

def loads(text, *, opts=None):
    """Parse YAML text and return Python objects."""
    doc = Document.parse(text, opts=opts)
    return doc.to_dict()


def load(fp, *, opts=None):
    """Parse YAML content from a file-like object and return Python objects."""
    content = fp.read()
    if isinstance(content, bytes):
        content = content.decode("utf-8")
    doc = Document.parse(content, opts=opts)
    return doc.to_dict()


def dumps(obj, *, opts=None):
    """Serialize Python objects or :class:`Document` instances to YAML text."""
    if isinstance(obj, Document):
        return obj.dump(opts=opts)
    return Document.from_dict(obj, opts=opts).dump(opts=opts)


def dump(obj, fp, *, opts=None):
    """Serialize and write YAML content to a file-like object."""
    text = dumps(obj, opts=opts)
    fp.write(text)
    return None

__all__ = [
    "Node",
    "NodeIterator",
    "Document",
    "loads",
    "load",
    "dumps",
    "dump",
]
