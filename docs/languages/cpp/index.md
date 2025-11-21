---
title: C++ API
id: cpp-index
slug: /languages/cpp
---

# C++ bindings overview

The C++ bindings wrap yyaml's C handles with modern abstractions so you can compose YAML documents using RAII-managed nodes, spans, and lightweight iterators.

## C++ layer highlights
- **RAII ownership** around parser/emitter handles and arenas to avoid manual cleanup.
- **Convenient views** converting YAML scalars to `std::string_view`, numeric types, and booleans.
- **Iterators & ranges** to walk mappings/sequences using standard algorithms.
- **Error handling** via exceptions or `std::error_code` depending on your build flags.

## Workflow
- Run `make docs-only` in `docs/` to refresh the generated API docs.
- Doxybook2 writes Markdown into `docs/languages/cpp/api/` using the Doxygen XML from `bindings/cpp/`.
- The `prepare-docusaurus` step copies the generated docs into the build site so navigation and search stay current.

## Integration pointers
- Prefer move semantics for large node graphs to avoid redundant copies.
- Use string views when bridging lifetimes into third-party allocators.
- Keep allocators consistent between C and C++ layers to prevent double-free hazards.
