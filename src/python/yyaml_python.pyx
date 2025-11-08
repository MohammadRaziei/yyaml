# cython: language_level=3
"""Cython bindings for the yyaml C library."""

import json
import os

# from libc.stddef cimport NULL, size_t
# from libc.stdbool cimport bool
# from libc.stdint cimport int64_t, uint32_t
# from libc.string cimport strlen

# cdef extern from "yyaml.h":
#     ctypedef enum yyaml_type:
#         YYAML_NULL
#         YYAML_BOOL
#         YYAML_INT
#         YYAML_DOUBLE
#         YYAML_STRING
#         YYAML_SEQUENCE
#         YYAML_MAPPING

#     ctypedef struct yyaml_string_struct:
#         uint32_t ofs
#         uint32_t len

#     ctypedef union yyaml_value_union:
#         bool boolean
#         int64_t integer
#         double real
#         yyaml_string_struct str

#     ctypedef struct yyaml_node:
#         uint32_t type
#         uint32_t flags
#         uint32_t parent
#         uint32_t next
#         uint32_t child
#         uint32_t extra
#         yyaml_value_union val

#     ctypedef struct yyaml_doc:
#         pass

#     ctypedef struct yyaml_read_opts:
#         bool allow_duplicate_keys
#         bool allow_trailing_content
#         bool allow_inf_nan
#         size_t max_nesting

#     ctypedef struct yyaml_err:
#         size_t pos
#         size_t line
#         size_t column
#         char msg[96]

#     ctypedef struct yyaml_write_opts:
#         size_t indent
#         bool final_newline

#     yyaml_doc *yyaml_read(const char *data, size_t len,
#                           const yyaml_read_opts *opts,
#                           yyaml_err *err)

#     void yyaml_doc_free(yyaml_doc *doc)

#     const yyaml_node *yyaml_doc_get_root(const yyaml_doc *doc)

#     const yyaml_node *yyaml_doc_get(const yyaml_doc *doc, uint32_t idx)

#     const char *yyaml_doc_get_scalar_buf(const yyaml_doc *doc)

#     size_t yyaml_doc_node_count(const yyaml_doc *doc)

#     bool yyaml_write(const yyaml_doc *doc, char **out, size_t *out_len,
#                      const yyaml_write_opts *opts, yyaml_err *err)

#     void yyaml_free_string(char *str)


# def _coerce_to_bytes(object data):
#     """Return *data* as UTF-8 encoded bytes."""
#     if isinstance(data, bytes):
#         return data
#     if isinstance(data, bytearray):
#         return bytes(data)
#     if isinstance(data, memoryview):
#         return data.tobytes()
#     if isinstance(data, str):
#         return data.encode("utf-8")
#     raise TypeError("expected str or bytes-like object")


# cdef inline str _decode_c_string(const char *value):
#     if value == NULL:
#         return ""
#     cdef size_t length = strlen(value)
#     if length == 0:
#         return ""
#     return (<bytes>value[:length]).decode("utf-8", "replace")


# cdef uint32_t YYAML_INDEX_NONE = <uint32_t>0xFFFFFFFF


# cdef object _node_to_python(const yyaml_doc *doc, const yyaml_node *node):
#     if node == NULL:
#         return None

#     cdef yyaml_type node_type = <yyaml_type>node.type

