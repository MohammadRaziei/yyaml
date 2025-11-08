# cython: language_level=3
"""Cython bindings for the yyaml C library."""

def hello():
    """Simple test function to verify Cython compilation works."""
    return "Hello from yyaml Python bindings!"

__all__ = ["hello"]
