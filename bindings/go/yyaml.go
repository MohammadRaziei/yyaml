//go:build !yyaml_no_cgo

// Package yyaml provides Go bindings for the yyaml C library.
package yyaml

/*
#cgo CFLAGS: -I${SRCDIR}/../../yyaml

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

// Include the actual C source files
#include "../../yyaml/yyaml.h"
#include "../../yyaml/yyaml.c"
*/
import "C"
import (
	"errors"
	"fmt"
	"io"
	"runtime"
	"unsafe"
)

// Document represents a YAML document tree.
type Document struct {
	doc *C.yyaml_doc
}

// Node represents a node in a YAML document tree.
type Node struct {
	node *C.yyaml_node
	doc  *Document
}

// Loads parses YAML string into Go interface{}.
func Loads(str string) (interface{}, error) {
	doc, err := parseString(str)
	if err != nil {
		return nil, err
	}
	defer doc.Close()
	return doc.ToInterface(), nil
}

// Load parses YAML from io.Reader into Go interface{}.
func Load(r io.Reader) (interface{}, error) {
	data, err := io.ReadAll(r)
	if err != nil {
		return nil, err
	}
	return Loads(string(data))
}

// Dumps converts Go value to YAML string.
func Dumps(v interface{}) (string, error) {
	doc := fromInterface(v)
	if doc == nil {
		return "", errors.New("failed to create document")
	}
	defer doc.Close()
	return doc.DumpString()
}

// Dump converts Go value to YAML and writes to io.Writer.
func Dump(w io.Writer, v interface{}) error {
	data, err := Dumps(v)
	if err != nil {
		return err
	}
	_, err = w.Write([]byte(data))
	return err
}

