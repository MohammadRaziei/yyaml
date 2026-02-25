package main

import (
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"
	"runtime"
	"sort"
	"strings"
	"time"
)

// collectYAMLFiles recursively collects all YAML files in a directory
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

// generateSummary generates a markdown summary with embedded SVG plots
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
	leftMargin := 150
	rightMargin := 90
	topMargin := 30
	bottomMargin := 10
	barHeight := 30
	height := 280
	width := 600

	// Scale maxVal to achieve width
	availableWidth := width - leftMargin - rightMargin
	scaleFactor := float64(availableWidth) / maxVal

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
	leftMargin := 200
	rightMargin := 120
	topMargin := 30
	bottomMargin := 10
	barHeight := 25
	width := 600

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

	// Scale maxVal to achieve width
	availableWidth := width - leftMargin - rightMargin
	scaleFactor := float64(availableWidth) / maxVal

	// Calculate needed height based on number of libraries
	numLibraries := len(libraries)
	height := topMargin + 100 + numLibraries*(barHeight*len(opNames)+40) + bottomMargin

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
