// Package yyaml provides Go bindings for the yyaml C library.
package yyaml

// #include "clib.go"
import "C"
import (
	"errors"
	"runtime"
	"unsafe"
)

// Type represents the type of a YAML node.
type Type int

const (
	TypeNull     Type = C.YYAML_NULL
	TypeBool     Type = C.YYAML_BOOL
	TypeInt      Type = C.YYAML_INT
	TypeDouble   Type = C.YYAML_DOUBLE
	TypeString   Type = C.YYAML_STRING
	TypeSequence Type = C.YYAML_SEQUENCE
	TypeMapping  Type = C.YYAML_MAPPING
)

// ReadOptions configures the YAML parser behavior.
type ReadOptions struct {
	AllowDuplicateKeys   bool
	AllowTrailingContent bool
	AllowInfNan          bool
	MaxNesting           int
}

// WriteOptions configures the YAML writer behavior.
type WriteOptions struct {
	Indent       int
	FinalNewline bool
}

// Error contains details about a parsing or writing error.
type Error struct {
	Pos    int
	Line   int
	Column int
	Msg    string
}

func (e *Error) Error() string {
	return e.Msg
}

// Document represents a YAML document tree.
type Document struct {
	doc *C.yyaml_doc
}

// Node represents a node in a YAML document tree.
type Node struct {
	node *C.yyaml_node
	doc  *Document
}

// Read parses YAML text into a document tree.
func Read(data []byte, opts *ReadOptions) (*Document, error) {
	var cOpts *C.yyaml_read_opts
	if opts != nil {
		cOpts = &C.yyaml_read_opts{
			allow_duplicate_keys:   C.bool(opts.AllowDuplicateKeys),
			allow_trailing_content: C.bool(opts.AllowTrailingContent),
			allow_inf_nan:          C.bool(opts.AllowInfNan),
			max_nesting:            C.size_t(opts.MaxNesting),
		}
	}

	var cErr C.yyaml_err
	cData := (*C.char)(unsafe.Pointer(&data[0]))
	cLen := C.size_t(len(data))

	cDoc := C.yyaml_read(cData, cLen, cOpts, &cErr)
	if cDoc == nil {
		return nil, &Error{
			Pos:    int(cErr.pos),
			Line:   int(cErr.line),
			Column: int(cErr.column),
			Msg:    C.GoString(&cErr.msg[0]),
		}
	}

	doc := &Document{doc: cDoc}
	runtime.SetFinalizer(doc, func(d *Document) {
		if d.doc != nil {
			C.yyaml_doc_free(d.doc)
			d.doc = nil
		}
	})
	return doc, nil
}

// ReadString parses YAML text from a string into a document tree.
func ReadString(str string, opts *ReadOptions) (*Document, error) {
	return Read([]byte(str), opts)
}

// NewDocument creates an empty document for manual construction.
func NewDocument() *Document {
	cDoc := C.yyaml_doc_new()
	if cDoc == nil {
		return nil
	}
	doc := &Document{doc: cDoc}
	runtime.SetFinalizer(doc, func(d *Document) {
		if d.doc != nil {
			C.yyaml_doc_free(d.doc)
			d.doc = nil
		}
	})
	return doc
}

// Close frees the document resources.
func (d *Document) Close() {
	if d.doc != nil {
		C.yyaml_doc_free(d.doc)
		d.doc = nil
	}
	runtime.SetFinalizer(d, nil)
}

// Root returns the root node of the document.
func (d *Document) Root() *Node {
	cNode := C.yyaml_doc_get_root(d.doc)
	if cNode == nil {
		return nil
	}
	return &Node{node: cNode, doc: d}
}

// NodeCount returns the total number of nodes in the document.
func (d *Document) NodeCount() int {
	return int(C.yyaml_doc_node_count(d.doc))
}

