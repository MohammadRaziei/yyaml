// Example 4: Convert JSON to YAML and print both
// To run: go run sample04_json_to_yaml.go
package main

import (
	"encoding/json"
	"fmt"
	"log"

	yyaml "github.com/mohammadraziei/yyaml"
)

func main() {
	fmt.Println("=== JSON to YAML Conversion Example ===\n")

	// Counter for examples
	exampleCounter := 1

	// Example 1: Compact JSON to YAML Conversion
	fmt.Printf("\n[Example %d] Compact JSON to YAML Conversion\n", exampleCounter)
	exampleCounter++

	jsonStr1 := `{"name":"John Doe","age":30,"active":true,"hobbies":["reading","hiking","coding"],"address":{"city":"San Francisco","zip":94107}}`
	fmt.Println("Original JSON input:")
	fmt.Printf("%s\n", jsonStr1)

	var data1 interface{}
	if err := json.Unmarshal([]byte(jsonStr1), &data1); err != nil {
		log.Printf("ERROR: Failed to parse JSON: %v", err)
	} else {
		yamlOutput1, err := yyaml.Marshal(data1)
		if err != nil {
			log.Printf("ERROR: Failed to convert to YAML: %v", err)
		} else {
			fmt.Println("\n--- YAML OUTPUT ---")
			fmt.Println(string(yamlOutput1))
			fmt.Println("--- END YAML ---")

			jsonPretty1, _ := json.MarshalIndent(data1, "", "    ")
			fmt.Println("\n--- JSON OUTPUT (beautified, 4-space) ---")
			fmt.Println(string(jsonPretty1))
			fmt.Println("--- END JSON ---")

			jsonPretty2_1, _ := json.MarshalIndent(data1, "", "  ")
			fmt.Println("\n--- JSON OUTPUT (beautified, 2-space) ---")
			fmt.Println(string(jsonPretty2_1))
			fmt.Println("--- END JSON ---")
		}
	}

	// Example 2: Complex JSON with Nested Structures
	fmt.Printf("\n[Example %d] Complex JSON with Nested Structures\n", exampleCounter)
	exampleCounter++

	jsonStr2 := `{"users":[{"id":1,"name":"Alice","roles":["admin","user"],"metadata":{"created":"2024-01-01","active":true,"permissions":{"read":true,"write":true,"delete":false}}},{"id":2,"name":"Bob","roles":["user"],"metadata":{"created":"2024-01-02","active":false,"permissions":{"read":true,"write":false,"delete":false}}}],"settings":{"version":"2.0.0","features":{"logging":true,"debug":false,"max_connections":100},"timeouts":{"api":"30s","database":"10s","cache":"5s"}}}`
	fmt.Println("Original JSON input:")
	fmt.Printf("%s\n", jsonStr2)

	var data2 interface{}
	if err := json.Unmarshal([]byte(jsonStr2), &data2); err != nil {
		log.Printf("ERROR: Failed to parse JSON: %v", err)
	} else {
		yamlOutput2, err := yyaml.Marshal(data2)
		if err != nil {
			log.Printf("ERROR: Failed to convert to YAML: %v", err)
		} else {
			fmt.Println("\n--- YAML OUTPUT ---")
			fmt.Println(string(yamlOutput2))
			fmt.Println("--- END YAML ---")

			jsonPretty2, _ := json.MarshalIndent(data2, "", "    ")
			fmt.Println("\n--- JSON OUTPUT (beautified, 4-space) ---")
			fmt.Println(string(jsonPretty2))
			fmt.Println("--- END JSON ---")

			jsonPretty2_2, _ := json.MarshalIndent(data2, "", "  ")
			fmt.Println("\n--- JSON OUTPUT (beautified, 2-space) ---")
			fmt.Println(string(jsonPretty2_2))
			fmt.Println("--- END JSON ---")
		}
	}

	// Example 3: Roundtrip JSON -> YAML -> JSON
	fmt.Printf("\n[Example %d] Roundtrip JSON -> YAML -> JSON\n", exampleCounter)
	exampleCounter++

	jsonStr3 := `{"product":{"name":"Laptop","price":1299.99,"in_stock":true,"specs":{"cpu":"Intel i7","ram":"16GB","storage":"512GB SSD"},"tags":["electronics","computers","portable"]}}`
	fmt.Println("Original JSON input:")
	fmt.Printf("%s\n", jsonStr3)

	var data3 interface{}
	if err := json.Unmarshal([]byte(jsonStr3), &data3); err != nil {
		log.Printf("ERROR: Failed to parse JSON: %v", err)
	} else {
		yamlOutput3, err := yyaml.Marshal(data3)
		if err != nil {
			log.Printf("ERROR: Failed to convert to YAML: %v", err)
		} else {
			fmt.Println("\n--- YAML OUTPUT ---")
			fmt.Println(string(yamlOutput3))
			fmt.Println("--- END YAML ---")

			jsonPretty3, _ := json.MarshalIndent(data3, "", "    ")
			fmt.Println("\n--- JSON OUTPUT (beautified, 4-space) ---")
			fmt.Println(string(jsonPretty3))
			fmt.Println("--- END JSON ---")

			jsonPretty2_3, _ := json.MarshalIndent(data3, "", "  ")
			fmt.Println("\n--- JSON OUTPUT (beautified, 2-space) ---")
			fmt.Println(string(jsonPretty2_3))
			fmt.Println("--- END JSON ---")
		}
	}

	// Example 4: JSON with All Data Types
	fmt.Printf("\n[Example %d] JSON with All Data Types\n", exampleCounter)
	exampleCounter++

	jsonStr4 := `{"string":"hello world","integer":42,"float":3.14159,"boolean_true":true,"boolean_false":false,"null_value":null,"array":[1,"two",3.0,true,null,{"nested":"object"}],"object":{"nested_string":"nested","nested_number":123,"deeply_nested":{"level":3,"active":true}},"empty_string":"","zero":0,"empty_array":[],"empty_object":{}}`
	fmt.Println("Original JSON input:")
	fmt.Printf("%s\n", jsonStr4)

	var data4 interface{}
	if err := json.Unmarshal([]byte(jsonStr4), &data4); err != nil {
		log.Printf("ERROR: Failed to parse JSON: %v", err)
	} else {
		yamlOutput4, err := yyaml.Marshal(data4)
		if err != nil {
			log.Printf("ERROR: Failed to convert to YAML: %v", err)
		} else {
			fmt.Println("\n--- YAML OUTPUT ---")
			fmt.Println(string(yamlOutput4))
			fmt.Println("--- END YAML ---")

			jsonPretty4, _ := json.MarshalIndent(data4, "", "    ")
			fmt.Println("\n--- JSON OUTPUT (beautified, 4-space) ---")
			fmt.Println(string(jsonPretty4))
			fmt.Println("--- END JSON ---")

			jsonPretty2_4, _ := json.MarshalIndent(data4, "", "  ")
			fmt.Println("\n--- JSON OUTPUT (beautified, 2-space) ---")
			fmt.Println(string(jsonPretty2_4))
			fmt.Println("--- END JSON ---")
		}
	}

	// Example 5: Side-by-Side Comparison
	fmt.Printf("\n[Example %d] Side-by-Side Comparison\n", exampleCounter)
	exampleCounter++

	jsonStr5 := `{"api":{"version":"v1","endpoints":[{"path":"/users","method":"GET"},{"path":"/users/{id}","method":"GET"},{"path":"/users","method":"POST"}],"security":{"enabled":true,"type":"jwt"}}}`
	fmt.Println("Original JSON input:")
	fmt.Printf("%s\n", jsonStr5)

	var data5 interface{}
	if err := json.Unmarshal([]byte(jsonStr5), &data5); err != nil {
		log.Printf("ERROR: Failed to parse JSON: %v", err)
	} else {
		yamlOutput5, err := yyaml.Marshal(data5)
		if err != nil {
			log.Printf("ERROR: Failed to convert to YAML: %v", err)
		} else {
			fmt.Println("\n--- YAML OUTPUT ---")
			fmt.Println(string(yamlOutput5))
			fmt.Println("--- END YAML ---")

			jsonPretty5, _ := json.MarshalIndent(data5, "", "    ")
			fmt.Println("\n--- JSON OUTPUT (beautified, 4-space) ---")
			fmt.Println(string(jsonPretty5))
			fmt.Println("--- END JSON ---")

			jsonPretty2_5, _ := json.MarshalIndent(data5, "", "  ")
			fmt.Println("\n--- JSON OUTPUT (beautified, 2-space) ---")
			fmt.Println(string(jsonPretty2_5))
			fmt.Println("--- END JSON ---")
		}
	}

	fmt.Println("\n=== JSON to YAML conversion example completed ===")
}
