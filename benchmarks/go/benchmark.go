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

	entries, err := os.ReadDir(dir)
	if err != nil {
		return nil, err
	}

	for _, entry := range entries {
		fullPath := filepath.Join(dir, entry.Name())

		if entry.IsDir() {
			subFiles, err := collectYAMLFiles(fullPath)
			if err != nil {
				return nil, err
			}
			files = append(files, subFiles...)
			continue
		}

		ext := strings.ToLower(filepath.Ext(entry.Name()))
		if ext == ".yaml" || ext == ".yml" {
			files = append(files, fullPath)
		}
	}

	return files, nil
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
			// parsed2, err := lib.Unmarshal(marshaled)
			if err != nil {
				errors++
				continue
			}

			// // Convert both to JSON for comparison (deep equality check)
			// json1, err1 := json.Marshal(parsed)
			// json2, err2 := json.Marshal(parsed2)
			// if err1 != nil || err2 != nil || string(json1) != string(json2) {
			// 	errors++
			// 	log.Printf("Roundtrip data mismatch for file %s with %s", file, lib.Name())
			// 	continue
			// }
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

	// Try to find the data directory relative to the executable
	// First try: from build directory (if running from build/)
	yamlDir := filepath.Join(dataDir, "yaml-test-suite")
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

	// Create output directory
	if err := os.MkdirAll("output", 0755); err != nil {
		log.Printf("Error creating output directory: %v", err)
	}

	// Save results to JSON file
	jsonData, err := json.MarshalIndent(allResults, "", "  ")
	if err != nil {
		log.Printf("Error marshaling results to JSON: %v", err)
	} else {
		outputPath := "output/benchmark_results.json"
		if err := ioutil.WriteFile(outputPath, jsonData, 0644); err != nil {
			log.Printf("Error writing results to file: %v", err)
		} else {
			fmt.Printf("\nResults saved to: %s\n", outputPath)
		}
	}

	// Generate summary markdown with embedded SVG
	generateSummary(allResults, files)
}

