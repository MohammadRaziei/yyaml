.DEFAULT_GOAL := build
.PHONY: clean ensure-doxygen ensure-doxybook2 ensure-node doxygen-c doxygen-cpp md-c md-cpp prepare-docusaurus docusaurus-build docs-only build serve

doxybook2_root := /tmp/doxybook2
doxybook2 := $(doxybook2_root)/bin/doxybook2


clean:
	rm -rf build/

ensure-doxygen:
	@if command -v doxygen >/dev/null 2>&1; then \
		echo "doxygen found"; \
	else \
		echo "doxygen not found; attempting install via apt-get"; \
		sudo apt-get update && sudo apt-get install -y doxygen || { echo "Failed to install doxygen; please install it manually."; exit 1; }; \
	fi

ensure-doxybook2:
	@if [ -x "$(doxybook2)" ]; then \
		echo "using bundled doxybook2"; \
	else \
		mkdir -p $(doxybook2_root); \
		url="https://github.com/matusnovak/doxybook2/releases/download/v1.5.0/doxybook2-linux-amd64-v1.5.0.zip"; \
		tmp="$$(mktemp -p build/temp -t doxybook2.XXXXXX.zip)"; \
		if command -v curl >/dev/null 2>&1; then \
			curl -L "$$url" -o "$$tmp" --silent; \
		elif command -v wget >/dev/null 2>&1; then \
			wget -O "$$tmp" "$$url" --no-verbose; \
		else \
			echo "Please install curl or wget to download doxybook2"; exit 1; \
		fi; \
		ZIP_PATH="$$tmp" python3 -c "import os, zipfile; zipfile.ZipFile(os.environ['ZIP_PATH']).extractall('$(doxybook2_root)')"; \
		chmod +x build/doxybook2/bin/doxybook2 || true; \
		rm -f "$$tmp"; \
	fi

ensure-node:
	@if command -v npm >/dev/null 2>&1; then \
		echo "npm found"; \
	else \
		echo "npm (Node.js) not found; please install Node.js to build the site"; \
		exit 1; \
	fi

doxygen-c: ensure-doxygen
	mkdir -p build/temp/c/xml
	cd languages/c && doxygen Doxyfile

doxygen-cpp: ensure-doxygen
	mkdir -p build/temp/cpp/xml
	cd languages/cpp && doxygen Doxyfile

md-c: ensure-doxybook2 doxygen-c
		mkdir -p build/docusaurus/languages/c/api; \
		"$(doxybook2)" \
			--input build/temp/c/xml \
			--output build/docusaurus/languages/c/api \
			--config languages/c/doxybook.json

md-cpp: ensure-doxybook2 doxygen-cpp
	@if [ ! -f build/temp/cpp/xml/index.xml ] || ! grep -q "<compound" build/temp/cpp/xml/index.xml 2>/dev/null; then \
		echo "No C++ XML found; skipping C++ Markdown generation."; \
		exit 0; \
	fi; \
	"$(doxybook2)" \
		--input build/temp/cpp/xml \
		--output build/docusaurus/languages/cpp/api \
		--config languages/cpp/doxybook.json

prepare-md: md-c md-cpp
	echo "md files are ready!"

prepare-docusaurus:
	# Copy landing + languages + benchmarks into build/docusaurus
	# Copy API markdowns into docusaurus/languages/<lang>/api
	mkdir -p build/docusaurus build/docusaurus/languages/c/api build/docusaurus/languages/cpp/api build/docusaurus/benchmarks
	# Copy all md, mdx, csv, svg, json, png, js (excluding build/) preserving structure
	rsync -av --prune-empty-dirs \
		--exclude 'Makefile' --exclude 'Doxyfile' \
		--exclude='build/**' ./ build/docusaurus/
	# Ensure benchmark CSV from root benchmarks/python is available
	mkdir -p build/docusaurus/benchmarks
	cp ../benchmarks/python/benchmark.csv build/docusaurus/benchmarks/benchmark.csv

docs-only: prepare-md prepare-docusaurus

docusaurus-build: ensure-node docs-only
	cd build/docusaurus && \
		if [ ! -d node_modules ]; then npm install --legacy-peer-deps; fi && \
		npx docusaurus build --out-dir ../output-html


build: docusaurus-build

serve:
	@if [ ! -d "build/output-html" ] || [ -z "$$(ls -A build/output-html 2>/dev/null)" ]; then \
		echo "Error: Documentation not built. Please run 'make build' first."; \
		exit 1; \
	else \
		cd build/output-html && npx serve -p 3000; \
	fi
