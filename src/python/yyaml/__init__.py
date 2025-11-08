"""Python convenience layer for the yyaml extension module."""

try:  # pragma: no cover - executed during runtime import
    from .yyaml_python import (  # type: ignore[attr-defined]
        YYAMLError,
        YYAMLDocument,
        dict2yyaml,
        yyaml2dict,
        dump,
        dumps,
        load,
        loads,
        yyaml_dump,
        yyaml_dumps,
        yyaml_load,
        yyaml_loads,
    )
except ImportError as exc:  # pragma: no cover
    raise ImportError("yyaml extension module is not built") from exc

__all__ = [
    "YYAMLError",
    "YYAMLDocument",
    "dict2yyaml",
    "yyaml2dict",
    "dump",
    "dumps",
    "load",
    "loads",
    "yyaml_dump",
    "yyaml_dumps",
    "yyaml_load",
    "yyaml_loads",
]
