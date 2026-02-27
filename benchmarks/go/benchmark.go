package main

import (
	"encoding/json"
	"fmt"
	"io"
	"log"
	"os"
	"path/filepath"
	"runtime"
	"sort"
	"strings"
	"time"

	shopifyyaml "github.com/Shopify/yaml"
	ghodssyaml "github.com/ghodss/yaml"
	goyaml "github.com/go-yaml/yaml"
	goccyyaml "github.com/goccy/go-yaml"
	yyaml "github.com/mohammadraziei/yyaml"
	k8syaml "sigs.k8s.io/yaml"
	yamlv3 "gopkg.in/yaml.v3"
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
	err := yamlv3.Unmarshal(data, &result)
	return result, err
}

func (y *YAMLv3) Marshal(data interface{}) ([]byte, error) {
	return yamlv3.Marshal(data)
}

// GoCcy YAML implementation
type GoCcyYAML struct{}

func (g *GoCcyYAML) Name() string { return "github.com/goccy/go-yaml" }

func (g *GoCcyYAML) Unmarshal(data []byte) (interface{}, error) {
	var result interface{}
	err := goccyyaml.Unmarshal(data, &result)
	return result, err
}

func (g *GoCcyYAML) Marshal(data interface{}) ([]byte, error) {
	return goccyyaml.Marshal(data)
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

// GoYAML implementation (github.com/go-yaml/yaml)
type GoYAML struct{}

func (g *GoYAML) Name() string { return "github.com/go-yaml/yaml" }

func (g *GoYAML) Unmarshal(data []byte) (interface{}, error) {
	var result interface{}
	err := goyaml.Unmarshal(data, &result)
	return result, err
}

func (g *GoYAML) Marshal(data interface{}) ([]byte, error) {
	return goyaml.Marshal(data)
}

// K8sYAML implementation (sigs.k8s.io/yaml)
type K8sYAML struct{}

func (k *K8sYAML) Name() string { return "sigs.k8s.io/yaml" }

func (k *K8sYAML) Unmarshal(data []byte) (interface{}, error) {
	var result interface{}
	err := k8syaml.Unmarshal(data, &result)
	return result, err
}

func (k *K8sYAML) Marshal(data interface{}) ([]byte, error) {
	return k8syaml.Marshal(data)
}

// GhodssYAML implementation (github.com/ghodss/yaml)
type GhodssYAML struct{}

func (g *GhodssYAML) Name() string { return "github.com/ghodss/yaml" }

func (g *GhodssYAML) Unmarshal(data []byte) (interface{}, error) {
	var result interface{}
	err := ghodssyaml.Unmarshal(data, &result)
	return result, err
}

func (g *GhodssYAML) Marshal(data interface{}) ([]byte, error) {
	return ghodssyaml.Marshal(data)
}


// ShopifyYAML implementation (github.com/Shopify/yaml)
type ShopifyYAML struct{}

func (s *ShopifyYAML) Name() string { return "github.com/Shopify/yaml" }

func (s *ShopifyYAML) Unmarshal(data []byte) (interface{}, error) {
	var result interface{}
	err := shopifyyaml.Unmarshal(data, &result)
	return result, err
}

func (s *ShopifyYAML) Marshal(data interface{}) ([]byte, error) {
	return shopifyyaml.Marshal(data)
}

// showProgress displays a simple tqdm-like progress indicator
func showProgress(current, total int, prefix string) {
	percent := float64(current) / float64(total) * 100
	barLen := 30
	filled := int(percent / 100 * float64(barLen))
	bar := strings.Repeat("█", filled) + strings.Repeat("░", barLen-filled)
	fmt.Printf("\r%s [%s] %3.0f%% (%d/%d)", prefix, bar, percent, current, total)
	if current == total {
		fmt.Printf("\n")
	}
}

func runBenchmark(lib LibraryBenchmark, files []string, operation string) BenchmarkResult {
	start := time.Now()
	success := 0
	errors := 0

	for i, file := range files {
		// Show progress for each file processed
		showProgress(i+1, len(files), fmt.Sprintf("  [%s/%s] %s", lib.Name(), operation, filepath.Base(file)))

		data, err := os.ReadFile(file)
		if err != nil {
			log.Printf("Error reading file %s: %v", file, err)
			errors++
			continue
		}

		// Process file with separate error handling function
		processFile := func() (err error) {
			// Add panic recovery for libraries that might panic
			defer func() {
				if r := recover(); r != nil {
					err = fmt.Errorf("panic recovered: %v", r)
				}
			}()

			if operation == "unmarshal" {
				_, err := lib.Unmarshal(data)
				return err
			} else if operation == "marshal" {
				// First unmarshal to get data structure
				parsed, err := lib.Unmarshal(data)
				if err != nil {
					return err
				}

				// Then marshal it back
				_, err = lib.Marshal(parsed)
				return err
			} else if operation == "roundtrip" {
				parsed, err := lib.Unmarshal(data)
				if err != nil {
					log.Printf("Roundtrip error (first unmarshal) for file %s with %s: %v", file, lib.Name(), err)
					return err
				}

				marshaled, err := lib.Marshal(parsed)
				if err != nil {
					log.Printf("Roundtrip error (marshal) for file %s with %s: %v", file, lib.Name(), err)
					return err
				}

				// Parse again to verify
				_, err = lib.Unmarshal(marshaled)
				if err != nil {
					log.Printf("Roundtrip error (second unmarshal) for file %s with %s: %v", file, lib.Name(), err)
					return err
				}
				return nil
			}
			return nil
		}

		// Execute the file processing and handle any errors
		if err := processFile(); err != nil {
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


// generateSummary generates a markdown summary with embedded SVG plots
func generateSummary(results []BenchmarkResult, files []string, datasetName string) string {
	summary := fmt.Sprintf("## %s\n\n", datasetName)
	summary += fmt.Sprintf("**Date:** %s\n", time.Now().Format("2006-01-02 15:04:05"))
	summary += fmt.Sprintf("**Total Files:** %d\n\n", len(files))

	// Group by operation
	operations := make(map[string][]BenchmarkResult)
	for _, r := range results {
		operations[r.Operation] = append(operations[r.Operation], r)
	}

	// Generate markdown content for each operation
	for op, opResults := range operations {
		sort.Slice(opResults, func(i, j int) bool {
			return opResults[i].TotalTime < opResults[j].TotalTime
		})

		summary += fmt.Sprintf("### %s\n\n", strings.ToUpper(op))

		summary += "| Library | Total Time | Avg Time | Success | Errors | Throughput |\n"
		summary += "|---------|------------|----------|---------|--------|------------|\n"

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
	summary += "---\n\n"
	return summary
}

func main() {
	// Create output directory
	if err := os.MkdirAll("output", 0755); err != nil {
		log.Printf("Error creating output directory: %v", err)
	}

	// Create error log file
	errorLogFile, err := os.OpenFile("output/error.log", os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		log.Printf("Error creating error log file: %v", err)
	} else {
		defer errorLogFile.Close()
		// Create multi-writer for log: both stderr and file
		multiWriter := io.MultiWriter(os.Stderr, errorLogFile)
		log.SetOutput(multiWriter)
	}

	// Get the executable path to calculate relative paths
	exePath, err := os.Executable()
	if err != nil {
		log.Fatalf("Failed to get executable path: %v", err)
	}

	dataDir := filepath.Join(exePath, "..", "..", "..", "data")
	if _, err := os.Stat(dataDir); os.IsNotExist(err) {
		dataDir = filepath.Join(dataDir, "..", "..", "data")
		if _, err := os.Stat(dataDir); os.IsNotExist(err) {
			log.Fatalf("Data directory does not exist: %s", dataDir)
		}
	}

	fmt.Printf("Scanning datasets in: %s\n", dataDir)

	// Read all subdirectories in dataDir
	entries, err := os.ReadDir(dataDir)
	if err != nil {
		log.Fatalf("Error reading data directory: %v", err)
	}

	// Filter and collect dataset directories
	var datasets []string
	for _, entry := range entries {
		if entry.IsDir() {
			datasets = append(datasets, entry.Name())
		}
	}

	// Initialize libraries
	libraries := []LibraryBenchmark{
		&YYAML{},
		&YAMLv3{},
		&GoCcyYAML{},
		&GoYAML{},
		&K8sYAML{},
		&GhodssYAML{},
		&ShopifyYAML{},
	}

	operations := []string{"unmarshal", "marshal", "roundtrip"}

	// Create output directory
	if err := os.MkdirAll("output", 0755); err != nil {
		log.Printf("Error creating output directory: %v", err)
	}

	// Start building the master README content
	var readmeContent strings.Builder
	readmeContent.WriteString("# YAML Library Benchmark Results\n\n")
	readmeContent.WriteString(fmt.Sprintf("**Generated:** %s\n", time.Now().Format("2006-01-02 15:04:05")))
	readmeContent.WriteString(fmt.Sprintf("**Go Version:** %s\n\n", runtime.Version()))
	readmeContent.WriteString("## System Information\n")
	readmeContent.WriteString(fmt.Sprintf("- **OS:** %s\n", runtime.GOOS))
	readmeContent.WriteString(fmt.Sprintf("- **Arch:** %s\n", runtime.GOARCH))
	readmeContent.WriteString(fmt.Sprintf("- **CPU Cores:** %d\n\n", runtime.NumCPU()))
	readmeContent.WriteString("## Results by Dataset\n\n")

	// Progress over datasets
	for dsIdx, datasetName := range datasets {
		showProgress(dsIdx+1, len(datasets), "Dataset")

		yamlDir := filepath.Join(dataDir, datasetName)

		if _, err := os.Stat(yamlDir); os.IsNotExist(err) {
			continue
		}

		fmt.Printf("\n[%d/%d] Processing dataset: %s\n", dsIdx+1, len(datasets), datasetName)
		fmt.Printf("Scanning YAML files in: %s\n", yamlDir)

		files, err := collectYAMLFiles(yamlDir)
		if err != nil {
			log.Printf("Error scanning YAML files in %s: %v", datasetName, err)
			continue
		}

		if len(files) == 0 {
			log.Printf("No YAML files found in %s, skipping\n", datasetName)
			continue
		}

		fmt.Printf("Found %d YAML files for benchmarking\n", len(files))

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

		// Save results to JSON file for this dataset
		jsonData, err := json.MarshalIndent(allResults, "", "  ")
		if err != nil {
			log.Printf("Error marshaling results to JSON: %v", err)
		} else {
			outputPath := fmt.Sprintf("output/%s_results.json", datasetName)
			if err := os.WriteFile(outputPath, jsonData, 0644); err != nil {
				log.Printf("Error writing results to file: %v", err)
			} else {
				fmt.Printf("\nResults saved to: %s\n", outputPath)
			}
		}

		// Append this dataset's summary to the master README
		readmeContent.WriteString(generateSummary(allResults, files, datasetName))
	}

	fmt.Printf("\n")

	// Write the unified README.md
	readmePath := "output/README.md"
	if err := os.WriteFile(readmePath, []byte(readmeContent.String()), 0644); err != nil {
		log.Printf("Error writing README: %v", err)
	} else {
		fmt.Printf("Master README saved to: %s\n", readmePath)
	}
}