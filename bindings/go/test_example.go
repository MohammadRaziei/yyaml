package yyaml

import (
	"fmt"
	"strings"
)

func Example() {
	// Example 1: Parse YAML
	yamlStr := `name: Alice
age: 30
hobbies:
  - reading
  - hiking
  - coding
address:
  city: San Francisco
  zip: 94107
`
	
	result, err := Loads(yamlStr)
	if err != nil {
		fmt.Printf("Error: %v\n", err)
		return
	}
	
	fmt.Printf("Parsed YAML: %v\n", result)
	
	// Example 2: Generate YAML
	data := map[string]interface{}{
		"name": "Bob",
		"age":  25,
		"skills": []interface{}{"Go", "Python", "JavaScript"},
		"employed": true,
		"salary": 85000.50,
	}
	
	output, err := Dumps(data)
	if err != nil {
		fmt.Printf("Error: %v\n", err)
		return
	}
	
	fmt.Printf("\nGenerated YAML:\n%s\n", output)
	
	// Example 3: Roundtrip
	roundtrip, err := Loads(output)
	if err != nil {
		fmt.Printf("Error: %v\n", err)
		return
	}
	
	fmt.Printf("\nRoundtrip successful: %v\n", roundtrip != nil)
	
	// Check if output contains expected strings
	if strings.Contains(output, "name: Bob") &&
		strings.Contains(output, "age: 25") &&
		strings.Contains(output, "- Go") {
		fmt.Println("Output contains expected content")
	}
}