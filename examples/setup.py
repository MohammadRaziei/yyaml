from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import os
import sys
import subprocess

# Check if Cython is available
try:
    from Cython.Build import cythonize
    USE_CYTHON = True
except ImportError:
    USE_CYTHON = False
    print("Warning: Cython not found. Using pre-generated C file if available.")

class CustomBuildExt(build_ext):
    """Custom build extension to handle yyaml library dependencies"""
    
    def build_extensions(self):
        # Add include directories
        include_dirs = [
            os.path.join(os.path.dirname(__file__), '..', 'includes'),
            os.path.join(os.path.dirname(__file__), '..', 'src')
        ]
        
        for ext in self.extensions:
            ext.include_dirs.extend(include_dirs)
            
            # Add compiler flags
            if self.compiler.compiler_type == 'unix':
                ext.extra_compile_args.extend(['-Wall', '-Wextra', '-O2'])
            elif self.compiler.compiler_type == 'msvc':
                ext.extra_compile_args.extend(['/W3', '/O2'])
        
        # First build the yyaml static library if needed
        self.build_yyaml_library()
        
        super().build_extensions()
    
    def build_yyaml_library(self):
        """Build the yyaml C library"""
        project_root = os.path.join(os.path.dirname(__file__), '..')
        build_dir = os.path.join(project_root, 'build')
        
        # Create build directory if it doesn't exist
        os.makedirs(build_dir, exist_ok=True)
        
        # Configure with CMake
        cmake_cmd = [
            'cmake', 
            '-B', build_dir,
            '-S', project_root,
            '-DCMAKE_BUILD_TYPE=Release'
        ]
        
        print("Configuring yyaml library...")
        result = subprocess.run(cmake_cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"CMake configuration failed: {result.stderr}")
            return
        
        # Build the library
        build_cmd = ['cmake', '--build', build_dir, '--target', 'yyaml']
        print("Building yyaml library...")
        result = subprocess.run(build_cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Build failed: {result.stderr}")
            return
        
        print("yyaml library built successfully!")

# Define the extension
ext = '.pyx' if USE_CYTHON else '.c'
ext_modules = [
    Extension(
        "yyaml",
        [f"yyaml{ext}"],
        libraries=['yyaml'],
        library_dirs=[os.path.join('..', 'build')],
        include_dirs=[os.path.join('..', 'includes')],
        language="c",
    )
]

# Use Cython if available
if USE_CYTHON:
    ext_modules = cythonize(ext_modules, compiler_directives={'language_level': 3})

setup(
    name="yyaml",
    version="1.0.0",
    description="Python wrapper for yyaml - a lightweight YAML parser",
    author="YYAML Developers",
    ext_modules=ext_modules,
    cmdclass={'build_ext': CustomBuildExt},
    zip_safe=False,
    python_requires='>=3.6',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Cython',
        'Topic :: Software Development :: Libraries',
        'Topic :: Text Processing :: Markup',
    ],
)

# Create a simple installation script
if __name__ == "__main__":
    print("Building yyaml Python extension...")
    print("Make sure you have Cython installed: pip install cython")
    print("Also ensure CMake is available for building the yyaml C library")
