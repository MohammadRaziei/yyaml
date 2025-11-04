# YYAML Python Wrapper

This directory contains a Cython-based Python wrapper for the yyaml C library, providing a Pythonic interface to the lightweight YAML parser.

## Overview

The yyaml Python wrapper allows you to:
- Parse YAML strings and files with high performance
- Access parsed data through Python objects
- Serialize Python data back to YAML
- Handle YAML parsing errors with detailed error messages

## Requirements

- Python 3.6+
- Cython (for building from source)
- CMake (for building the yyaml C library)
- C compiler (GCC, Clang, or MSVC)

## Installation

### Building from Source

1. **Install dependencies:**
   ```bash
   pip install cython
   ```

2. **Build the extension:**
   ```bash
   cd examples
   python setup.py build_ext --inplace
   ```

3. **Test the installation:**
   ```bash
   python example_usage.py
   ```

## Usage Examples

### Basic Usage

```python
from yyaml import loads

yaml_string = """
name: John Doe
age: 30
active: true
scores: [95, 87, 92]
"""

data = loads(yaml_string)
print(data)
# Output: {'name': 'John Doe', 'age': 30, 'active': True, 'scores': [95, 87, 92]}
```

### Document-based Usage

```python
from yyaml import YYAMLDocument

yaml_string = """
users:
  - name: Alice
    age: 25
  - name: Bob  
    age: 30
"""

doc = YYAMLDocument.loads(yaml_string)
root = doc.root

print(f"Document has {len(doc)} nodes")
print(f"Root type: {root.type}")  # 'mapping'

# Access nested data
users = root.value['users']
for user in users:
    print(f"User: {user.value}")

# Serialize back to YAML
serialized = doc.dumps(indent=2)
print(serialized)
```

### Error Handling

```python
from yyaml import loads, YYAMLError

invalid_yaml = """
name: John
age: thirty  # Invalid: should be a number
- item1
"""

try:
    data = loads(invalid_yaml)
except YYAMLError as e:
    print(f"YAML parsing error: {e}")
    # Output: YAML parsing error: parse error at line 3, column 1 (position 25)
```

## API Reference

### Functions

- `loads(yaml_string, **kwargs)` - Parse YAML string and return Python object
- `load(yaml_bytes, **kwargs)` - Parse YAML bytes and return Python object

### Classes

- `YYAMLDocument` - Main document class for parsing and serialization
  - `loads(yaml_string, **kwargs)` - Class method to create document from string
  - `load(yaml_bytes, **kwargs)` - Class method to create document from bytes
  - `root` - Property returning the root node
  - `dumps(**kwargs)` - Serialize document to YAML string
  - `__len__()` - Number of nodes in document

- `YYAMLNode` - Wrapper for YAML nodes
  - `type` - Node type ('null', 'bool', 'int', 'double', 'string', 'sequence', 'mapping')
  - `value` - Python representation of the node value

- `YYAMLError` - Exception for YAML parsing errors

### Options

**Reading Options:**
- `allow_duplicate_keys` (bool): Allow duplicate mapping keys (default: False)
- `allow_trailing_content` (bool): Ignore trailing non-empty content (default: False)
- `allow_inf_nan` (bool): Parse inf/nan literals (default: False)
- `max_nesting` (int): Maximum indentation nesting level (default: 64)

**Writing Options:**
- `indent` (int): Spaces per indentation level (default: 2)
- `final_newline` (bool): Append trailing newline (default: True)

## Performance

The yyaml Python wrapper provides excellent performance due to:
- Direct C library integration via Cython
- Minimal Python object creation overhead
- Efficient memory management
- Optimized parsing algorithms

## File Structure

- `yyaml.pyx` - Cython source code for the wrapper
- `setup.py` - Build configuration and installation script
- `example_usage.py` - Comprehensive usage examples
- `README.md` - This documentation file

## Building for Distribution

To create a distributable package:

```bash
cd examples
python setup.py sdist bdist_wheel
```

This will create source distribution and wheel packages in the `dist/` directory.

## Troubleshooting

### Common Issues

1. **ImportError: No module named 'yyaml'**
   - Make sure you've built the extension with `python setup.py build_ext --inplace`

2. **Cython not found**
   - Install Cython: `pip install cython`

3. **CMake not found**
   - Install CMake for your platform

4. **Library not found errors**
   - Ensure the yyaml C library is built in the `../build` directory

## License

Same as the main yyaml project.
