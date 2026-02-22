package yyaml_test

import (
	"strings"
	"testing"
	
	yyaml "github.com/mohammadraziei/yyaml/bindings/go"
)

func TestUnmarshalSimple(t *testing.T) {
	yamlStr := `name: John
age: 30
active: true
`
	
	// Test Unmarshal API
	var result interface{}
	err := yyaml.Unmarshal([]byte(yamlStr), &result)
	if err != nil {
		t.Fatalf("Unmarshal failed: %v", err)
	}
	
	m, ok := result.(map[string]interface{})
	if !ok {
		t.Fatalf("Expected map, got %T", result)
	}
	
	if m["name"] != "John" {
		t.Errorf("Expected name=John, got %v", m["name"])
	}
	
	// yyaml distinguishes between integers and floats
	if m["age"] != int64(30) {
		t.Errorf("Expected age=30, got %v", m["age"])
	}
	
	if m["active"] != true {
		t.Errorf("Expected active=true, got %v", m["active"])
	}
}

func TestUnmarshalArray(t *testing.T) {
	yamlStr := `- apple
- banana
- cherry
`
	
	// Test Unmarshal API
	var result interface{}
	err := yyaml.Unmarshal([]byte(yamlStr), &result)
	if err != nil {
		t.Fatalf("Unmarshal failed: %v", err)
	}
	
	arr, ok := result.([]interface{})
	if !ok {
		t.Fatalf("Expected array, got %T", result)
	}
	
	if len(arr) != 3 {
		t.Errorf("Expected 3 items, got %d", len(arr))
	}
	
	if arr[0] != "apple" || arr[1] != "banana" || arr[2] != "cherry" {
		t.Errorf("Expected [apple banana cherry], got %v", arr)
	}
}

func TestMarshalSimple(t *testing.T) {
	data := map[string]interface{}{
		"name":   "Alice",
		"age":    25,
		"active": false,
	}
	
	// Test Marshal API
	output, err := yyaml.Marshal(data)
	if err != nil {
		t.Fatalf("Marshal failed: %v", err)
	}
	
	outputStr := string(output)
	if !strings.Contains(outputStr, "name: Alice") {
		t.Errorf("Expected output to contain 'name: Alice', got:\n%s", outputStr)
	}
	
	if !strings.Contains(outputStr, "age: 25") {
		t.Errorf("Expected output to contain 'age: 25', got:\n%s", outputStr)
	}
}

func TestRoundtrip(t *testing.T) {
	original := map[string]interface{}{
		"string": "hello",
		"number": 42,
		"float":  3.14,
		"bool":   true,
		"null":   nil,
		"array":  []interface{}{1, "two", 3.0},
		"object": map[string]interface{}{
			"nested": "value",
		},
	}
	
	// Convert to YAML using Marshal
	yamlData, err := yyaml.Marshal(original)
	if err != nil {
		t.Fatalf("Marshal failed: %v", err)
	}
	
	// Parse back using Unmarshal
	var result interface{}
	err = yyaml.Unmarshal(yamlData, &result)
	if err != nil {
		t.Fatalf("Unmarshal failed: %v", err)
	}
	
	// Verify structure
	m, ok := result.(map[string]interface{})
	if !ok {
		t.Fatalf("Expected map after roundtrip, got %T", result)
	}
	
	if m["string"] != "hello" {
		t.Errorf("Expected string=hello, got %v", m["string"])
	}
	
	// 42 is an integer, should be parsed as int64
	if m["number"] != int64(42) {
		t.Errorf("Expected number=42, got %v", m["number"])
	}
	
	if m["float"] != 3.14 {
		t.Errorf("Expected float=3.14, got %v", m["float"])
	}
	
	if m["bool"] != true {
		t.Errorf("Expected bool=true, got %v", m["bool"])
	}
	
	// Note: nil becomes nil in the map
	if m["null"] != nil {
		t.Errorf("Expected null=nil, got %v", m["null"])
	}
}

func TestEmptyDocument(t *testing.T) {
	// Empty document should return nil
	var result interface{}
	err := yyaml.Unmarshal([]byte(""), &result)
	if err != nil {
		t.Fatalf("Unmarshal empty failed: %v", err)
	}
	
	if result != nil {
		t.Errorf("Expected nil for empty document, got %v", result)
	}
}

func TestNestedStructures(t *testing.T) {
	yamlStr := `users:
  - name: Alice
    age: 25
  - name: Bob
    age: 30
settings:
  theme: dark
  notifications: true
`
	
	var result interface{}
	err := yyaml.Unmarshal([]byte(yamlStr), &result)
	if err != nil {
		t.Fatalf("Unmarshal failed: %v", err)
	}
	
	m, ok := result.(map[string]interface{})
	if !ok {
		t.Fatalf("Expected map, got %T", result)
	}
	
	users, ok := m["users"].([]interface{})
	if !ok {
		t.Fatalf("Expected users array, got %T", m["users"])
	}
	
	if len(users) != 2 {
		t.Errorf("Expected 2 users, got %d", len(users))
	}
	
	settings, ok := m["settings"].(map[string]interface{})
	if !ok {
		t.Fatalf("Expected settings map, got %T", m["settings"])
	}
	
	if settings["theme"] != "dark" {
		t.Errorf("Expected theme=dark, got %v", settings["theme"])
	}
}
