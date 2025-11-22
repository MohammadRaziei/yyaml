# yyaml 

![yyaml-logo](docs/static/img/yyaml.svg)

yyaml is a high-performance YAML parser and emitter written in C with ergonomic C++ bindings. It is optimized for speed and low memory usage, inspired by the architecture of yyjson, and designed to slot into latency-sensitive systems.

## Features
- Fast C core with streaming-friendly parser and emitter layers
- C++ bindings that wrap the C API with RAII helpers and type-safe wrappers
- Benchmark harness to compare yyaml with other YAML libraries
- Documentation pipeline that turns source comments into a static site

## Documentation architecture
- Clean content lives in `docs/` (language overviews, benchmark page, configs)
- Generated assets stay under `docs/build/` to keep the repo tidy
- Final static site is emitted to `docs/build/output-html/`
- Source API docs are produced in `docs/languages/<lang>/api/` via Doxygen + Doxybook2

## Build instructions
1. `cd docs`
2. `make`

## Build pipeline
`Doxygen → Doxybook2 → Markdown → Docusaurus → output-html`

- Doxygen extracts XML from the C sources (`src`, `include`) and C++ bindings (`bindings/cpp`).
- Doxybook2 converts XML into Docusaurus-friendly Markdown inside `docs/languages/<lang>/api/`.
- The Makefile copies the docs, benchmarks, and generated API pages into `docs/build/docusaurus/`.
- Docusaurus builds the static site into `docs/build/output-html/`.

## Repository pointers
- C implementation: `src/` and public headers in `include/`
- C++ bindings: `bindings/cpp/`
- Benchmarks: `benchmarks/python/`

Happy hacking and fast YAML parsing!
