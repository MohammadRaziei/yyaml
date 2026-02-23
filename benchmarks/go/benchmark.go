package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"
	"runtime"
	"sort"
	"strings"
	"time"

	"github.com/goccy/go-yaml"
	yyaml "github.com/mohammadraziei/yyaml"
	jsonschema "github.com/santhosh-tekuri/jsonschema/v5"
	goyaml "gopkg.in/yaml.v3"
)

type BenchmarkResult struct {
	Library      string
	Operation    string
	TotalFiles   int
	TotalTime    time.Duration
	AvgTime      time.Duration
	SuccessCount int
	ErrorCount   int
	Throughput   float64 // files per second
}

type LibraryBenchmark interface {
	Name() string
	Unmarshal(data []byte) (interface{}, error)
	Marshal(data interface{}) ([]byte, error)
}

// YAML v3 implementation
type YAMLv3 struct{}

func (y *YAMLv3) Name() string { return "gopkg.in/yaml.v3" }

func (y *YAMLv3) Unmarshal(data []byte) (interface{}, error) {
	var result interface{}
	err := goyaml.Unmarshal(data, &result)
	return result, err
}

func (y *YAMLv3) Marshal(data interface{}) ([]byte, error) {
	return goyaml.Marshal(data)
}

// GoCcy YAML implementation
type GoCcyYAML struct{}

func (g *GoCcyYAML) Name() string { return "github.com/goccy/go-yaml" }

func (g *GoCcyYAML) Unmarshal(data []byte) (interface{}, error) {
	var result interface{}
	err := yaml.Unmarshal(data, &result)
	return result, err
}

func (g *GoCcyYAML) Marshal(data interface{}) ([]byte, error) {
	return yaml.Marshal(data)
}

// JSONSchema implementation (uses YAML internally)
type JSONSchemaYAML struct{}

func (j *JSONSchemaYAML) Name() string { return "github.com/santhosh-tekuri/jsonschema/v5" }

func (j *JSONSchemaYAML) Unmarshal(data []byte) (interface{}, error) {
	// jsonschema library doesn't have direct YAML parsing
	// We'll convert YAML to JSON first using our own library
	var yamlData interface{}
	err := yyaml.Unmarshal(data, &yamlData)
	if err != nil {
		return nil, err
	}

	// Then validate with JSON schema (dummy schema that accepts anything)
	compiler := jsonschema.NewCompiler()
	schema := `{"type": "object"}`
	if err := compiler.AddResource("test", strings.NewReader(schema)); err != nil {
		return nil, err
	}

	// Convert to JSON for validation
	jsonData, err := json.Marshal(yamlData)
	if err != nil {
		return nil, err
	}

	// Validate (this is just to test performance)
	s, err := compiler.Compile("test")
	if err != nil {
		return nil, err
	}

	var v interface{}
	if err := json.Unmarshal(jsonData, &v); err != nil {
		return nil, err
	}

	if err := s.Validate(v); err != nil {
		return nil, err
	}

	return yamlData, nil
}

func (j *JSONSchemaYAML) Marshal(data interface{}) ([]byte, error) {
	// Convert to JSON first, then to YAML using our library
	jsonData, err := json.Marshal(data)
	if err != nil {
		return nil, err
	}

	var jsonObj interface{}
	if err := json.Unmarshal(jsonData, &jsonObj); err != nil {
		return nil, err
	}

	return yyaml.Marshal(jsonObj)
}

// YYAML implementation (our library)
type YYAML struct{}

func (y *YYAML) Name() string { return "github.com/mohammadraziei/yyaml" }

func (y *YYAML) Unmarshal(data []byte) (interface{}, error) {
	var result interface{}
	err := yyaml.Unmarshal(data, &result)
	return result, err
}

func (y *YYAML) Marshal(data interface{}) ([]byte, error) {
	return yyaml.Marshal(data)
}

func collectYAMLFiles(dir string) ([]string, error) {
	var files []string

	err := filepath.Walk(dir, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		if !info.IsDir() && strings.HasSuffix(strings.ToLower(path), ".yaml") {
			files = append(files, path)
		}

		return nil
	})

	return files, err
}

