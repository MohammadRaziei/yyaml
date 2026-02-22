// Basic example showing how to use yyaml Go package
// To run: go run basic_example.go
package main

import (
	"fmt"
	"log"
	
	yyaml "github.com/mohammadraziei/yyaml/bindings/go"
)

func main() {
	// Example 1: Parse YAML string
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
	
	fmt.Println("=== Example 1: Parsing YAML ===")
	result, err := yyaml.Loads(yamlStr)
	if err != nil {
		log.Fatalf("Failed to parse YAML: %v", err)
	}
	
	fmt.Printf("Parsed result: %v\n", result)
	fmt.Println()
	
	// Example 2: Create data and convert to YAML
	fmt.Println("=== Example 2: Generating YAML ===")
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
	
	output, err := yyaml.Dumps(data)
	if err != nil {
		log.Fatalf("Failed to generate YAML: %v", err)
	}
	
	fmt.Println("Generated YAML:")
	fmt.Println(output)
	
	// Example 3: Roundtrip test
	fmt.Println("=== Example 3: Roundtrip Test ===")
	parsedBack, err := yyaml.Loads(output)
	if err != nil {
		log.Fatalf("Failed to parse generated YAML: %v", err)
	}
	
	fmt.Printf("Successfully parsed back: %v\n", parsedBack != nil)
	
	// Example 4: Working with files
	fmt.Println("\n=== Example 4: File Operations ===")
	
	// Create a temporary file
	tempData := map[string]interface{}{
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
	
	yamlOutput, _ := yyaml.Dumps(tempData)
	fmt.Println("Sample configuration:")
	fmt.Println(yamlOutput)
}