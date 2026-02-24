# Makefile for YAML benchmark
# Builds and runs the benchmark in a build directory

.PHONY: all build run clean help

# Default target
all: build run

# Build the benchmark
build:
	@echo "Building benchmark..."
	@mkdir -p build
	@cd build && CGO_CFLAGS="-O3 -flto" go build -trimpath -ldflags="-s -w" -o benchmark_app ../benchmark.go
	@echo "Build complete: build/benchmark_app"

# Run the benchmark
run: build
	@echo "Running benchmark..."
	@cd build && ./benchmark_app

# Clean build artifacts
clean:
	@echo "Cleaning build directory..."
	@rm -rf build output

# Show help
help:
	@echo "YAML Benchmark Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build and run benchmark (default)"
	@echo "  build   - Build benchmark in build directory"
	@echo "  run     - Run benchmark (builds if needed)"
	@echo "  clean   - Remove build directory and output"
	@echo "  help    - Show this help message"
	@echo ""
	@echo "Output files will be saved to: output/"