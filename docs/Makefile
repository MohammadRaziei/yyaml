.DEFAULT_GOAL := build
.PHONY: clean ensure-doxygen ensure-doxybook2 ensure-node doxygen-c doxygen-cpp md-c md-cpp prepare-docusaurus docusaurus-build docs-only build

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
	@if [ -x "$(CURDIR)/build/doxybook2/bin/doxybook2" ]; then \
		echo "using bundled doxybook2"; \
	else \
		if command -v doxybook2 >/dev/null 2>&1; then \
			echo "system doxybook2 found, but downloading bundled copy for consistency"; \
		else \
			echo "doxybook2 not found; downloading bundled binary..."; \
		fi; \
		mkdir -p build/doxybook2; \
		url="https://github.com/matusnovak/doxybook2/releases/download/v1.5.0/doxybook2-linux-amd64-v1.5.0.zip"; \
		tmp="$$(mktemp -p build -t doxybook2.XXXXXX.zip)"; \
		if command -v curl >/dev/null 2>&1; then \
			curl -L "$$url" -o "$$tmp"; \
		elif command -v wget >/dev/null 2>&1; then \
			wget -O "$$tmp" "$$url"; \
		else \
			echo "Please install curl or wget to download doxybook2"; exit 1; \
		fi; \
		ZIP_PATH="$$tmp" python3 -c "import os, zipfile; zipfile.ZipFile(os.environ['ZIP_PATH']).extractall('build/doxybook2')"; \
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
	mkdir -p build/temp/c/xml languages/c/api
	cd languages/c && doxygen Doxyfile

doxygen-cpp: ensure-doxygen
	mkdir -p build/temp/cpp/xml languages/cpp/api
	cd languages/cpp && doxygen Doxyfile

md-c: ensure-doxybook2
	@DOXYBOOK_BIN=$$(if [ -x "$(CURDIR)/build/doxybook2/bin/doxybook2" ]; then echo "$(CURDIR)/build/doxybook2/bin/doxybook2"; else command -v doxybook2; fi); \
		test -n "$$DOXYBOOK_BIN" && test -x "$$DOXYBOOK_BIN" || { echo "doxybook2 missing after setup"; exit 1; }; \
		"$$DOXYBOOK_BIN" \
			--input build/temp/c/xml \
			--output languages/c/api \
			--config languages/c/doxybook.json

md-cpp: ensure-doxybook2
	@if [ ! -f build/temp/cpp/xml/index.xml ] || ! grep -q "<compound" build/temp/cpp/xml/index.xml 2>/dev/null; then \
		echo "No C++ XML found; skipping C++ Markdown generation."; \
		exit 0; \
	fi; \
	DOXYBOOK_BIN=$$(if [ -x "$(CURDIR)/build/doxybook2/bin/doxybook2" ]; then echo "$(CURDIR)/build/doxybook2/bin/doxybook2"; else command -v doxybook2; fi); \
	test -n "$$DOXYBOOK_BIN" && test -x "$$DOXYBOOK_BIN" || { echo "doxybook2 missing after setup"; exit 1; }; \
	"$$DOXYBOOK_BIN" \
		--input build/temp/cpp/xml \
		--output languages/cpp/api \
		--config languages/cpp/doxybook.json

prepare-docusaurus:
	# Copy landing + languages + benchmarks into build/docusaurus
	# Copy API markdowns into docusaurus/languages/<lang>/api
	mkdir -p build/docusaurus/languages/c/api build/docusaurus/languages/cpp/api build/docusaurus/benchmarks
	cp docusaurus.config.js build/docusaurus/docusaurus.config.js
	cp sidebars.js build/docusaurus/sidebars.js
	cp package.json build/docusaurus/package.json
	cp index.mdx build/docusaurus/index.mdx
	cp languages/index.md build/docusaurus/languages/index.md
	cp languages/c/index.md build/docusaurus/languages/c/index.md
	cp languages/cpp/index.md build/docusaurus/languages/cpp/index.md
	cp benchmarks/index.mdx build/docusaurus/benchmarks/index.mdx
	cp benchmarks/benchmark.csv build/docusaurus/benchmarks/benchmark.csv
	cp -r languages/c/api/. build/docusaurus/languages/c/api/ 2>/dev/null || true
	cp -r languages/cpp/api/. build/docusaurus/languages/cpp/api/ 2>/dev/null || true

docusaurus-build: ensure-node
	cd build/docusaurus && \
		if [ ! -d node_modules ]; then npm install --legacy-peer-deps; fi && \
		npx docusaurus build --out-dir ../output-html

docs-only: doxygen-c doxygen-cpp md-c md-cpp

build: docs-only prepare-docusaurus docusaurus-build