func generateSummary(results []BenchmarkResult, files []string) {
	// Create output directory
	if err := os.MkdirAll("output", 0755); err != nil {
		log.Printf("Error creating output directory: %v", err)
	}

	summary := "# YAML Library Benchmark Results\n\n"
	summary += fmt.Sprintf("**Date:** %s\n", time.Now().Format("2006-01-02 15:04:05"))
	summary += fmt.Sprintf("**Total Files:** %d\n", len(files))
	summary += fmt.Sprintf("**Go Version:** %s\n\n", runtime.Version())

	// Group by operation
	operations := make(map[string][]BenchmarkResult)
	for _, r := range results {
		operations[r.Operation] = append(operations[r.Operation], r)
	}

	// Generate markdown with embedded SVG
	for op, opResults := range operations {
		// Sort by total time (fastest first)
		sort.Slice(opResults, func(i, j int) bool {
			return opResults[i].TotalTime < opResults[j].TotalTime
		})

		summary += fmt.Sprintf("## %s\n\n", strings.ToUpper(op))

		// Generate and embed SVG plot
		svgContent := createSimpleSVG(opResults, op)
		// Embed SVG directly in markdown with caption
		summary += fmt.Sprintf("<div align=\"center\">\n%s\n</div>\n", svgContent)
		summary += fmt.Sprintf("<p align=\"center\"><em>Figure: %s performance comparison across YAML libraries</em></p>\n\n", strings.ToUpper(op))

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

	// Add combined comparison section
	summary += "## Combined Performance Comparison\n\n"

	// Generate combined SVG plot
	combinedSVG := createCombinedSVG(results, operations)
	summary += fmt.Sprintf("<div align=\"center\">\n%s\n</div>\n\n", combinedSVG)

	// Add performance comparison tables
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

	outputPath := "output/BENCHMARK_SUMMARY.md"
	if err := ioutil.WriteFile(outputPath, []byte(summary), 0644); err != nil {
		log.Printf("Error writing summary: %v", err)
	} else {
		fmt.Printf("Summary saved to: %s\n", outputPath)
	}
}

// createSimpleSVG creates a simple SVG horizontal bar chart (bars go left to right)
func createSimpleSVG(results []BenchmarkResult, operation string) string {
	// Calculate width based on max throughput value
	maxVal := 0.0
	for _, r := range results {
		if r.Throughput > maxVal {
			maxVal = r.Throughput
		}
	}

	// Manual margins: left 200, right 20, top 30, bottom 10
	leftMargin := 200
	rightMargin := 80
	topMargin := 30
	bottomMargin := 10
	barHeight := 30
	height := 320

	// Scale maxVal to achieve width = 800
	targetWidth := 800
	availableWidth := targetWidth - leftMargin - rightMargin
	scaleFactor := float64(availableWidth) / maxVal

	width := targetWidth

	var svg strings.Builder
	svg.WriteString(fmt.Sprintf(`<svg width="%d" height="%d" xmlns="http://www.w3.org/2000/svg">`, width, height))
	svg.WriteString(fmt.Sprintf(`<rect width="%d" height="%d" fill="white"/>`, width, height))
	svg.WriteString(fmt.Sprintf(`<text x="%d" y="%d" font-family="Arial" font-size="18" text-anchor="middle">%s Performance</text>`, width/2, topMargin, strings.ToUpper(operation)))

	// Draw horizontal bars (left to right) - longer bars for higher throughput
	for i, r := range results {
		y := topMargin + 50 + i*(barHeight+20)
		barWidth := int(r.Throughput * scaleFactor)
		x := leftMargin

		// Bar
		svg.WriteString(fmt.Sprintf(`<rect x="%d" y="%d" width="%d" height="%d" fill="steelblue" stroke="black" stroke-width="1"/>`,
			x, y, barWidth, barHeight))

		// Library label - remove only "github.com/" prefix, display horizontally on left
		libName := r.Library
		if strings.HasPrefix(libName, "github.com/") {
			libName = strings.TrimPrefix(libName, "github.com/")
		}
		// Display text horizontally (left aligned)
		svg.WriteString(fmt.Sprintf(`<text x="%d" y="%d" font-family="Arial" font-size="12" text-anchor="end" dy="0.35em">%s</text>`,
			leftMargin-10, y+barHeight/2, libName))

		// Value label at the end of the bar
		svg.WriteString(fmt.Sprintf(`<text x="%d" y="%d" font-family="Arial" font-size="11" text-anchor="start" dy="0.35em">%.1f files/s</text>`,
			x+barWidth+5, y+barHeight/2, r.Throughput))
	}

	// X-axis label
	svg.WriteString(fmt.Sprintf(`<text x="%d" y="%d" font-family="Arial" font-size="14" text-anchor="middle">Throughput (files per second)</text>`,
		width/2, height-bottomMargin))

	svg.WriteString("</svg>")
	return svg.String()
}

// createCombinedSVG creates a combined SVG horizontal bar chart for all operations
func createCombinedSVG(results []BenchmarkResult, operations map[string][]BenchmarkResult) string {
	// Manual margins: left 200, right 20, top 30, bottom 10
	leftMargin := 250
	rightMargin := 100
	topMargin := 30
	bottomMargin := 10
	barHeight := 25

	// Colors for different operations
	colors := []string{"#FF6B6B", "#4ECDC4", "#45B7D1", "#96CEB4", "#FFEAA7"}
	opNames := []string{"UNMARSHAL", "MARSHAL", "ROUNDTRIP"}

	// Get unique libraries
	librarySet := make(map[string]bool)
	for _, opResults := range operations {
		for _, r := range opResults {
			librarySet[r.Library] = true
		}
	}

	// Convert to sorted list
	var libraries []string
	for lib := range librarySet {
		libraries = append(libraries, lib)
	}
	sort.Strings(libraries)

	// Find max throughput value for scaling
	maxVal := 0.0
	for _, opResults := range operations {
		for _, r := range opResults {
			if r.Throughput > maxVal {
				maxVal = r.Throughput
			}
		}
	}

	// Scale maxVal to achieve width = 800
	targetWidth := 800
	availableWidth := targetWidth - leftMargin - rightMargin
	scaleFactor := float64(availableWidth) / maxVal

	// Calculate needed height based on number of libraries
	numLibraries := len(libraries)
	height := topMargin + 100 + numLibraries*(barHeight*len(opNames)+40) + bottomMargin
	width := targetWidth

	var svg strings.Builder
	svg.WriteString(fmt.Sprintf(`<svg width="%d" height="%d" xmlns="http://www.w3.org/2000/svg">`, width, height))
	svg.WriteString(fmt.Sprintf(`<rect width="%d" height="%d" fill="white"/>`, width, height))
	svg.WriteString(fmt.Sprintf(`<text x="%d" y="%d" font-family="Arial" font-size="20" text-anchor="middle">Combined Performance Comparison</text>`, width/2, topMargin))

	// Draw horizontal legend at the top
	svg.WriteString(`<g transform="translate(50, 60)">`)
	for i, op := range opNames {
		if i < len(operations) {
			svg.WriteString(fmt.Sprintf(`<rect x="%d" y="0" width="15" height="15" fill="%s"/>`, i*120, colors[i%len(colors)]))
			svg.WriteString(fmt.Sprintf(`<text x="%d" y="12" font-family="Arial" font-size="14">%s</text>`, i*120+20, op))
		}
	}
	svg.WriteString("</g>")

	// Draw horizontal bars for each library
	for libIdx, lib := range libraries {
		y := topMargin + 100 + libIdx*(barHeight*len(opNames)+40)

		// Library label - remove only "github.com/" prefix
		shortLib := lib
		if strings.HasPrefix(shortLib, "github.com/") {
			shortLib = strings.TrimPrefix(shortLib, "github.com/")
		}
		// Display library name on the left
		svg.WriteString(fmt.Sprintf(`<text x="%d" y="%d" font-family="Arial" font-size="14" text-anchor="end" dy="0.35em">%s</text>`,
			leftMargin-10, y+barHeight*len(opNames)/2, shortLib))

		// Draw bars for each operation
		for opIdx, op := range opNames {
			if opResults, ok := operations[strings.ToLower(op)]; ok {
				// Find throughput value for this library and operation
				value := 0.0
				for _, r := range opResults {
					if r.Library == lib {
						value = r.Throughput
						break
					}
				}

				if value > 0 {
					barWidth := int(value * scaleFactor)
					x := leftMargin
					barY := y + opIdx*barHeight

					svg.WriteString(fmt.Sprintf(`<rect x="%d" y="%d" width="%d" height="%d" fill="%s" stroke="black" stroke-width="1"/>`,
						x, barY, barWidth, barHeight, colors[opIdx%len(colors)]))

					// Value label at the end of the bar
					svg.WriteString(fmt.Sprintf(`<text x="%d" y="%d" font-family="Arial" font-size="11" text-anchor="start" dy="0.35em">%.1f files/s</text>`,
						x+barWidth+5, barY+barHeight/2, value))
				}
			}
		}
	}

	// X-axis label
	svg.WriteString(fmt.Sprintf(`<text x="%d" y="%d" font-family="Arial" font-size="16" text-anchor="middle">Throughput (files per second)</text>`,
		width/2, height-bottomMargin))

	svg.WriteString("</svg>")
	return svg.String()
}
