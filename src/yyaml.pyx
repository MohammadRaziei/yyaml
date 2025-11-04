# Cython wrapper for yyaml library
# This provides a Python interface to the yyaml C library

cdef extern from "yyaml.h":
    ctypedef enum yyaml_type:
        YYAML_NULL = 0
        YYAML_BOOL
        YYAML_INT
        YYAML_DOUBLE
        YYAML_STRING
        YYAML_SEQUENCE
        YYAML_MAPPING
    
    ctypedef struct yyaml_node:
        uint32_t type
        uint32_t flags
        uint32_t parent
        uint32_t next
        uint32_t child
        uint32_t extra
        union:
            bool boolean
            int64_t integer
            double real
            struct:
                uint32_t ofs
                uint32_t len
    
    ctypedef struct yyaml_doc:
        pass
    
    ctypedef struct yyaml_read_opts:
        bool allow_duplicate_keys
        bool allow_trailing_content
        bool allow_inf_nan
        size_t max_nesting
    
    ctypedef struct yyaml_err:
        size_t pos
        size_t line
        size_t column
        char msg[96]
    
    ctypedef struct yyaml_write_opts:
        size_t indent
        bool final_newline
    
    # Reading API
    yyaml_doc *yyaml_read(const char *data, size_t len,
                         const yyaml_read_opts *opts,
                         yyaml_err *err)
    
    void yyaml_doc_free(yyaml_doc *doc)
    
    const yyaml_node *yyaml_doc_get_root(const yyaml_doc *doc)
    
    const yyaml_node *yyaml_doc_get(const yyaml_doc *doc, uint32_t idx)
    
    const char *yyaml_doc_get_scalar_buf(const yyaml_doc *doc)
    
    size_t yyaml_doc_node_count(const yyaml_doc *doc)
    
    # Convenience helpers
    bool yyaml_is_scalar(const yyaml_node *node)
    
    bool yyaml_is_container(const yyaml_node *node)
    
    bool yyaml_str_eq(const yyaml_doc *doc, const yyaml_node *node,
                     const char *str)
    
    const yyaml_node *yyaml_map_get(const yyaml_doc *doc,
                                   const yyaml_node *map,
                                   const char *key)
    
    const yyaml_node *yyaml_seq_get(const yyaml_doc *doc,
                                   const yyaml_node *seq,
                                   size_t index)
    
    size_t yyaml_seq_len(const yyaml_node *seq)
    
    size_t yyaml_map_len(const yyaml_node *map)
    
    # Writing API
    bool yyaml_write(const yyaml_doc *doc, char **out, size_t *out_len,
                    const yyaml_write_opts *opts, yyaml_err *err)
    
    void yyaml_free_string(char *str)

from libc.stdint cimport uint32_t, int64_t
from libc.stdlib cimport malloc, free
from libc.string cimport strlen

cdef class YYAMLError(Exception):
    """Exception raised for YYAML parsing errors"""
    def __init__(self, message, pos=0, line=0, column=0):
        self.message = message
        self.pos = pos
        self.line = line
        self.column = column
        super().__init__(f"{message} at line {line}, column {column} (position {pos})")

cdef class YYAMLNode:
    """Wrapper for yyaml_node"""
    cdef const yyaml_doc* _doc
    cdef const yyaml_node* _node
    
    def __cinit__(self, const yyaml_doc* doc, const yyaml_node* node):
        self._doc = doc
        self._node = node
    
    @property
    def type(self):
        """Get the node type"""
        if self._node == NULL:
            return None
        cdef yyaml_type node_type = <yyaml_type>self._node.type
        type_names = {
            YYAML_NULL: "null",
            YYAML_BOOL: "bool",
            YYAML_INT: "int",
            YYAML_DOUBLE: "double",
            YYAML_STRING: "string",
            YYAML_SEQUENCE: "sequence",
            YYAML_MAPPING: "mapping"
        }
        return type_names.get(node_type, "unknown")
    
    @property
    def value(self):
        """Get the node value"""
        if self._node == NULL:
            return None
        
        cdef yyaml_type node_type = <yyaml_type>self._node.type
        
        if node_type == YYAML_NULL:
            return None
        elif node_type == YYAML_BOOL:
            return bool(self._node.boolean)
        elif node_type == YYAML_INT:
            return self._node.integer
        elif node_type == YYAML_DOUBLE:
            return self._node.real
        elif node_type == YYAML_STRING:
            cdef const char* buf = yyaml_doc_get_scalar_buf(self._doc)
            if buf == NULL:
                return ""
            return buf[self._node.str.ofs:self._node.str.ofs + self._node.str.len].decode('utf-8')
        elif node_type == YYAML_SEQUENCE:
            return [YYAMLNode(self._doc, yyaml_seq_get(self._doc, self._node, i)) 
                   for i in range(yyaml_seq_len(self._node))]
        elif node_type == YYAML_MAPPING:
            # For mappings, we need to iterate through children
            result = {}
            cdef uint32_t idx = self._node.child
            cdef const yyaml_node* child
            cdef const char* buf
            cdef const char* key_str
            
            while idx != 0xFFFFFFFF:  # UINT32_MAX
                child = yyaml_doc_get(self._doc, idx)
                if child != NULL:
                    buf = yyaml_doc_get_scalar_buf(self._doc)
                    if buf != NULL:
                        key_str = buf + child.extra
                        result[key_str[:child.flags].decode('utf-8')] = YYAMLNode(self._doc, child)
                idx = child.next if child != NULL else 0xFFFFFFFF
            
            return result
        
        return None
    
    def __str__(self):
        return str(self.value)
    
    def __repr__(self):
        return f"YYAMLNode(type={self.type}, value={self.value})"

