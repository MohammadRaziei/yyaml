#!/usr/bin/env python3
"""
Example usage of the yyaml Python wrapper

This demonstrates how to use the Cython-wrapped yyaml library from Python.
"""

import sys
import os

# Add the current directory to Python path to import the local yyaml module
sys.path.insert(0, os.path.dirname(__file__))

try:
    from yyaml import loads, YYAMLDocument, YYAMLError
    print("✓ Successfully imported yyaml module")
except ImportError as e:
    print(f"✗ Failed to import yyaml: {e}")
    print("Make sure you've built the extension first:")
    print("cd examples && python setup.py build_ext --inplace")
    sys.exit(1)

def basic_example():
    """Basic YAML parsing example"""
    print("\n" + "="*50)
    print("BASIC YAML PARSING")
    print("="*50)
    
    yaml_string = """
name: John Doe
age: 30
active: true
height: 175.5
scores: [95, 87, 92]
address:
  street: 123 Main St
  city: Anytown
  zipcode: "12345"
"""
    
    try:
        result = loads(yaml_string)
        print("Parsed YAML:")
        print(result)
        print(f"\nType of result: {type(result)}")
    except YYAMLError as e:
        print(f"Error parsing YAML: {e}")

def document_example():
    """Example using YYAMLDocument for more control"""
    print("\n" + "="*50)
    print("DOCUMENT-BASED USAGE")
    print("="*50)
    
    yaml_string = """
users:
  - name: Alice
    age: 25
    roles: [admin, user]
  - name: Bob
    age: 30
    roles: [user]
config:
  debug: false
  timeout: 30
  retries: 3
"""
    
    try:
        doc = YYAMLDocument.loads(yaml_string)
        print(f"Document has {len(doc)} nodes")
        
        root = doc.root
        print(f"Root type: {root.type}")
        print(f"Root value type: {type(root.value)}")
        
        # Access nested data
        users = root.value['users']
        print(f"\nNumber of users: {len(users)}")
        
        for i, user in enumerate(users):
            print(f"User {i}: {user.value}")
        
        # Serialize back to YAML
        serialized = doc.dumps(indent=2)
        print(f"\nSerialized YAML:\n{serialized}")
        
    except YYAMLError as e:
        print(f"Error: {e}")

def error_handling_example():
    """Example showing error handling"""
    print("\n" + "="*50)
    print("ERROR HANDLING")
    print("="*50)
    
    invalid_yaml = """
name: John
age: thirty  # This should be a number
- item1
- item2
"""
    
    try:
        result = loads(invalid_yaml)
        print("Unexpectedly parsed invalid YAML:", result)
    except YYAMLError as e:
        print(f"Caught expected error: {e}")

def performance_example():
    """Simple performance comparison example"""
    print("\n" + "="*50)
    print("PERFORMANCE EXAMPLE")
    print("="*50)
    
    import time
    
    # Create a moderately sized YAML
    yaml_data = "data:\n"
    for i in range(100):
        yaml_data += f"  item{i}: value{i}\n"
    
    # Time the parsing
    start_time = time.time()
    try:
        result = loads(yaml_data)
        end_time = time.time()
        print(f"Parsed 100 items in {end_time - start_time:.4f} seconds")
        print(f"Number of items parsed: {len(result['data'])}")
    except YYAMLError as e:
        print(f"Error during performance test: {e}")

if __name__ == "__main__":
    print("YYAML Python Wrapper Examples")
    print("="*50)
    
    basic_example()
    document_example()
    error_handling_example()
    performance_example()
    
    print("\n" + "="*50)
    print("All examples completed!")
    print("="*50)
