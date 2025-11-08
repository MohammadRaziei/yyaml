"""Python convenience layer for the yyaml extension module."""

try:  # pragma: no cover - executed during runtime import
    from .yyaml_python import hello  # type: ignore[attr-defined]
except ImportError as exc:  # pragma: no cover
    raise ImportError("yyaml extension module is not built") from exc

__all__ = ["hello"]