cdef class YYAMLDocument:
    """Wrapper for yyaml_doc"""
    cdef yyaml_doc* _doc
    
    def __cinit__(self):
        self._doc = NULL
    
    def __dealloc__(self):
        if self._doc != NULL:
            yyaml_doc_free(self._doc)
    
    @staticmethod
    def load(bytes yaml_data, **kwargs):
        """Load YAML from bytes"""
        cdef YYAMLDocument doc = YYAMLDocument()
        cdef yyaml_read_opts opts
        cdef yyaml_err err
        
        # Set options
        opts.allow_duplicate_keys = kwargs.get('allow_duplicate_keys', False)
        opts.allow_trailing_content = kwargs.get('allow_trailing_content', False)
        opts.allow_inf_nan = kwargs.get('allow_inf_nan', False)
        opts.max_nesting = kwargs.get('max_nesting', 64)
        
        doc._doc = yyaml_read(yaml_data, len(yaml_data), &opts, &err)
        
        if doc._doc == NULL:
            raise YYAMLError(err.msg.decode('utf-8'), err.pos, err.line, err.column)
        
        return doc
    
    @staticmethod
    def loads(str yaml_string, **kwargs):
        """Load YAML from string"""
        return YYAMLDocument.load(yaml_string.encode('utf-8'), **kwargs)
    
    @property
    def root(self):
        """Get the root node"""
        if self._doc == NULL:
            return None
        cdef const yyaml_node* root = yyaml_doc_get_root(self._doc)
        if root == NULL:
            return None
        return YYAMLNode(self._doc, root)
    
    def dumps(self, **kwargs):
        """Serialize document to string"""
        if self._doc == NULL:
            return ""
        
        cdef yyaml_write_opts opts
        cdef yyaml_err err
        cdef char* output = NULL
        cdef size_t output_len = 0
        
        # Set options
        opts.indent = kwargs.get('indent', 2)
        opts.final_newline = kwargs.get('final_newline', True)
        
        cdef bool result = yyaml_write(self._doc, &output, &output_len, &opts, &err)
        
        if not result:
            raise YYAMLError(err.msg.decode('utf-8'), err.pos, err.line, err.column)
        
        try:
            if output != NULL:
                return output[:output_len].decode('utf-8')
            return ""
        finally:
            if output != NULL:
                yyaml_free_string(output)
    
    def __len__(self):
        """Number of nodes in the document"""
        if self._doc == NULL:
            return 0
        return yyaml_doc_node_count(self._doc)

# Convenience functions
def load(bytes yaml_data, **kwargs):
    """Load YAML from bytes and return the root node's value"""
    doc = YYAMLDocument.load(yaml_data, **kwargs)
    return doc.root.value if doc.root else None

def loads(str yaml_string, **kwargs):
    """Load YAML from string and return the root node's value"""
    doc = YYAMLDocument.loads(yaml_string, **kwargs)
    return doc.root.value if doc.root else None

# Example usage
if __name__ == "__main__":
    # Simple example
    yaml_data = """
    name: John Doe
    age: 30
    active: true
    scores: [95, 87, 92]
    """
    
    try:
        result = loads(yaml_data)
        print("Parsed YAML:")
        print(result)
        
        # Test round-trip
        doc = YYAMLDocument.loads(yaml_data)
        serialized = doc.dumps()
        print("\nSerialized back:")
        print(serialized)
        
    except YYAMLError as e:
        print(f"Error: {e}")
