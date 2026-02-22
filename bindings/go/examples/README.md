# YYAML Go Examples

This directory contains example programs demonstrating how to use the YYAML Go package.

## Prerequisites

1. Install Go (version 1.21 or later)
2. Install GCC (required for CGo compilation)
3. Clone the repository or install the package:

```bash
go get github.com/mohammadraziei/yyaml/bindings/
```

## Running the Examples

Each example can be run independently:

```bash
# Navigate to examples directory
cd bindings/go/examples

# Run basic example
go run sample01_basic.go

# Run advanced example
go run sample02_advanced.go

# Run file operations example
go run sample03_file_operations.go

# Run JSON to YAML conversion example
go run sample04_json_to_yaml.go

# Run YAML to JSON conversion example
go run sample05_yaml_to_json.go
```

## Example Overview

### 1. `sample01_basic.go` - Basic Usage
Demonstrates the fundamental operations:
- Parsing YAML strings with `Loads()`
- Generating YAML from Go data structures with `Dumps()`
- Simple roundtrip testing
- Basic configuration examples

**Key Features:**
- Simple YAML parsing and generation
- Working with maps, arrays, and nested structures
- Type preservation (integers, floats, booleans, strings)

### 2. `sample02_advanced.go` - Advanced Features
Shows more complex scenarios:
- Error handling with invalid YAML
- Complex nested data structures
- Data type preservation and validation
- Configuration file simulation
- Working with empty and null values

**Key Features:**
- Error handling examples
- Complex nested object hierarchies
- Type checking and validation
- Configuration modification

### 3. `sample03_file_operations.go` - File Operations
Focuses on real-world file handling:
- Reading and writing YAML files
- Configuration management
- Merging multiple configuration files
- File validation and error handling
- Temporary file operations

**Key Features:**
- File I/O operations with YAML
- Configuration merging strategies
- File validation and error handling
- Real-world configuration examples

### 4. `sample04_json_to_yaml.go` - JSON to YAML Conversion
Demonstrates interoperability between JSON and YAML:
- Converting compact JSON strings to YAML format
- Roundtrip conversion (JSON -> YAML -> JSON)
- Type preservation across formats
- Error handling with invalid JSON
- Side-by-side comparison of JSON and YAML formats

**Key Features:**
- Uses clean, refactored code with structs and loops
- Processes multiple JSON examples in a list
- Shows both compact and pretty JSON output
- Demonstrates data structure preservation

### 5. `sample05_yaml_to_json.go` - YAML to JSON Conversion
Demonstrates the reverse conversion process:
- Parsing YAML strings with comments and formatting
- Converting YAML to JSON with different formatting options
- Roundtrip conversion (YAML -> JSON -> YAML)
- Handling of YAML comments (and their loss during parsing)
- Comparison of YAML and JSON representations

**Key Features:**
- Shows YAML parsing with complex structures
- Demonstrates comment handling in YAML
- Provides side-by-side comparison of formats
- Includes roundtrip validation
- Uses independent example counter system

## Common Patterns

### Parsing YAML
```go
import yyaml "github.com/mohammadraziei/yyaml/bindings/go"

yamlStr := `name: John
age: 30
active: true`

result, err := yyaml.Loads(yamlStr)
if err != nil {
    log.Fatal(err)
}
```

### Generating YAML
```go
data := map[string]interface{}{
    "name": "Alice",
    "age": 25,
    "skills": []interface{}{"Go", "Python"},
}

yamlOutput, err := yyaml.Dumps(data)
if err != nil {
    log.Fatal(err)
}
```

### Working with Files
```go
// Read from file
content, _ := ioutil.ReadFile("config.yaml")
config, _ := yyaml.Loads(string(content))

// Write to file
yamlContent, _ := yyaml.Dumps(config)
ioutil.WriteFile("output.yaml", []byte(yamlContent), 0644)
```

## Data Type Mapping

| YAML Type | Go Type (parsed) | Notes |
|-----------|------------------|-------|
| Scalar (string) | `string` | UTF-8 strings |
| Scalar (integer) | `int64` | 64-bit integers |
| Scalar (float) | `float64` | Double precision |
| Scalar (boolean) | `bool` | True/false |
| Scalar (null) | `nil` | Null values |
| Sequence | `[]interface{}` | Arrays/lists |
| Mapping | `map[string]interface{}` | Objects/dictionaries |

## Error Handling

The YYAML package returns errors for:
- Invalid YAML syntax
- Type conversion failures
- Memory allocation issues
- File I/O errors (when using file operations)

Always check errors:
```go
result, err := yyaml.Loads(invalidYAML)
if err != nil {
    fmt.Printf("Error: %v\n", err)
    // Handle error appropriately
}
```

## Best Practices

1. **Always validate input**: Check for errors after parsing YAML
2. **Use type assertions carefully**: When accessing parsed data, use type assertions with checks
3. **Handle null values**: Check for `nil` values in parsed data
4. **Clean up resources**: Use `defer` for file operations
5. **Test edge cases**: Test with empty strings, null values, and nested structures

## Troubleshooting

### Common Issues

1. **"could not determine kind of name for C.xxx"**: Ensure GCC is installed and CGo can compile the C code
2. **Type assertion panics**: Always check types with `ok` pattern: `if m, ok := value.(map[string]interface{}); ok { ... }`
3. **Memory issues**: The package handles C memory automatically, but large documents may require optimization

### Getting Help

- Check the [main documentation](../../README.md)
- Review the [source code](../yyaml.go)
- Run the tests: `go test ../tests/go/...`

## License

These examples are part of the YYAML project. See the main project for license information.