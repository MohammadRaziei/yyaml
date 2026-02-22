// Basic example showing how to use yyaml Go package with standard Marshal/Unmarshal API
// To run: go run sample01_basic.go
package main

import (
	"fmt"
	"log"
	
	yyaml "github.com/mohammadraziei/yyaml/bindings/go"
)

func main() {
	// Example 1: Parse YAML string using Unmarshal
	yamlStr := `name: John Doe
age: 30
active: true
hobbies:
  - reading
  - hiking
  - coding
address:
  city: San Francisco
  zip: 94107
`
	
	fmt.Println("=== Example 1: Parsing YAML with Unmarshal ===")
	var result interface{}
	err := yyaml.Unmarshal([]byte(yamlStr), &result)
	if err != nil {
		log.Fatalf("Failed to parse YAML: %v", err)
	}
	
	fmt.Printf("Parsed result: %v\n", result)
	fmt.Println()
	
	// Example 2: Create data and convert to YAML using Marshal
	fmt.Println("=== Example 2: Generating YAML with Marshal ===")
	data := map[string]interface{}{
		"name": "Alice Smith",
		"age":  25,
		"skills": []interface{}{"Go", "Python", "JavaScript"},
		"employed": true,
		"salary": 85000.50,
		"projects": []interface{}{
			map[string]interface{}{
				"name": "Web API",
				"status": "completed",
			},
			map[string]interface{}{
				"name": "Mobile App",
				"status": "in-progress",
			},
		},
	}
	
	output, err := yyaml.Marshal(data)
	if err != nil {
		log.Fatalf("Failed to generate YAML: %v", err)
	}
	
	fmt.Println("Generated YAML:")
	fmt.Println(string(output))
	
	// Example 3: Roundtrip test with Marshal/Unmarshal
	fmt.Println("=== Example 3: Roundtrip Test ===")
	var parsedBack interface{}
	err = yyaml.Unmarshal(output, &parsedBack)
	if err != nil {
		log.Fatalf("Failed to parse generated YAML: %v", err)
	}
	
	fmt.Printf("Successfully parsed back: %v\n", parsedBack != nil)
	
	// Example 4: Working with configuration data
	fmt.Println("\n=== Example 4: Configuration Example ===")
	
	// Create configuration data
	configData := map[string]interface{}{
		"config": map[string]interface{}{
			"version": "1.0.0",
			"debug": false,
			"port": 8080,
		},
		"database": map[string]interface{}{
			"host": "localhost",
			"port": 5432,
			"name": "mydb",
		},
	}
	
	yamlOutput, _ := yyaml.Marshal(configData)
	fmt.Println("Sample configuration:")
	fmt.Println(string(yamlOutput))
	
	// Example 5: Type demonstration
	fmt.Println("\n=== Example 5: Type Preservation ===")
	typeDemo := map[string]interface{}{
		"integer": 42,
		"float":   3.14159,
		"boolean": true,
		"null":    nil,
		"string":  "Hello, World!",
		"array":   []interface{}{1, "two", 3.0, true},
	}
	
	typeOutput, _ := yyaml.Marshal(typeDemo)
	fmt.Println("Type-preserving YAML:")
	fmt.Println(string(typeOutput))
}