func runBenchmark(lib LibraryBenchmark, files []string, operation string) BenchmarkResult {
	start := time.Now()
	success := 0
	errors := 0

	for _, file := range files {
		data, err := ioutil.ReadFile(file)
		if err != nil {
			log.Printf("Error reading file %s: %v", file, err)
			errors++
			continue
		}

		if operation == "unmarshal" {
			_, err = lib.Unmarshal(data)
		} else if operation == "marshal" {
			// First unmarshal to get data structure
			parsed, err := lib.Unmarshal(data)
			if err != nil {
				errors++
				continue
			}

			// Then marshal it back
			_, err = lib.Marshal(parsed)
		} else if operation == "roundtrip" {
			parsed, err := lib.Unmarshal(data)
			if err != nil {
				errors++
				continue
			}

			marshaled, err := lib.Marshal(parsed)
			if err != nil {
				errors++
				continue
			}

			// Parse again to verify
			_, err = lib.Unmarshal(marshaled)
		}

		if err != nil {
			errors++
			log.Printf("Error processing file %s with %s: %v", file, lib.Name(), err)
		} else {
			success++
		}
	}

	totalTime := time.Since(start)
	avgTime := time.Duration(0)
	if success > 0 {
		avgTime = totalTime / time.Duration(success)
	}

	throughput := 0.0
	if totalTime > 0 {
		throughput = float64(success) / totalTime.Seconds()
	}

	return BenchmarkResult{
		Library:      lib.Name(),
		Operation:    operation,
		TotalFiles:   len(files),
		TotalTime:    totalTime,
		AvgTime:      avgTime,
		SuccessCount: success,
		ErrorCount:   errors,
		Throughput:   throughput,
	}
}

func printResults(results []BenchmarkResult) {
	fmt.Println("\n" + strings.Repeat("=", 100))
	fmt.Println("YAML LIBRARY BENCHMARK RESULTS")
	fmt.Println(strings.Repeat("=", 100))

	// Group by operation
	operations := make(map[string][]BenchmarkResult)
	for _, r := range results {
		operations[r.Operation] = append(operations[r.Operation], r)
	}

	for op, opResults := range operations {
		fmt.Printf("\nOperation: %s\n", strings.ToUpper(op))
		fmt.Println(strings.Repeat("-", 100))
		fmt.Printf("%-40s %12s %12s %12s %12s %12s\n",
			"Library", "Total Time", "Avg Time", "Success", "Errors", "Throughput")
		fmt.Println(strings.Repeat("-", 100))

		// Sort by total time (fastest first)
		sort.Slice(opResults, func(i, j int) bool {
			return opResults[i].TotalTime < opResults[j].TotalTime
		})

		for _, r := range opResults {
			fmt.Printf("%-40s %12v %12v %12d %12d %12.2f files/s\n",
				r.Library,
				r.TotalTime.Round(time.Millisecond),
				r.AvgTime.Round(time.Microsecond),
				r.SuccessCount,
				r.ErrorCount,
				r.Throughput)
		}
	}

	fmt.Println("\n" + strings.Repeat("=", 100))
	fmt.Println("SYSTEM INFORMATION")
	fmt.Println(strings.Repeat("-", 100))
	fmt.Printf("Go Version: %s\n", runtime.Version())
	fmt.Printf("OS: %s\n", runtime.GOOS)
	fmt.Printf("Arch: %s\n", runtime.GOARCH)
	fmt.Printf("CPU Cores: %d\n", runtime.NumCPU())
	fmt.Println(strings.Repeat("=", 100))
}