// parseString parses YAML string into Document.
func parseString(str string) (*Document, error) {
	var cErr C.yyaml_err
	cStr := C.CString(str)
	defer C.free(unsafe.Pointer(cStr))
	cLen := C.size_t(len(str))

	cDoc := C.yyaml_read(cStr, cLen, nil, &cErr)
	if cDoc == nil {
		return nil, errors.New(C.GoString(&cErr.msg[0]))
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

// fromInterface creates Document from Go interface{}.
func fromInterface(v interface{}) *Document {
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
	
	rootIdx := encodeValue(doc, v)
	if rootIdx == ^uint32(0) || !doc.SetRoot(rootIdx) {
		doc.Close()
		return nil
	}
	
	return doc
}

// encodeValue converts Go value to yyaml node index.
func encodeValue(doc *Document, v interface{}) uint32 {
	switch val := v.(type) {
	case nil:
		return doc.addNull()
	case bool:
		return doc.addBool(val)
	case int:
		return doc.addInt(int64(val))
	case int64:
		return doc.addInt(val)
	case float64:
		return doc.addDouble(val)
	case string:
		return doc.addString(val)
	case []interface{}:
		seqIdx := doc.addSequence()
		if seqIdx == ^uint32(0) {
			return ^uint32(0)
		}
		for _, elem := range val {
			elemIdx := encodeValue(doc, elem)
			if elemIdx == ^uint32(0) || !doc.seqAppend(seqIdx, elemIdx) {
				return ^uint32(0)
			}
		}
		return seqIdx
	case map[string]interface{}:
		mapIdx := doc.addMapping()
		if mapIdx == ^uint32(0) {
			return ^uint32(0)
		}
		for key, elem := range val {
			elemIdx := encodeValue(doc, elem)
			if elemIdx == ^uint32(0) || !doc.mapAppend(mapIdx, key, elemIdx) {
				return ^uint32(0)
			}
		}
		return mapIdx
	default:
		// Try to convert to string
		return doc.addString(fmt.Sprintf("%v", val))
	}
}

// Close frees the document resources.
func (d *Document) Close() {
	if d.doc != nil {
		C.yyaml_doc_free(d.doc)
		d.doc = nil
	}
	runtime.SetFinalizer(d, nil)
}

// ToInterface converts Document to Go interface{}.
func (d *Document) ToInterface() interface{} {
	root := d.root()
	if root == nil {
		return nil
	}
	return root.toInterface()
}

// DumpString serializes Document to YAML string.
func (d *Document) DumpString() (string, error) {
	root := d.root()
	if root == nil {
		return "", errors.New("document has no root")
	}
	
	var cOut *C.char
	var cLen C.size_t
	var cErr C.yyaml_err

	success := C.yyaml_write(root.node, &cOut, &cLen, nil, &cErr)
	if !success {
		return "", errors.New(C.GoString(&cErr.msg[0]))
	}
	if cOut == nil {
		return "", errors.New("yyaml_write returned nil output")
	}

	defer C.yyaml_free_string(cOut)
	return C.GoStringN(cOut, C.int(cLen)), nil
}

// root returns the root node.
func (d *Document) root() *Node {
	cNode := C.yyaml_doc_get_root(d.doc)
	if cNode == nil {
		return nil
	}
	return &Node{node: cNode, doc: d}
}

// toInterface converts Node to Go interface{}.
func (n *Node) toInterface() interface{} {
	if n == nil || n.node == nil {
		return nil
	}
	
	switch n.node._type {
	case C.YYAML_NULL:
		return nil
	case C.YYAML_BOOL:
		// Access union field through unsafe pointer
		return *(*bool)(unsafe.Pointer(&n.node.val[0]))
	case C.YYAML_INT:
		return *(*int64)(unsafe.Pointer(&n.node.val[0]))
	case C.YYAML_DOUBLE:
		return *(*float64)(unsafe.Pointer(&n.node.val[0]))
	case C.YYAML_STRING:
		cBuf := C.yyaml_doc_get_scalar_buf(n.doc.doc)
		if cBuf == nil {
			return ""
		}
		// Access string offset and length from the union
		ofs := *(*uint32)(unsafe.Pointer(&n.node.val[0]))
		length := *(*uint32)(unsafe.Pointer(&n.node.val[4]))
		offset := uintptr(unsafe.Pointer(cBuf)) + uintptr(ofs)
		return C.GoStringN((*C.char)(unsafe.Pointer(offset)), C.int(length))
	case C.YYAML_SEQUENCE:
		length := int(C.yyaml_seq_len(n.node))
		result := make([]interface{}, length)
		for i := 0; i < length; i++ {
			cNode := C.yyaml_seq_get(n.node, C.size_t(i))
			if cNode == nil {
				result[i] = nil
			} else {
				result[i] = (&Node{node: cNode, doc: n.doc}).toInterface()
			}
		}
		return result
	case C.YYAML_MAPPING:
		result := make(map[string]interface{})
		cBuf := C.yyaml_doc_get_scalar_buf(n.doc.doc)
		if cBuf == nil {
			return result
		}
		
		// Get the first child
		if n.node.child == ^C.uint32_t(0) {
			return result
		}
		
		// Iterate through mapping children (key-value pairs)
		// In yyaml, mapping children are stored as alternating key-value pairs
		keyIdx := n.node.child
		for keyIdx != ^C.uint32_t(0) {
			cKey := C.yyaml_doc_get(n.doc.doc, keyIdx)
			if cKey == nil {
				break
			}
			
			// Get the key string
			keyOffset := uintptr(unsafe.Pointer(cBuf)) + uintptr(cKey.extra)
			keyLen := cKey.flags
			key := C.GoStringN((*C.char)(unsafe.Pointer(keyOffset)), C.int(keyLen))
			
			// Get the value (next sibling)
			valIdx := cKey.next
			if valIdx == ^C.uint32_t(0) {
				break
			}
			
			cValue := C.yyaml_doc_get(n.doc.doc, valIdx)
			if cValue != nil {
				result[key] = (&Node{node: cValue, doc: n.doc}).toInterface()
			}
			
			// Move to next key-value pair
			if cValue != nil && cValue.next != ^C.uint32_t(0) {
				keyIdx = cValue.next
			} else {
				break
			}
		}
		return result
	default:
		return nil
	}
}

// Helper methods for Document
func (d *Document) addNull() uint32 {
	return uint32(C.yyaml_doc_add_null(d.doc))
}

func (d *Document) addBool(value bool) uint32 {
	return uint32(C.yyaml_doc_add_bool(d.doc, C.bool(value)))
}

func (d *Document) addInt(value int64) uint32 {
	return uint32(C.yyaml_doc_add_int(d.doc, C.int64_t(value)))
}

func (d *Document) addDouble(value float64) uint32 {
	return uint32(C.yyaml_doc_add_double(d.doc, C.double(value)))
}

func (d *Document) addString(str string) uint32 {
	cStr := C.CString(str)
	defer C.free(unsafe.Pointer(cStr))
	return uint32(C.yyaml_doc_add_string(d.doc, cStr, C.size_t(len(str))))
}

func (d *Document) addSequence() uint32 {
	return uint32(C.yyaml_doc_add_sequence(d.doc))
}

func (d *Document) addMapping() uint32 {
	return uint32(C.yyaml_doc_add_mapping(d.doc))
}

func (d *Document) SetRoot(idx uint32) bool {
	return bool(C.yyaml_doc_set_root(d.doc, C.uint32_t(idx)))
}

func (d *Document) seqAppend(seqIdx, childIdx uint32) bool {
	return bool(C.yyaml_doc_seq_append(d.doc, C.uint32_t(seqIdx), C.uint32_t(childIdx)))
}

func (d *Document) mapAppend(mapIdx uint32, key string, valIdx uint32) bool {
	cKey := C.CString(key)
	defer C.free(unsafe.Pointer(cKey))
	return bool(C.yyaml_doc_map_append(d.doc, C.uint32_t(mapIdx), cKey, C.size_t(len(key)), C.uint32_t(valIdx)))
}