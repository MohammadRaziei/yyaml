# =============================================================================
# YYAML Documentation Makefile
# =============================================================================

# Default goal
.DEFAULT_GOAL := build

# Phony targets
.PHONY: all clean help \
        ensure-doxygen ensure-doxybook2 ensure-node \
        doxygen-c doxygen-cpp md-c md-cpp \
        prepare-md prepare-docusaurus docs-only \
        docusaurus-build build serve

# =============================================================================
# Configuration Variables
# =============================================================================

# Directories
BUILD_DIR := $(CURDIR)/build
TEMP_DIR := $(BUILD_DIR)/temp
DOCUSAURUS_DIR := $(BUILD_DIR)/docusaurus
OUTPUT_DIR := $(BUILD_DIR)/output-html

# Doxybook2 configuration
DOXYBOOK2_ROOT := $(TEMP_DIR)/doxybook2
DOXYBOOK2_BIN := $(DOXYBOOK2_ROOT)/bin/doxybook2
DOXYBOOK2_VERSION := v1.5.0
DOXYBOOK2_URL := https://github.com/matusnovak/doxybook2/releases/download/$(DOXYBOOK2_VERSION)/doxybook2-linux-amd64-$(DOXYBOOK2_VERSION).zip

# Language-specific directories
C_LANG_DIR := languages/c
CPP_LANG_DIR := languages/cpp
C_XML_DIR := $(TEMP_DIR)/c/xml
CPP_XML_DIR := $(TEMP_DIR)/cpp/xml
C_API_DIR := $(DOCUSAURUS_DIR)/languages/c/api
CPP_API_DIR := $(DOCUSAURUS_DIR)/languages/cpp/api

# Benchmark data
BENCHMARKS_DIR := ../benchmarks/python
BENCHMARK_CSV := $(BENCHMARKS_DIR)/benchmark.csv

# Server configuration
SERVE_PORT := 3000

# =============================================================================
# Main Targets
# =============================================================================

# Default target - build everything
all: build

# Help target
help:
	@echo "YYAML Documentation Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  build        - Build complete documentation (default)"
	@echo "  clean        - Remove all build artifacts"
	@echo "  serve        - Serve built documentation on port $(SERVE_PORT)"
	@echo "  docs-only    - Prepare documentation files only"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Development targets:"
	@echo "  doxygen-c    - Generate C API documentation"
	@echo "  doxygen-cpp  - Generate C++ API documentation"
	@echo "  md-c         - Convert C API to Markdown"
	@echo "  md-cpp       - Convert C++ API to Markdown"
	@echo "  prepare-md   - Prepare all Markdown files"
	@echo ""
	@echo "Dependency targets:"
	@echo "  ensure-doxygen   - Check/install Doxygen"
	@echo "  ensure-doxybook2 - Check/install Doxybook2"
	@echo "  ensure-node      - Check Node.js installation"


# =============================================================================
# Cleanup Targets
# =============================================================================

clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)/

# =============================================================================
# Dependency Management
# =============================================================================

ensure-doxygen:
	@echo "Checking for Doxygen..."
	@if command -v doxygen >/dev/null 2>&1; then \
		echo "✓ Doxygen found"; \
	else \
		echo "Doxygen not found; attempting install via apt-get..."; \
		sudo apt-get update && sudo apt-get install -y doxygen || { echo "✗ Failed to install Doxygen; please install it manually."; exit 1; }; \
	fi

ensure-doxybook2:
	@echo "Checking for Doxybook2..."
	@if [ -x "$(DOXYBOOK2_BIN)" ]; then \
		echo "✓ Using bundled Doxybook2"; \
	else \
		echo "Doxybook2 not found; downloading..."; \
		mkdir -p $(DOXYBOOK2_ROOT); \
		tmp="$$(mktemp -p $(TEMP_DIR) -t doxybook2.XXXXXX.zip)"; \
		if command -v curl >/dev/null 2>&1; then \
			curl -L "$(DOXYBOOK2_URL)" -o "$$tmp" --silent; \
		elif command -v wget >/dev/null 2>&1; then \
			wget -O "$$tmp" "$(DOXYBOOK2_URL)" --no-verbose; \
		else \
			echo "✗ Please install curl or wget to download Doxybook2"; exit 1; \
		fi; \
		ZIP_PATH="$$tmp" python3 -c "import os, zipfile; zipfile.ZipFile(os.environ['ZIP_PATH']).extractall('$(DOXYBOOK2_ROOT)')"; \
		chmod +x $(DOXYBOOK2_BIN) || true; \
		rm -f "$$tmp"; \
		echo "✓ Doxybook2 installed successfully"; \
	fi