func main() {
	yamlDir := "../data/yaml-test-suite/src"

	// Check if directory exists
	if _, err := os.Stat(yamlDir); os.IsNotExist(err) {
		log.Fatalf("YAML test suite directory does not exist: %s", yamlDir)
	}

	fmt.Printf("Scanning YAML files in: %s\n", yamlDir)

	files, err := collectYAMLFiles(yamlDir)
	if err != nil {
		log.Fatalf("Error scanning YAML files: %v", err)
	}

	if len(files) == 0 {
		log.Fatal("No YAML files found in the test suite directory")
	}

	fmt.Printf("Found %d YAML files for benchmarking\n", len(files))

	// Limit to first 50 files for faster benchmarking
	if len(files) > 50 {
		fmt.Printf("Limiting to first 50 files for faster benchmarking\n")
		files = files[:50]
	}

	// Initialize libraries
	libraries := []LibraryBenchmark{
		&YAMLv3{},
		&GoCcyYAML{},
		&JSONSchemaYAML{},
		&YYAML{},
	}

	operations := []string{"unmarshal", "marshal", "roundtrip"}

	var allResults []BenchmarkResult

	fmt.Println("\nStarting benchmarks...")
	fmt.Println(strings.Repeat("-", 100))

	for _, lib := range libraries {
		fmt.Printf("\nBenchmarking: %s\n", lib.Name())

		for _, op := range operations {
			fmt.Printf("  Running %s... ", op)
			result := runBenchmark(lib, files, op)
			allResults = append(allResults, result)
			fmt.Printf("Done (Success: %d/%d, Time: %v)\n",
				result.SuccessCount, result.TotalFiles, result.TotalTime.Round(time.Millisecond))
		}
	}

	printResults(allResults)

	// Save results to JSON file
	jsonData, err := json.MarshalIndent(allResults, "", "  ")
	if err != nil {
		log.Printf("Error marshaling results to JSON: %v", err)
	} else {
		if err := ioutil.WriteFile("benchmark_results.json", jsonData, 0644); err != nil {
			log.Printf("Error writing results to file: %v", err)
		} else {
			fmt.Printf("\nResults saved to: benchmark_results.json\n")
		}
	}

	// Generate summary markdown
	generateSummary(allResults, files)
}

func generateSummary(results []BenchmarkResult, files []string) {
	summary := "# YAML Library Benchmark Results\n\n"
	summary += fmt.Sprintf("**Date:** %s\n", time.Now().Format("2006-01-02 15:04:05"))
	summary += fmt.Sprintf("**Total Files:** %d\n", len(files))
	summary += fmt.Sprintf("**Go Version:** %s\n\n", runtime.Version())

	// Group by operation
	operations := make(map[string][]BenchmarkResult)
	for _, r := range results {
		operations[r.Operation] = append(operations[r.Operation], r)
	}

	for op, opResults := range operations {
		summary += fmt.Sprintf("## %s\n\n", strings.ToUpper(op))
		summary += "| Library | Total Time | Avg Time | Success | Errors | Throughput |\n"
		summary += "|---------|------------|----------|---------|--------|------------|\n"

		// Sort by total time (fastest first)
		sort.Slice(opResults, func(i, j int) bool {
			return opResults[i].TotalTime < opResults[j].TotalTime
		})

		for _, r := range opResults {
			summary += fmt.Sprintf("| %s | %v | %v | %d | %d | %.2f files/s |\n",
				r.Library,
				r.TotalTime.Round(time.Millisecond),
				r.AvgTime.Round(time.Microsecond),
				r.SuccessCount,
				r.ErrorCount,
				r.Throughput)
		}
		summary += "\n"
	}

	// Add performance comparison
	summary += "## Performance Comparison\n\n"

	for op, opResults := range operations {
		if len(opResults) < 2 {
			continue
		}

		sort.Slice(opResults, func(i, j int) bool {
			return opResults[i].TotalTime < opResults[j].TotalTime
		})

		fastest := opResults[0]
		summary += fmt.Sprintf("### %s\n\n", strings.ToUpper(op))
		summary += fmt.Sprintf("**Fastest:** %s (%v)\n\n", fastest.Library, fastest.TotalTime.Round(time.Millisecond))
		summary += "| Library | Relative Speed | Difference |\n"
		summary += "|---------|----------------|------------|\n"

		for i, r := range opResults {
			if i == 0 {
				summary += fmt.Sprintf("| %s | 1.00x | - |\n", r.Library)
			} else {
				relative := float64(r.TotalTime) / float64(fastest.TotalTime)
				diff := r.TotalTime - fastest.TotalTime
				summary += fmt.Sprintf("| %s | %.2fx | +%v |\n",
					r.Library, relative, diff.Round(time.Millisecond))
			}
		}
		summary += "\n"
	}

	if err := ioutil.WriteFile("BENCHMARK_SUMMARY.md", []byte(summary), 0644); err != nil {
		log.Printf("Error writing summary: %v", err)
	} else {
		fmt.Printf("Summary saved to: BENCHMARK_SUMMARY.md\n")
	}
}
