// Example 4: Convert JSON to YAML and print both
// To run: go run sample04_json_to_yaml.go
package main

import (
	"encoding/json"
	"fmt"
	"log"
	
	yyaml "github.com/mohammadraziei/yyaml/bindings/go"
)

// JSONExample represents a JSON example with name and content
type JSONExample struct {
	Name    string
	JSONStr string
}

// processExample processes a JSON example and prints results
func processExample(ex JSONExample) {
	fmt.Printf("\n>> %s:\n", ex.Name)
	fmt.Println(">>   Original JSON:")
	fmt.Printf("   %s\n", ex.JSONStr)
	
	// Parse JSON
	var data interface{}
	if err := json.Unmarshal([]byte(ex.JSONStr), &data); err != nil {
		log.Printf("Failed to parse JSON: %v", err)
		return
	}
	
	// Convert to YAML
	yamlOutput, err := yyaml.Dumps(data)
	if err != nil {
		log.Printf("Failed to convert to YAML: %v", err)
		return
	}
	
	fmt.Println("\n   Converted YAML:")
	fmt.Println(yamlOutput)
	
	// Dump back to JSON with beautiful formatting
	jsonPretty, err := json.MarshalIndent(data, "", "    ")
	if err != nil {
		log.Printf("Failed to convert back to JSON: %v", err)
		return
	}
	
	fmt.Println("\n   Dumped back to JSON (beautified with 4-space indentation):")
	fmt.Println(string(jsonPretty))
	
	// Also show compact JSON dump
	jsonCompact, err := json.Marshal(data)
	if err != nil {
		log.Printf("Failed to convert to compact JSON: %v", err)
		return
	}
	
	fmt.Println("\n>>   Dumped back to JSON (compact):")
	fmt.Printf("   %s\n", string(jsonCompact))
	
	// Show JSON with 2-space indentation (alternative formatting)
	jsonPretty2, _ := json.MarshalIndent(data, "", "  ")
	fmt.Println("\n   Dumped back to JSON (beautified with 2-space indentation):")
	fmt.Println(string(jsonPretty2))
}

func main() {
	fmt.Println("=== JSON to YAML Conversion Example ===\n")
	
	// List of JSON examples to process
	examples := []JSONExample{
		{
			Name: "1. Compact JSON to YAML Conversion",
			JSONStr: `{"name":"John Doe","age":30,"active":true,"hobbies":["reading","hiking","coding"],"address":{"city":"San Francisco","zip":94107}}`,
		},
		{
			Name: "2. Complex JSON with Nested Structures",
			JSONStr: `{"users":[{"id":1,"name":"Alice","roles":["admin","user"],"metadata":{"created":"2024-01-01","active":true,"permissions":{"read":true,"write":true,"delete":false}}},{"id":2,"name":"Bob","roles":["user"],"metadata":{"created":"2024-01-02","active":false,"permissions":{"read":true,"write":false,"delete":false}}}],"settings":{"version":"2.0.0","features":{"logging":true,"debug":false,"max_connections":100},"timeouts":{"api":"30s","database":"10s","cache":"5s"}}}`,
		},
		{
			Name: "3. Roundtrip JSON -> YAML -> JSON",
			JSONStr: `{"product":{"name":"Laptop","price":1299.99,"in_stock":true,"specs":{"cpu":"Intel i7","ram":"16GB","storage":"512GB SSD"},"tags":["electronics","computers","portable"]}}`,
		},
		{
			Name: "4. JSON with All Data Types",
			JSONStr: `{"string":"hello world","integer":42,"float":3.14159,"boolean_true":true,"boolean_false":false,"null_value":null,"array":[1,"two",3.0,true,null,{"nested":"object"}],"object":{"nested_string":"nested","nested_number":123,"deeply_nested":{"level":3,"active":true}},"empty_string":"","zero":0,"empty_array":[],"empty_object":{}}`,
		},
	}
	
	// Process all examples
	for _, ex := range examples {
		processExample(ex)
	}
	
	// Special example: Error handling
	fmt.Println("\n>> 5. Error Handling with Invalid JSON:")
	invalidJSON := `{"name":"John","age":thirty,"address":{"city":"New York"}}`
	fmt.Println(">>   Invalid JSON:")
	fmt.Printf("   %s\n", invalidJSON)
	
	var invalidData interface{}
	if err := json.Unmarshal([]byte(invalidJSON), &invalidData); err != nil {
		fmt.Printf("   Expected JSON parsing error: %v\n", err)
		
		// Try with valid JSON instead for demonstration
		validJSON := `{"name":"John","age":30}`
		json.Unmarshal([]byte(validJSON), &invalidData)
		yamlOutput, _ := yyaml.Dumps(invalidData)
		fmt.Println("\n   Valid JSON converted to YAML instead:")
		fmt.Println(yamlOutput)
	}
	
	// Special example: Format comparison
	fmt.Println("\n>> 6. Side-by-Side Comparison:")
	comparisonJSON := `{"api":{"version":"v1","endpoints":[{"path":"/users","method":"GET"},{"path":"/users/{id}","method":"GET"},{"path":"/users","method":"POST"}],"security":{"enabled":true,"type":"jwt"}}}`
	
	fmt.Println("   JSON Format (compact):")
	fmt.Printf("   %s\n", comparisonJSON)
	
	var comparisonData interface{}
	json.Unmarshal([]byte(comparisonJSON), &comparisonData)
	comparisonYAML, _ := yyaml.Dumps(comparisonData)
	
	fmt.Println("\n   YAML Format:")
	fmt.Println(comparisonYAML)
	
	// Show pretty JSON for comparison
	comparisonPretty, _ := json.MarshalIndent(comparisonData, "", "  ")
	fmt.Println("\n   JSON Format (pretty):")
	fmt.Println(string(comparisonPretty))
	
	fmt.Println("\n   Observations:")
	fmt.Println("   - YAML is more human-readable for configuration")
	fmt.Println("   - JSON is better for data interchange")
	fmt.Println("   - Both preserve the same data structure")
	fmt.Println("   - YAML supports comments (not shown in this conversion)")
	
	fmt.Println("\n=== JSON to YAML conversion example completed ===")
}