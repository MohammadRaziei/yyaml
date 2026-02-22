// Example 5: Convert YAML to JSON and print both
// To run: go run sample05_yaml_to_json.go
package main

import (
	"encoding/json"
	"fmt"
	"log"
	
	yyaml "github.com/mohammadraziei/yyaml/bindings/go"
)

func main() {
	fmt.Println("=== YAML to JSON Conversion Example ===\n")
	
	// Counter for examples
	exampleCounter := 1
	
	// Example 1: Simple YAML to JSON Conversion
	fmt.Printf("\n[Example %d] Simple YAML to JSON Conversion\n", exampleCounter)
	exampleCounter++
	
	yamlStr1 := `name: John Doe
age: 30
active: true
hobbies:
  - reading
  - hiking
  - coding
address:
  city: San Francisco
  zip: 94107`
	
	fmt.Println("Original YAML input:")
	fmt.Println(yamlStr1)
	
	data1, err := yyaml.Loads(yamlStr1)
	if err != nil {
		log.Printf("ERROR: Failed to parse YAML: %v", err)
	} else {
		// Dump back to YAML
		yamlOutput1, err := yyaml.Dumps(data1)
		if err != nil {
			log.Printf("ERROR: Failed to convert back to YAML: %v", err)
		} else {
			fmt.Println("\n--- YAML OUTPUT (re-generated) ---")
			fmt.Println(yamlOutput1)
			fmt.Println("--- END YAML ---")
		}
		
		// Convert to JSON with different formatting
		jsonPretty1, _ := json.MarshalIndent(data1, "", "    ")
		fmt.Println("\n--- JSON OUTPUT (beautified, 4-space) ---")
		fmt.Println(string(jsonPretty1))
		fmt.Println("--- END JSON ---")
		
		jsonPretty2_1, _ := json.MarshalIndent(data1, "", "  ")
		fmt.Println("\n--- JSON OUTPUT (beautified, 2-space) ---")
		fmt.Println(string(jsonPretty2_1))
		fmt.Println("--- END JSON ---")
	}
	
	// Example 2: Complex YAML with Nested Structures
	fmt.Printf("\n[Example %d] Complex YAML with Nested Structures\n", exampleCounter)
	exampleCounter++
	
	yamlStr2 := `users:
  - id: 1
    name: Alice
    roles:
      - admin
      - user
    metadata:
      created: "2024-01-01"
      active: true
      permissions:
        read: true
        write: true
        delete: false
  - id: 2
    name: Bob
    roles:
      - user
    metadata:
      created: "2024-01-02"
      active: false
      permissions:
        read: true
        write: false
        delete: false
settings:
  version: "2.0.0"
  features:
    logging: true
    debug: false
    max_connections: 100
  timeouts:
    api: "30s"
    database: "10s"
    cache: "5s"`
	
	fmt.Println("Original YAML input:")
	fmt.Println(yamlStr2)
	
	data2, err := yyaml.Loads(yamlStr2)
	if err != nil {
		log.Printf("ERROR: Failed to parse YAML: %v", err)
	} else {
		// Dump back to YAML
		yamlOutput2, err := yyaml.Dumps(data2)
		if err != nil {
			log.Printf("ERROR: Failed to convert back to YAML: %v", err)
		} else {
			fmt.Println("\n--- YAML OUTPUT (re-generated) ---")
			fmt.Println(yamlOutput2)
			fmt.Println("--- END YAML ---")
		}
		
		// Convert to JSON with different formatting
		jsonPretty2, _ := json.MarshalIndent(data2, "", "    ")
		fmt.Println("\n--- JSON OUTPUT (beautified, 4-space) ---")
		fmt.Println(string(jsonPretty2))
		fmt.Println("--- END JSON ---")
		
		jsonPretty2_2, _ := json.MarshalIndent(data2, "", "  ")
		fmt.Println("\n--- JSON OUTPUT (beautified, 2-space) ---")
		fmt.Println(string(jsonPretty2_2))
		fmt.Println("--- END JSON ---")
	}
	
	// Example 3: YAML with All Data Types
	fmt.Printf("\n[Example %d] YAML with All Data Types\n", exampleCounter)
	exampleCounter++
	
	yamlStr3 := `string: "hello world"
integer: 42
float: 3.14159
boolean_true: true
boolean_false: false
null_value: null
array:
  - 1
  - "two"
  - 3.0
  - true
  - null
  - nested: object
object:
  nested_string: nested
  nested_number: 123
  deeply_nested:
    level: 3
    active: true
empty_string: ""
zero: 0
empty_array: []
empty_object: {}`
	
	fmt.Println("Original YAML input:")
	fmt.Println(yamlStr3)
	
	data3, err := yyaml.Loads(yamlStr3)
	if err != nil {
		log.Printf("ERROR: Failed to parse YAML: %v", err)
	} else {
		// Dump back to YAML
		yamlOutput3, err := yyaml.Dumps(data3)
		if err != nil {
			log.Printf("ERROR: Failed to convert back to YAML: %v", err)
		} else {
			fmt.Println("\n--- YAML OUTPUT (re-generated) ---")
			fmt.Println(yamlOutput3)
			fmt.Println("--- END YAML ---")
		}
		
		// Convert to JSON with different formatting
		jsonPretty3, _ := json.MarshalIndent(data3, "", "    ")
		fmt.Println("\n--- JSON OUTPUT (beautified, 4-space) ---")
		fmt.Println(string(jsonPretty3))
		fmt.Println("--- END JSON ---")
		
		jsonPretty2_3, _ := json.MarshalIndent(data3, "", "  ")
		fmt.Println("\n--- JSON OUTPUT (beautified, 2-space) ---")
		fmt.Println(string(jsonPretty2_3))
		fmt.Println("--- END JSON ---")
	}
	
	// Example 4: YAML with Comments and Special Formatting
	fmt.Printf("\n[Example %d] YAML with Comments and Special Formatting\n", exampleCounter)
	exampleCounter++
	
	yamlStr4 := `# API Configuration
api:
  version: "v1"
  endpoints:
    - path: "/users"
      method: GET
      description: "Get all users"
    - path: "/users/{id}"
      method: GET
      description: "Get user by ID"
    - path: "/users"
      method: POST
      description: "Create new user"
  
  # Security settings
  security:
    enabled: true
    type: jwt
    token_expiry: "24h"
  
  # Rate limiting
  rate_limit:
    requests_per_minute: 100
    burst_size: 20
  
# Database configuration
database:
  host: "localhost"
  port: 5432
  name: "myapp"
  pool:
    max_connections: 20
    min_connections: 5
    connection_timeout: "30s"
  
# Logging
logging:
  level: "info"
  format: "json"
  output: "stdout"`
	
	fmt.Println("Original YAML input (with comments):")
	fmt.Println(yamlStr4)
	
	data4, err := yyaml.Loads(yamlStr4)
	if err != nil {
		log.Printf("ERROR: Failed to parse YAML: %v", err)
	} else {
		// Note: Comments are not preserved in parsed data
		fmt.Println("\nNote: YAML comments are not preserved in parsed data structure")
		
		// Dump back to YAML (comments will be lost)
		yamlOutput4, err := yyaml.Dumps(data4)
		if err != nil {
			log.Printf("ERROR: Failed to convert back to YAML: %v", err)
		} else {
			fmt.Println("\n--- YAML OUTPUT (re-generated, comments lost) ---")
			fmt.Println(yamlOutput4)
			fmt.Println("--- END YAML ---")
		}
		
		// Convert to JSON
		jsonPretty4, _ := json.MarshalIndent(data4, "", "    ")
		fmt.Println("\n--- JSON OUTPUT (beautified, 4-space) ---")
		fmt.Println(string(jsonPretty4))
		fmt.Println("--- END JSON ---")
		
		jsonCompact4, _ := json.Marshal(data4)
		fmt.Println("\n--- JSON OUTPUT (compact) ---")
		fmt.Printf("%s\n", string(jsonCompact4))
		fmt.Println("--- END JSON ---")
	}
	
	// Example 5: Roundtrip YAML -> JSON -> YAML
	fmt.Printf("\n[Example %d] Roundtrip YAML -> JSON -> YAML\n", exampleCounter)
	exampleCounter++
	
	yamlStr5 := `product:
  name: "Laptop"
  price: 1299.99
  in_stock: true
  specs:
    cpu: "Intel i7"
    ram: "16GB"
    storage: "512GB SSD"
  tags:
    - electronics
    - computers
    - portable`
	
	fmt.Println("Original YAML input:")
	fmt.Println(yamlStr5)
	
	data5, err := yyaml.Loads(yamlStr5)
	if err != nil {
		log.Printf("ERROR: Failed to parse YAML: %v", err)
	} else {
		// First convert to JSON
		jsonOutput5, _ := json.MarshalIndent(data5, "", "  ")
		fmt.Println("\n--- Intermediate JSON ---")
		fmt.Println(string(jsonOutput5))
		fmt.Println("--- END JSON ---")
		
		// Parse JSON back
		var jsonData interface{}
		if err := json.Unmarshal(jsonOutput5, &jsonData); err != nil {
			log.Printf("ERROR: Failed to parse JSON back: %v", err)
		} else {
			// Convert back to YAML
			finalYAML, err := yyaml.Dumps(jsonData)
			if err != nil {
				log.Printf("ERROR: Failed to convert back to YAML: %v", err)
			} else {
				fmt.Println("\n--- Final YAML (after roundtrip) ---")
				fmt.Println(finalYAML)
				fmt.Println("--- END YAML ---")
			}
		}
	}
	
	fmt.Println("\nObservations:")
	fmt.Println("- YAML is more human-readable with comments and formatting")
	fmt.Println("- JSON is more compact for data interchange")
	fmt.Println("- Both formats preserve the same data structure")
	fmt.Println("- YAML comments are lost during parsing")
	fmt.Println("- Roundtrip conversion preserves data integrity")
	
	fmt.Println("\n=== YAML to JSON conversion example completed ===")
}