#     if node_type == YYAML_NULL:
#         return None
#     elif node_type == YYAML_BOOL:
#         return bool(node.val.boolean)
#     elif node_type == YYAML_INT:
#         return node.val.integer
#     elif node_type == YYAML_DOUBLE:
#         return node.val.real
#     elif node_type == YYAML_STRING:
#         cdef const char *buf = yyaml_doc_get_scalar_buf(doc)
#         if buf == NULL:
#             return ""
#         cdef uint32_t start = node.val.str.ofs
#         cdef uint32_t length = node.val.str.len
#         if length == 0:
#             return ""
#         return (<bytes>buf[start:start + length]).decode("utf-8")
#     elif node_type == YYAML_SEQUENCE:
#         result = []
#         cdef uint32_t idx = node.child
#         cdef const yyaml_node *child
#         while idx != YYAML_INDEX_NONE:
#             child = yyaml_doc_get(doc, idx)
#             if child == NULL:
#                 break
#             result.append(_node_to_python(doc, child))
#             idx = child.next
#         return result
#     elif node_type == YYAML_MAPPING:
#         mapping = {}
#         cdef uint32_t idx = node.child
#         cdef const yyaml_node *child
#         cdef const char *buf = yyaml_doc_get_scalar_buf(doc)
#         cdef uint32_t key_len
#         cdef uint32_t key_ofs
#         while idx != YYAML_INDEX_NONE:
#             child = yyaml_doc_get(doc, idx)
#             if child == NULL:
#                 break
#             key_len = child.flags
#             key_ofs = child.extra
#             if buf != NULL and key_len != 0:
#                 key = (<bytes>buf[key_ofs:key_ofs + key_len]).decode("utf-8")
#             else:
#                 key = ""
#             mapping[key] = _node_to_python(doc, child)
#             idx = child.next
#         return mapping

#     return None


# cdef class YYAMLError(Exception):
#     """Exception raised when yyaml fails to parse or write data."""

#     def __init__(self, message, pos=0, line=0, column=0):
#         self.message = message
#         self.pos = pos
#         self.line = line
#         self.column = column
#         super().__init__(f"{message} at line {line}, column {column} (position {pos})")


# cdef class YYAMLDocument:
#     """A thin wrapper over :c:type:`yyaml_doc`."""

#     cdef yyaml_doc *_doc

#     def __cinit__(self):
#         self._doc = NULL

#     def __dealloc__(self):
#         if self._doc != NULL:
#             yyaml_doc_free(self._doc)
#             self._doc = NULL

#     @staticmethod
#     def load(bytes yaml_data, **kwargs):
#         cdef YYAMLDocument doc = YYAMLDocument()
#         cdef yyaml_read_opts opts
#         cdef yyaml_err err
#         err.msg[0] = '\0'

#         opts.allow_duplicate_keys = bool(kwargs.get("allow_duplicate_keys", False))
#         opts.allow_trailing_content = bool(kwargs.get("allow_trailing_content", False))
#         opts.allow_inf_nan = bool(kwargs.get("allow_inf_nan", False))
#         opts.max_nesting = <size_t>kwargs.get("max_nesting", 64)

#         cdef const char *data_ptr = yaml_data
#         cdef size_t data_len = <size_t>len(yaml_data)

#         doc._doc = yyaml_read(data_ptr, data_len, &opts, &err)

#         if doc._doc == NULL:
#             raise YYAMLError(_decode_c_string(<const char *>err.msg), err.pos, err.line, err.column)

#         return doc

#     @staticmethod
#     def loads(object yaml_string, **kwargs):
#         yaml_data = _coerce_to_bytes(yaml_string)
#         return YYAMLDocument.load(yaml_data, **kwargs)

#     @property
#     def root(self):
#         if self._doc == NULL:
#             return None
#         cdef const yyaml_node *root_node = yyaml_doc_get_root(self._doc)
#         if root_node == NULL:
#             return None
#         return root_node

#     def as_python(self):
#         if self._doc == NULL:
#             return None
#         cdef const yyaml_node *root_node = yyaml_doc_get_root(self._doc)
#         if root_node == NULL:
#             return None
#         return _node_to_python(self._doc, root_node)

#     def dumps(self, indent=2, final_newline=True):
#         if self._doc == NULL:
#             return ""

#         indent_value = int(indent)
#         if indent_value < 0:
#             raise ValueError("indent must be non-negative")

#         cdef yyaml_write_opts opts
#         cdef yyaml_err err
#         cdef char *output = NULL
#         cdef size_t output_len = 0

#         opts.indent = <size_t>indent_value
#         opts.final_newline = bool(final_newline)
#         err.msg[0] = '\0'

#         cdef bint result = yyaml_write(self._doc, &output, &output_len, &opts, &err)

#         if not result:
#             raise YYAMLError(_decode_c_string(<const char *>err.msg), err.pos, err.line, err.column)