// GetNode returns a node by its index within the document.
func (d *Document) GetNode(idx int) *Node {
	cNode := C.yyaml_doc_get(d.doc, C.uint32_t(idx))
	if cNode == nil {
		return nil
	}
	return &Node{node: cNode, doc: d}
}

// Type returns the type of the node.
func (n *Node) Type() Type {
	return Type(n.node._type)
}

// IsScalar returns true if the node is a scalar type.
func (n *Node) IsScalar() bool {
	return bool(C.yyaml_is_scalar(n.node))
}

// IsContainer returns true if the node is a sequence or mapping.
func (n *Node) IsContainer() bool {
	return bool(C.yyaml_is_container(n.node))
}

// Bool returns the boolean value for a boolean node.
func (n *Node) Bool() bool {
	if n.Type() != TypeBool {
		return false
	}
	return bool(n.node.val.boolean)
}

// Int returns the integer value for an integer node.
func (n *Node) Int() int64 {
	if n.Type() != TypeInt {
		return 0
	}
	return int64(n.node.val.integer)
}

// Double returns the floating-point value for a double node.
func (n *Node) Double() float64 {
	if n.Type() != TypeDouble {
		return 0
	}
	return float64(n.node.val.real)
}

// String returns the string value for a string node.
func (n *Node) String() string {
	if n.Type() != TypeString {
		return ""
	}
	cBuf := C.yyaml_doc_get_scalar_buf(n.doc.doc)
	if cBuf == nil {
		return ""
	}
	offset := uintptr(unsafe.Pointer(cBuf)) + uintptr(n.node.val.str.ofs)
	return C.GoString((*C.char)(unsafe.Pointer(offset)))
}

// MapGet looks up a mapping value by key.
func (n *Node) MapGet(key string) *Node {
	if n.Type() != TypeMapping {
		return nil
	}
	cKey := C.CString(key)
	defer C.free(unsafe.Pointer(cKey))
	cNode := C.yyaml_map_get(n.node, cKey)
	if cNode == nil {
		return nil
	}
	return &Node{node: cNode, doc: n.doc}
}

// SeqGet retrieves a sequence element by index.
func (n *Node) SeqGet(index int) *Node {
	if n.Type() != TypeSequence {
		return nil
	}
	cNode := C.yyaml_seq_get(n.node, C.size_t(index))
	if cNode == nil {
		return nil
	}
	return &Node{node: cNode, doc: n.doc}
}

// SeqLen returns the number of elements in a sequence.
func (n *Node) SeqLen() int {
	if n.Type() != TypeSequence {
		return 0
	}
	return int(C.yyaml_seq_len(n.node))
}

// MapLen returns the number of members in a mapping.
func (n *Node) MapLen() int {
	if n.Type() != TypeMapping {
		return 0
	}
	return int(C.yyaml_map_len(n.node))
}

// Write serializes a node tree to YAML text.
func Write(root *Node, opts *WriteOptions) ([]byte, error) {
	var cOpts *C.yyaml_write_opts
	if opts != nil {
		cOpts = &C.yyaml_write_opts{
			indent:        C.size_t(opts.Indent),
			final_newline: C.bool(opts.FinalNewline),
		}
	}

	var cOut *C.char
	var cLen C.size_t
	var cErr C.yyaml_err

	success := C.yyaml_write(root.node, &cOut, &cLen, cOpts, &cErr)
	if !success {
		return nil, &Error{
			Pos:    int(cErr.pos),
			Line:   int(cErr.line),
			Column: int(cErr.column),
			Msg:    C.GoString(&cErr.msg[0]),
		}
	}
	if cOut == nil {
		return nil, errors.New("yyaml_write returned nil output")
	}

	defer C.yyaml_free_string(cOut)
	return C.GoBytes(unsafe.Pointer(cOut), C.int(cLen)), nil
}

// WriteString serializes a node tree to a YAML string.
func WriteString(root *Node, opts *WriteOptions) (string, error) {
	data, err := Write(root, opts)
	if err != nil {
		return "", err
	}
	return string(data), nil
}