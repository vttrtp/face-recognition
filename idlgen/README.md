# IDL Code Generator

A Python-based code generator that creates C API, WASM (Emscripten), and JNI bindings from IDL definitions.

## Installation

```bash
pip install -e .
```

## Usage

```bash
python bin/generate_bindings.py <idl_file> \
    --output-dir <output_dir> \
    --namespace <namespace> \
    --impl-header <header.hpp> \
    [--java] \
    [--java-package <package>] \
    [--java-output-dir <dir>]
```

## Supported Generators

- **C API** - C-compatible API with opaque handles
- **Client** - C++ client that loads library dynamically
- **WASM** - Emscripten bindings for WebAssembly
- **JNI** - Java Native Interface bindings

## IDL Syntax

```idl
namespace MyLib;

struct Point {
    int x;
    int y;
}

callback OnResult(Point result);

interface Calculator {
    int add(int a, int b);
    void processAsync(OnResult callback);
}
```

## Testing

The `samples/` directory contains test samples for each language. Generated sources are stored in per-language test folders and are not under source control.

### Build and Run All Tests (from parent project)

```bash
# From face-recognition root
.vscode/build_all.sh
```

### Individual Test Commands

**C++ tests (using CMake presets from parent):**
```bash
cmake --preset default      # Configure
cmake --build --preset default  # Build
ctest --preset default      # Run tests (includes IDL samples tests)
```

**Java tests:**
```bash
# After native build completes
cd idlgen/samples/tests/java
javac -d out generated/idl/samples/*.java SamplesTest.java
java -Djava.library.path=../../../../build/idlgen/samples -cp out idl.samples.SamplesTest
```

**WASM tests:**
```bash
source ../emsdk/emsdk_env.sh  # Activate Emscripten
cmake --preset idl-wasm
cmake --build --preset idl-wasm --target samples_wasm
node build-wasm-idl/idlgen/samples/samples_test.js
```

## Directory Structure

```
idlgen/
├── pyproject.toml          # Python package configuration
├── README.md               # This file
├── bin/
│   └── generate_bindings.py  # CLI entry point
├── idlgen/                 # Python package
│   ├── __init__.py
│   ├── parser.py           # IDL parser
│   ├── type_mapper.py      # Type mapping utilities
│   ├── types.py            # Type definitions
│   ├── c_api_generator.py  # C API generator
│   ├── client_generator.py # C++ client generator
│   ├── wasm_generator.py   # WASM bindings generator
│   └── jni_generator.py    # JNI bindings generator
└── samples/
    ├── CMakeLists.txt      # Included from parent project
    ├── samples.idl         # Sample IDL definitions
    ├── samples.hpp         # Sample C++ implementation
    ├── samples.cpp         # Sample C++ implementation
    └── tests/
        ├── cpp/
        │   ├── samples_test.cpp
        │   └── generated/  # (gitignored) Generated C++ sources
        ├── java/
        │   ├── SamplesTest.java
        │   └── generated/  # (gitignored) Generated Java sources
        └── wasm/
            ├── samples_test.js
            └── generated/  # (gitignored) Generated WASM sources
```
