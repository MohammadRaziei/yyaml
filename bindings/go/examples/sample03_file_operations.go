// Example 3: File operations with YAML using Marshal/Unmarshal API
// To run: go run sample03_file_operations.go
package main

import (
	"fmt"
	"io/ioutil"
	"log"
	"os"
	
	yyaml "github.com/mohammadraziei/yyaml/bindings/go"
)

func main() {
	fmt.Println("=== YAML File Operations Example with Marshal/Unmarshal ===\n")
	
	// Example 1: Create and write YAML to a file using Marshal
	fmt.Println("1. Creating and writing YAML to a file:")
	
	configData := map[string]interface{}{
		"application": map[string]interface{}{
			"name": "MyApp",
			"version": "1.0.0",
			"environment": "development",
		},
		"server": map[string]interface{}{
			"host": "localhost",
			"port": 8080,
			"timeout": "30s",
		},
		"database": map[string]interface{}{
			"driver": "postgres",
			"host": "localhost",
			"port": 5432,
			"name": "mydb",
			"username": "admin",
			"password": "secret",
		},
		"features": map[string]interface{}{
			"logging": true,
			"metrics": true,
			"tracing": false,
		},
	}
	
	// Convert to YAML using Marshal
	yamlContent, err := yyaml.Marshal(configData)
	if err != nil {
		log.Fatalf("Failed to generate YAML: %v", err)
	}
	
	// Write to a temporary file
	tmpFile, err := ioutil.TempFile("", "config-*.yaml")
	if err != nil {
		log.Fatalf("Failed to create temp file: %v", err)
	}
	defer os.Remove(tmpFile.Name())
	
	if _, err := tmpFile.Write(yamlContent); err != nil {
		log.Fatalf("Failed to write to file: %v", err)
	}
	tmpFile.Close()
	
	fmt.Printf("   Created config file: %s\n", tmpFile.Name())
	fmt.Println("   File content:")
	fmt.Println(string(yamlContent))
	
	// Example 2: Read and parse YAML from a file using Unmarshal
	fmt.Println("\n2. Reading and parsing YAML from a file:")
	
	// Read the file back
	fileContent, err := ioutil.ReadFile(tmpFile.Name())
	if err != nil {
		log.Fatalf("Failed to read file: %v", err)
	}
	
	// Parse the YAML using Unmarshal
	var parsedConfig interface{}
	err = yyaml.Unmarshal(fileContent, &parsedConfig)
	if err != nil {
		log.Fatalf("Failed to parse YAML from file: %v", err)
	}
	
	fmt.Println("   Successfully parsed configuration from file")
	
	// Access specific values
	if configMap, ok := parsedConfig.(map[string]interface{}); ok {
		app := configMap["application"].(map[string]interface{})
		server := configMap["server"].(map[string]interface{})
		
		fmt.Printf("   Application: %s v%s\n", app["name"], app["version"])
		fmt.Printf("   Server: %s:%v\n", server["host"], server["port"])
	}
	
	// Example 3: Modify configuration and save back
	fmt.Println("\n3. Modifying configuration and saving back:")
	
	if configMap, ok := parsedConfig.(map[string]interface{}); ok {
		// Modify values
		server := configMap["server"].(map[string]interface{})
		server["port"] = 9090
		server["host"] = "0.0.0.0"
		
		// Add new section
		configMap["cache"] = map[string]interface{}{
			"enabled": true,
			"type": "redis",
			"host": "cache.example.com",
			"port": 6379,
		}
		
		// Update features
		features := configMap["features"].(map[string]interface{})
		features["tracing"] = true
		features["rate_limiting"] = true
	}
	
	// Convert back to YAML using Marshal
	updatedYAML, err := yyaml.Marshal(parsedConfig)
	if err != nil {
		log.Fatalf("Failed to generate updated YAML: %v", err)
	}
	
	// Write to a new file
	updatedFile, err := ioutil.TempFile("", "config-updated-*.yaml")
	if err != nil {
		log.Fatalf("Failed to create updated temp file: %v", err)
	}
	defer os.Remove(updatedFile.Name())
	
	if _, err := updatedFile.Write(updatedYAML); err != nil {
		log.Fatalf("Failed to write updated file: %v", err)
	}
	updatedFile.Close()
	
	fmt.Printf("   Updated config saved to: %s\n", updatedFile.Name())
	fmt.Println("   Updated file content:")
	fmt.Println(string(updatedYAML))
	
	// Example 4: Working with multiple configuration files
	fmt.Println("\n4. Merging multiple configuration files:")
	
	// Create base config
	baseConfig := map[string]interface{}{
		"app": map[string]interface{}{
			"name": "MyApp",
			"debug": false,
		},
		"common": map[string]interface{}{
			"timeout": "30s",
			"retries": 3,
		},
	}
	
	// Create environment-specific config
	envConfig := map[string]interface{}{
		"app": map[string]interface{}{
			"debug": true,  // Override
		},
		"database": map[string]interface{}{
			"host": "localhost",
			"port": 5432,
		},
		"environment": "development",  // New key
	}
	
	// Merge configurations
	mergedConfig := make(map[string]interface{})
	
	// Simple merge implementation
	for key, value := range baseConfig {
		mergedConfig[key] = value
	}
	
	for key, value := range envConfig {
		if existing, ok := mergedConfig[key]; ok {
			// If both are maps, merge them
			if destMap, ok := existing.(map[string]interface{}); ok {
				if srcMap, ok := value.(map[string]interface{}); ok {
					// Merge nested maps
					for k, v := range srcMap {
						destMap[k] = v
					}
					continue
				}
			}
		}
		mergedConfig[key] = value
	}
	
	mergedYAML, _ := yyaml.Marshal(mergedConfig)
	fmt.Println("   Merged configuration:")
	fmt.Println(string(mergedYAML))
	
	// Example 5: Validating configuration
	fmt.Println("\n5. Configuration validation:")
	
	// Parse the merged config using Unmarshal
	var validatedConfig interface{}
	err = yyaml.Unmarshal(mergedYAML, &validatedConfig)
	if err != nil {
		fmt.Printf("   Validation failed: %v\n", err)
	} else {
		fmt.Println("   Configuration is valid YAML")
		
		// Check required fields
		if configMap, ok := validatedConfig.(map[string]interface{}); ok {
			requiredFields := []string{"app", "environment"}
			missingFields := []string{}
			
			for _, field := range requiredFields {
				if _, exists := configMap[field]; !exists {
					missingFields = append(missingFields, field)
				}
			}
			
			if len(missingFields) == 0 {
				fmt.Println("   All required fields are present")
			} else {
				fmt.Printf("   Missing required fields: %v\n", missingFields)
			}
		}
	}
	
	// Example 6: Error handling with files
	fmt.Println("\n6. Error handling with file operations:")
	
	// Try to parse invalid file content
	invalidContent := `name: John
age: thirty  # Invalid: not a number
address:
  city: New York
  # Missing closing quote
  street: "123 Main Street
`
	
	invalidFile, err := ioutil.TempFile("", "invalid-*.yaml")
	if err != nil {
		log.Fatalf("Failed to create invalid temp file: %v", err)
	}
	defer os.Remove(invalidFile.Name())
	
	if _, err := invalidFile.Write([]byte(invalidContent)); err != nil {
		log.Fatalf("Failed to write invalid file: %v", err)
	}
	invalidFile.Close()
	
	// Try to parse the invalid file using Unmarshal
	invalidFileContent, _ := ioutil.ReadFile(invalidFile.Name())
	var invalidResult interface{}
	parseErr := yyaml.Unmarshal(invalidFileContent, &invalidResult)
	
	if parseErr != nil {
		fmt.Printf("   Expected error when parsing invalid YAML: %v\n", parseErr)
	} else {
		fmt.Println("   Unexpected: No error when parsing invalid YAML")
	}
	
	fmt.Println("\n=== File operations example completed ===")
}
