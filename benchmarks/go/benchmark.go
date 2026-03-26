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

// BenchmarkResult holds metrics for a single benchmark execution
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

// LibraryBenchmark interface for YAML library implementations
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

// GoYAML implementation
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

// K8sYAML implementation
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

// GhodssYAML implementation
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

// ShopifyYAML implementation
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

// showProgress displays a tqdm-like progress indicator
func showProgress(current, total int, prefix string) {
	if total == 0 {
		return
	}
	percent := float64(current) / float64(total) * 100
	barLen := 30
	filled := int(percent / 100 * float64(barLen))
	bar := strings.Repeat("█", filled) + strings.Repeat("░", barLen-filled)
	fmt.Printf("\r%s [%s] %3.0f%% (%d/%d)", prefix, bar, percent, current, total)
	if current == total {
		fmt.Printf("\n")
	}
}

// runBenchmark executes benchmark for a library, files, and operation
func runBenchmark(lib LibraryBenchmark, files []string, operation string) BenchmarkResult {
	start := time.Now()
	success := 0
	errors := 0

	for i, file := range files {
		showProgress(i+1, len(files), fmt.Sprintf("  [%s/%s] %s", lib.Name(), operation, filepath.Base(file)))

		data, err := os.ReadFile(file)
		if err != nil {
			log.Printf("Error reading file %s: %v", file, err)
			errors++
			continue
		}

		processFile := func() (err error) {
			defer func() {
				if r := recover(); r != nil {
					err = fmt.Errorf("panic recovered: %v", r)
				}
			}()

			if operation == "unmarshal" {
				_, err := lib.Unmarshal(data)
				return err
			} else if operation == "marshal" {
				parsed, err := lib.Unmarshal(data)
				if err != nil {
					return err
				}
				_, err = lib.Marshal(parsed)
				return err
			} else if operation == "roundtrip" {
				parsed, err := lib.Unmarshal(data)
				if err != nil {
					return err
				}
				marshaled, err := lib.Marshal(parsed)
				if err != nil {
					return err
				}
				_, err = lib.Unmarshal(marshaled)
				return err
			}
			return nil
		}

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

// printResults displays benchmark results in console
func printResults(results []BenchmarkResult) {
	fmt.Println("\n" + strings.Repeat("=", 100))
	fmt.Println("YAML LIBRARY BENCHMARK RESULTS")
	fmt.Println(strings.Repeat("=", 100))

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

// generateDatasetSection generates markdown section for a single dataset with embedded SVGs
// Uses user-provided functions: createSimpleSVG, createCombinedSVG
func generateDatasetSection(results []BenchmarkResult, files []string, datasetName string) string {
	var sb strings.Builder

	sb.WriteString(fmt.Sprintf("## %s\n\n", datasetName))
	sb.WriteString(fmt.Sprintf("**Date:** %s\n", time.Now().Format("2006-01-02 15:04:05")))
	sb.WriteString(fmt.Sprintf("**Total Files:** %d\n\n", len(files)))

	operations := make(map[string][]BenchmarkResult)
	for _, r := range results {
		operations[r.Operation] = append(operations[r.Operation], r)
	}

	for op, opResults := range operations {
		sort.Slice(opResults, func(i, j int) bool {
			return opResults[i].TotalTime < opResults[j].TotalTime
		})

		sb.WriteString(fmt.Sprintf("### %s\n\n", strings.ToUpper(op)))

		// Embed SVG using user-provided createSimpleSVG function
		svgContent := createSimpleSVG(opResults, op)
		sb.WriteString(fmt.Sprintf("<div align=\"center\">\n%s\n</div>\n", svgContent))
		sb.WriteString(fmt.Sprintf("<p align=\"center\"><em>Figure: %s performance comparison</em></p>\n\n", strings.ToUpper(op)))

		sb.WriteString("| Library | Total Time | Avg Time | Success | Errors | Throughput |\n")
		sb.WriteString("|---------|------------|----------|---------|--------|------------|\n")

		for _, r := range opResults {
			sb.WriteString(fmt.Sprintf("| %s | %v | %v | %d | %d | %.2f files/s |\n",
				r.Library,
				r.TotalTime.Round(time.Millisecond),
				r.AvgTime.Round(time.Microsecond),
				r.SuccessCount,
				r.ErrorCount,
				r.Throughput))
		}
		sb.WriteString("\n")
	}

	// Add combined comparison with user-provided createCombinedSVG function
	sb.WriteString("### Combined Comparison\n\n")
	combinedSVG := createCombinedSVG(results, operations)
	sb.WriteString(fmt.Sprintf("<div align=\"center\">\n%s\n</div>\n", combinedSVG))
	sb.WriteString("\n---\n\n")

	return sb.String()
}

func main() {
	// Create output directory
	if err := os.MkdirAll("output", 0755); err != nil {
		log.Printf("Error creating output directory: %v", err)
	}

	// Setup error logging to both stderr and file
	errorLogFile, err := os.OpenFile("output/error.log", os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0644)
	if err != nil {
		log.Printf("Error creating error log file: %v", err)
	} else {
		defer errorLogFile.Close()
		multiWriter := io.MultiWriter(os.Stderr, errorLogFile)
		log.SetOutput(multiWriter)
	}

	// Resolve data directory path
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

	// Read subdirectories in dataDir
	entries, err := os.ReadDir(dataDir)
	if err != nil {
		log.Fatalf("Error reading data directory: %v", err)
	}

	// Collect dataset directories
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

	// Initialize master README content
	var readmeContent strings.Builder
	readmeContent.WriteString("# YAML Library Benchmark Results\n\n")
	readmeContent.WriteString(fmt.Sprintf("**Generated:** %s\n", time.Now().Format("2006-01-02 15:04:05")))
	readmeContent.WriteString(fmt.Sprintf("**Go Version:** %s\n\n", runtime.Version()))
	readmeContent.WriteString("## System Information\n")
	readmeContent.WriteString(fmt.Sprintf("- **OS:** %s\n", runtime.GOOS))
	readmeContent.WriteString(fmt.Sprintf("- **Arch:** %s\n", runtime.GOARCH))
	readmeContent.WriteString(fmt.Sprintf("- **CPU Cores:** %d\n\n", runtime.NumCPU()))
	readmeContent.WriteString("## Results by Dataset\n\n")

	// Process each dataset sequentially - NO mixing of results
	for dsIdx, datasetName := range datasets {
		showProgress(dsIdx+1, len(datasets), "Dataset")

		yamlDir := filepath.Join(dataDir, datasetName)
		if _, err := os.Stat(yamlDir); os.IsNotExist(err) {
			continue
		}

		fmt.Printf("\n[%d/%d] Processing dataset: %s\n", dsIdx+1, len(datasets), datasetName)

		// Collect YAML files using user-provided function
		files, err := collectYAMLFiles(yamlDir)
		if err != nil {
			log.Printf("Error scanning YAML files in %s: %v", datasetName, err)
			continue
		}
		if len(files) == 0 {
			fmt.Printf("No YAML files in %s, skipping\n", datasetName)
			continue
		}
		fmt.Printf("Found %d YAML files\n", len(files))

		// Run benchmarks for this dataset only
		var datasetResults []BenchmarkResult
		for _, lib := range libraries {
			for _, op := range operations {
				result := runBenchmark(lib, files, op)
				datasetResults = append(datasetResults, result)
			}
		}

		// Print results to console
		printResults(datasetResults)

		// Save JSON for this dataset ONLY (separate file)
		jsonData, err := json.MarshalIndent(datasetResults, "", "  ")
		if err != nil {
			log.Printf("Error marshaling results for %s: %v", datasetName, err)
		} else {
			jsonPath := fmt.Sprintf("output/%s_results.json", datasetName)
			if err := os.WriteFile(jsonPath, jsonData, 0644); err != nil {
				log.Printf("Error writing JSON for %s: %v", datasetName, err)
			} else {
				fmt.Printf("Saved: %s\n", jsonPath)
			}
		}

		// Generate and append this dataset's section to master README
		// Results are NOT combined with other datasets
		datasetSection := generateDatasetSection(datasetResults, files, datasetName)
		readmeContent.WriteString(datasetSection)

		fmt.Printf("✅ Completed dataset: %s\n", datasetName)
	}

	fmt.Printf("\n")

	// Write the unified README.md with all dataset sections appended sequentially
	readmePath := "output/README.md"
	if err := os.WriteFile(readmePath, []byte(readmeContent.String()), 0644); err != nil {
		log.Printf("Error writing README: %v", err)
	} else {
		fmt.Printf("Master README saved to: %s\n", readmePath)
	}
}