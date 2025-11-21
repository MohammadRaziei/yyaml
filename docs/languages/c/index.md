---
title: C API
id: c-index
slug: /languages/c
---

# C API overview

yyaml's C API is the canonical interface, exposing a small set of structs and functions for parsing, emitting, and manipulating YAML documents with minimal overhead.

## Core components
- **Reader/Parser**: streaming lexer and parser that can build DOM nodes or stream events.
- **Emitter**: configurable serializer with options for canonical YAML, compact formatting, and custom sinks.
- **Memory strategy**: arena-backed allocations with user-supplied hooks to integrate with custom allocators.
- **Error model**: numeric error codes paired with optional diagnostic strings and offsets.

## Using the reference
- Run `make docs-only` inside `docs/` to produce fresh XML + Markdown.
- Generated Markdown lands in `docs/languages/c/api/` and is copied into the Docusaurus site during `make build`.
- Navigate the C API index once generated to explore functions, structs, enums, and typedefs.

## Integration tips
- Keep buffers aligned with yaml emitter expectations to avoid extra copies.
- Prefer reusable arenas for batch parsing to minimize allocations.
- Validate node ownership when bridging to foreign runtimes or FFI layers.
