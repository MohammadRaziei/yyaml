---
title: yyaml Languages
id: languages-index
slug: /languages
---

# Choose a language

yyaml ships a C core with C++ bindings layered on top. Pick the stack that matches your runtime and deployment model.

- [C API](c/index.md): fastest path with direct access to allocators, parser states, and emitter controls.
- [C++ API](cpp/index.md): idiomatic wrappers with RAII, strong types, and iterator-friendly traversal over YAML nodes.

Both paths share the same parser and emitter engine, so performance characteristics are consistent. The generated API references live under each language's `api/` directory after running the docs pipeline.