#         try:
#             if output != NULL and output_len > 0:
#                 return (<bytes>output[:output_len]).decode("utf-8")
#             return ""
#         finally:
#             if output != NULL:
#                 yyaml_free_string(output)

#     def __len__(self):
#         if self._doc == NULL:
#             return 0
#         return yyaml_doc_node_count(self._doc)


# def yyaml_loads(object yaml_string, **kwargs):
#     """Parse *yaml_string* and return a :class:`YYAMLDocument`."""
#     return YYAMLDocument.loads(yaml_string, **kwargs)


# def yyaml_load(stream, **kwargs):
#     """Parse YAML content from *stream* and return a :class:`YYAMLDocument`."""
#     if hasattr(stream, "read"):
#         data = stream.read()
#     elif isinstance(stream, (str, os.PathLike)):
#         with open(stream, "rb") as fh:
#             data = fh.read()
#     else:
#         raise TypeError("stream must be a file path or a file-like object")

#     return yyaml_loads(data, **kwargs)


# def yyaml_dumps(object doc, indent=2, final_newline=True):
#     """Serialize a :class:`YYAMLDocument` into a YAML string."""
#     if isinstance(doc, YYAMLDocument):
#         return doc.dumps(indent=indent, final_newline=final_newline)
#     raise TypeError("yyaml_dumps expects a YYAMLDocument instance")


# def yyaml_dump(object doc, stream, indent=2, final_newline=True):
#     """Serialize a :class:`YYAMLDocument` and write it to *stream*."""
#     yaml_str = yyaml_dumps(doc, indent=indent, final_newline=final_newline)

#     if hasattr(stream, "write"):
#         stream.write(yaml_str)
#     elif isinstance(stream, (str, os.PathLike)):
#         with open(stream, "w", encoding="utf-8") as fh:
#             fh.write(yaml_str)
#     else:
#         raise TypeError("stream must be a file path or a file-like object")


# def dict2yyaml(dict mapping):
#     """Return a :class:`YYAMLDocument` representing *mapping*."""
#     if not isinstance(mapping, dict):
#         raise TypeError("dict2yyaml expects a dict instance")

#     intermediate = json.dumps(mapping, ensure_ascii=False)
#     return yyaml_loads(intermediate)


# def yyaml2dict(object data):
#     """Return a Python ``dict`` representation of *data*."""
#     if isinstance(data, YYAMLDocument):
#         result = data.as_python()
#     else:
#         raise TypeError("yyaml2dict expects a YYAMLDocument instance")

#     if not isinstance(result, dict):
#         raise TypeError("YYAMLDocument root is not a mapping")

#     return result


# def loads(object yaml_string, **kwargs):
#     """Parse *yaml_string* and return the corresponding Python ``dict``."""
#     doc = yyaml_loads(yaml_string, **kwargs)
#     return yyaml2dict(doc)


# def load(stream, **kwargs):
#     """Parse YAML content from *stream* and return a Python ``dict``."""
#     doc = yyaml_load(stream, **kwargs)
#     return yyaml2dict(doc)


# def dumps(dict mapping, indent=2, final_newline=True):
#     """Serialize a Python ``dict`` into a YAML string."""
#     doc = dict2yyaml(mapping)
#     return doc.dumps(indent=indent, final_newline=final_newline)


# def dump(dict mapping, stream, indent=2, final_newline=True):
#     """Serialize a Python ``dict`` and write it to *stream*."""
#     yaml_str = dumps(mapping, indent=indent, final_newline=final_newline)

#     if hasattr(stream, "write"):
#         stream.write(yaml_str)
#     elif isinstance(stream, (str, os.PathLike)):
#         with open(stream, "w", encoding="utf-8") as fh:
#             fh.write(yaml_str)
#     else:
#         raise TypeError("stream must be a file path or a file-like object")


__all__ = [
#     "YYAMLDocument",
#     "YYAMLError",
#     "dict2yyaml",
#     "yyaml2dict",
#     "load",
#     "loads",
#     "dump",
#     "dumps",
#     "yyaml_load",
#     "yyaml_loads",
#     "yyaml_dump",
#     "yyaml_dumps",
]
