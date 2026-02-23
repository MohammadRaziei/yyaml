// Advanced example showing error handling and complex operations with Marshal/Unmarshal API
// To run: go run sample02_advanced.go
package main

import (
	"fmt"
	"log"

	yyaml "github.com/mohammadraziei/yyaml"
)

func main() {
	fmt.Println("=== Advanced YAML Examples with Marshal/Unmarshal ===\n")

	// Example 1: Error handling with Unmarshal
	fmt.Println("1. Error Handling Examples:")

	// Invalid YAML
	invalidYAML := `name: John
age: thirty  # This is not a number
`
	fmt.Println("   a) Parsing invalid YAML with Unmarshal:")
	var invalidResult interface{}
	err := yyaml.Unmarshal([]byte(invalidYAML), &invalidResult)
	if err != nil {
		fmt.Printf("      Error: %v\n", err)
	} else {
		fmt.Println("      No error (unexpected)")
	}

	// Valid YAML
	validYAML := `name: John
age: 30
`
	fmt.Println("   b) Parsing valid YAML with Unmarshal:")
	var validResult interface{}
	err = yyaml.Unmarshal([]byte(validYAML), &validResult)
	if err != nil {
		fmt.Printf("      Error: %v\n", err)
	} else {
		fmt.Printf("      Success: %v\n", validResult)
	}

	// Example 2: Complex nested structures with Marshal
	fmt.Println("\n2. Complex Nested Structures:")

	complexData := map[string]interface{}{
		"users": []interface{}{
			map[string]interface{}{
				"id":    1,
				"name":  "Alice",
				"roles": []interface{}{"admin", "user"},
				"metadata": map[string]interface{}{
					"created": "2024-01-01",
					"active":  true,
				},
			},
			map[string]interface{}{
				"id":    2,
				"name":  "Bob",
				"roles": []interface{}{"user"},
				"metadata": map[string]interface{}{
					"created": "2024-01-02",
					"active":  false,
				},
			},
		},
		"settings": map[string]interface{}{
			"version": "2.0.0",
			"features": map[string]interface{}{
				"logging":         true,
				"debug":           false,
				"max_connections": 100,
			},
		},
	}

	yamlOutput, err := yyaml.Marshal(complexData)
	if err != nil {
		log.Fatalf("Failed to generate YAML: %v", err)
	}

	fmt.Println("   Generated YAML:")
	fmt.Println(string(yamlOutput))

	// Example 3: Working with different data types
	fmt.Println("\n3. Data Type Preservation:")

	typeTestData := map[string]interface{}{
		"integer":    42,
		"float":      3.14159,
		"boolean":    true,
		"null_value": nil,
		"string":     "hello world",
		"array":      []interface{}{1, "two", 3.0, true, nil},
	}

	typeTestYAML, _ := yyaml.Marshal(typeTestData)
	var parsedBack interface{}
	yyaml.Unmarshal(typeTestYAML, &parsedBack)

	fmt.Println("   Original types:")
	for k, v := range typeTestData {
		fmt.Printf("      %s: %v (%T)\n", k, v, v)
	}

	fmt.Println("\n   Parsed back types:")
	if m, ok := parsedBack.(map[string]interface{}); ok {
		for k, v := range m {
			fmt.Printf("      %s: %v (%T)\n", k, v, v)
		}
	}

	// Example 4: File operations simulation
	fmt.Println("\n4. File Operations Simulation:")

	// Simulate reading from a file
	configYAML := `server:
  host: "0.0.0.0"
  port: 8080
  ssl: true
  timeout: 30s

database:
  host: "db.example.com"
  port: 5432
  name: "production"
  user: "admin"

logging:
  level: "info"
  file: "/var/log/app.log"
  max_size: "100MB"
`

	fmt.Println("   Parsing configuration file content:")
	var config interface{}
	err = yyaml.Unmarshal([]byte(configYAML), &config)
	if err != nil {
		log.Fatalf("Failed to parse config: %v", err)
	}

	if configMap, ok := config.(map[string]interface{}); ok {
		fmt.Printf("   Server host: %v\n", configMap["server"].(map[string]interface{})["host"])
		fmt.Printf("   Database name: %v\n", configMap["database"].(map[string]interface{})["name"])
		fmt.Printf("   Logging level: %v\n", configMap["logging"].(map[string]interface{})["level"])
	}

	// Example 5: Modifying and saving configuration
	fmt.Println("\n5. Modifying Configuration:")

	// Modify the config
	if configMap, ok := config.(map[string]interface{}); ok {
		server := configMap["server"].(map[string]interface{})
		server["port"] = 9090
		server["timeout"] = "60s"

		logging := configMap["logging"].(map[string]interface{})
		logging["level"] = "debug"

		// Add new section
		configMap["cache"] = map[string]interface{}{
			"enabled": true,
			"size":    "512MB",
			"ttl":     "1h",
		}
	}

	updatedYAML, _ := yyaml.Marshal(config)
	fmt.Println("   Updated configuration:")
	fmt.Println(string(updatedYAML))

	// Example 6: Empty and null values
	fmt.Println("\n6. Empty and Null Values:")

	emptyData := map[string]interface{}{
		"empty_string": "",
		"null_value":   nil,
		"empty_array":  []interface{}{},
		"empty_map":    map[string]interface{}{},
		"zero":         0,
		"false_value":  false,
	}

	emptyYAML, _ := yyaml.Marshal(emptyData)
	fmt.Println("   YAML with empty values:")
	fmt.Println(string(emptyYAML))

	// Parse it back
	var parsedEmpty interface{}
	yyaml.Unmarshal(emptyYAML, &parsedEmpty)
	fmt.Println("\n   Parsed back:")
	fmt.Printf("   %v\n", parsedEmpty)

	// Example 7: Performance demonstration
	fmt.Println("\n7. Performance Demonstration:")

	// Create a large data structure
	largeData := make(map[string]interface{})
	for i := 0; i < 100; i++ {
		key := fmt.Sprintf("item_%d", i)
		largeData[key] = map[string]interface{}{
			"id":     i,
			"name":   fmt.Sprintf("Item %d", i),
			"value":  float64(i) * 1.5,
			"active": i%2 == 0,
		}
	}

	fmt.Println("   Marshaling large data structure...")
	largeYAML, err := yyaml.Marshal(largeData)
	if err != nil {
		fmt.Printf("   Error: %v\n", err)
	} else {
		fmt.Printf("   Success! Generated %d bytes of YAML\n", len(largeYAML))

		// Unmarshal it back
		var largeParsed interface{}
		fmt.Println("   Unmarshaling back...")
		err = yyaml.Unmarshal(largeYAML, &largeParsed)
		if err != nil {
			fmt.Printf("   Error: %v\n", err)
		} else {
			fmt.Println("   Successfully unmarshaled large data structure!")
		}
	}

	fmt.Println("\n=== Examples completed successfully ===")
}