ensure-node:
	@echo "Checking for Node.js..."
	@if command -v npm >/dev/null 2>&1; then \
		echo "✓ npm found"; \
	else \
		echo "✗ npm (Node.js) not found; please install Node.js to build the site"; \
		exit 1; \
	fi

# =============================================================================
# Documentation Generation
# =============================================================================

doxygen-c: ensure-doxygen
	@echo "Generating C API documentation..."
	mkdir -p $(C_XML_DIR)
	cd $(C_LANG_DIR) && doxygen Doxyfile
	@echo "✓ C API documentation generated"

doxygen-cpp: ensure-doxygen
	@echo "Generating C++ API documentation..."
	mkdir -p $(CPP_XML_DIR)
	cd $(CPP_LANG_DIR) && doxygen Doxyfile
	@echo "✓ C++ API documentation generated"

md-c: ensure-doxybook2 doxygen-c
	@echo "Converting C API to Markdown..."
	mkdir -p $(C_API_DIR)
	"$(DOXYBOOK2_BIN)" \
		--input $(C_XML_DIR) \
		--output $(C_API_DIR) \
		--config $(C_LANG_DIR)/doxybook.json
	@echo "✓ C API converted to Markdown"

md-cpp: ensure-doxybook2 doxygen-cpp
	@echo "Converting C++ API to Markdown..."
	@if [ ! -f $(CPP_XML_DIR)/index.xml ] || ! grep -q "<compound" $(CPP_XML_DIR)/index.xml 2>/dev/null; then \
		echo "⚠ No C++ XML found; skipping C++ Markdown generation."; \
	else \
		mkdir -p $(CPP_API_DIR); \
		"$(DOXYBOOK2_BIN)" \
			--input $(CPP_XML_DIR) \
			--output $(CPP_API_DIR) \
			--config $(CPP_LANG_DIR)/doxybook.json; \
		echo "✓ C++ API converted to Markdown"; \
	fi

prepare-md: md-c md-cpp
	@echo "✓ All Markdown files are ready!"

# =============================================================================
# Docusaurus Build System
# =============================================================================

prepare-docusaurus:
	@echo "Preparing Docusaurus build environment..."
	# Create necessary directories
	mkdir -p $(DOCUSAURUS_DIR) $(C_API_DIR) $(CPP_API_DIR) $(DOCUSAURUS_DIR)/benchmarks
	
	# Copy all documentation files using .docsignore for exclusions
	rsync -av --prune-empty-dirs \
		--exclude-from=.docsignore \
		--exclude='$(BUILD_DIR)/**' ./ $(DOCUSAURUS_DIR)/
	
	# Copy benchmark data
	cp $(BENCHMARK_CSV) $(DOCUSAURUS_DIR)/benchmarks/benchmark.csv
	@echo "✓ Docusaurus environment prepared"

docs-only: prepare-md prepare-docusaurus
	@echo "✓ Documentation files are ready for build"

docusaurus-build: ensure-node docs-only
	@echo "Building Docusaurus site..."
	cd $(DOCUSAURUS_DIR) && \
		if [ ! -d node_modules ]; then npm install --legacy-peer-deps; fi && \
		npx docusaurus build --out-dir $(OUTPUT_DIR)
	@echo "✓ Docusaurus site built successfully"

# =============================================================================
# Main Build Targets
# =============================================================================

build: docusaurus-build
	@echo "🎉 Documentation build completed successfully!"
	@echo "Output directory: $(OUTPUT_DIR)"

serve:
	@echo "Starting documentation server on port $(SERVE_PORT)..."
	@if [ ! -d "$(OUTPUT_DIR)" ] || [ -z "$$(ls -A $(OUTPUT_DIR) 2>/dev/null)" ]; then \
		echo "✗ Error: Documentation not built. Please run 'make build' first."; \
		exit 1; \
	else \
		cd $(OUTPUT_DIR) && npx serve -p $(SERVE_PORT); \
	fi
