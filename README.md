# Face Recognition

A cross-platform C++ application for detecting faces in images. The project consists of:
- A dynamic library (`face_detector`) for face detection using OpenCV Haar cascades
- A console application that processes images recursively and saves results
- A WebAssembly (WASM) module for browser-based face detection
- **Automatic binding generation** from IDL for C++ runtime loading, Java JNI, and WebAssembly

**[🔗 Live Demo](https://vttrtp.github.io/face-recognition/)**

> **Note:** The application is designed to be cross-platform but has only been tested on Linux and WebAssembly (browser).

## Features

- Detects faces in images using OpenCV's Haar cascade classifier
- Recursively processes directories with unlimited nesting
- Creates half-size copies of images with blurred face regions
- Saves detection results in JSON format
- Dynamic library loading at runtime
- Web-based face detection demo using WebAssembly
- **IDL-based code generation** for multiple platforms:
  - C API with dynamic library export and import
  - C++ client wrapper with runtime library loading
  - Emscripten/WebAssembly bindings
  - Java JNI bindings

## Requirements

- CMake 3.16+
- C++17 compatible compiler
- Python 3.8+ (for code generation)
- Ninja build system
- vcpkg package manager
- Emscripten SDK (for WASM build)

## Installing Prerequisites

### vcpkg

```bash
# Clone vcpkg (should be at ../vcpkg relative to this project)
cd ..
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh   # Linux/macOS
# or bootstrap-vcpkg.bat on Windows
```

### Emscripten SDK (for WASM build)

```bash
# Clone and install emsdk (should be at ../emsdk relative to this project)
cd ..
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh   # Linux/macOS
```

**Note:** Update the `EMSDK` path in `CMakePresets.json` to match your installation location if needed.

## Dependencies

Managed via vcpkg:
- OpenCV 4 (with jpeg, png, objdetect modules)
- jsoncpp
- Google Test (for unit tests)

## Building

1. Make sure vcpkg is installed at `../vcpkg` relative to this project (or set `VCPKG_ROOT` environment variable).

2. Configure and build (dependencies are installed automatically via vcpkg manifest mode):
   ```bash
   cmake --preset default
   cmake --build --preset default
   ```

   For debug build:
   ```bash
   cmake --preset debug
   cmake --build --preset debug
   ```

## Usage

```bash
./build/app/face_recognition_app <input_directory> [options]
```

### Options

- `--library <path>` - Path to face_detector library (auto-detected by default)
- `--cascade <path>` - Path to Haar cascade XML file
- `--output <path>` - Output directory for result images (preserves folder structure; defaults to saving next to originals)
- `--help` - Show help message

### Example

```bash
./build/src/app/face_recognition_app ./tests/data --output ./results
```

This will:
1. Find all images in `./tests/data` and its subdirectories
2. Detect faces in each image
3. Create half-size copies with blurred faces in `./results` (preserving folder structure)
4. Save `result.json` in the output directory

Without `--output`, result images are saved next to the original images.

## Output

### Result Images

For each processed image, a `*_result.jpg` file is created with:
- Half the original dimensions
- Gaussian blur applied to detected face regions

### JSON Output

`result.json` contains an array of results:
```json
[
  {
    "original_file": "/path/to/image.jpg",
    "result_file": "/path/to/image_result.jpg",
    "success": true,
    "faces": [
      {"x": 100, "y": 50, "width": 200, "height": 200}
    ],
    "face_count": 1
  }
]
```

## Running Tests

```bash
cd build
ctest --output-on-failure
```

Or run the test executable directly:
```bash
./build/tests/face_detector_test
```

## WebAssembly Build

### Building the WASM Module

```bash
cmake --preset wasm
cmake --build --preset wasm
```

This will:
1. Build the face detector library for WebAssembly
2. Create WASM bindings with Emscripten
3. Copy all necessary files to `build-wasm/web/`

### Running the Web Demo

Start a local HTTP server in the web directory:

```bash
cd build-wasm/web
python3 -m http.server 8080
```

Then open your browser to: http://localhost:8080

The demo allows you to:
- Load a test image (automatically loaded) or select your own
- Detect faces with a single click
- View detection results with green rectangles drawn around faces
- See detection time in milliseconds

## Project Structure

```
face-recognition/
├── CMakeLists.txt              # Root CMake configuration
├── CMakePresets.json           # CMake presets for vcpkg
├── vcpkg.json                  # vcpkg dependencies manifest
├── cmake/                      # CMake helper modules
├── triplets/                   # Custom vcpkg triplets
├── tools/
│   ├── generate_bindings.py    # Main code generator script
│   └── idlgen/                 # Code generator package
│       ├── parser.py           # IDL parser (C++-like syntax)
│       ├── type_mapper.py      # Type mapping utilities
│       ├── c_api_generator.py  # C API generator
│       ├── client_generator.py # C++ client wrapper generator
│       ├── wasm_generator.py   # Emscripten bindings generator
│       └── jni_generator.py    # Java JNI bindings generator
├── src/
│   ├── facedetector/           # Face detector library implementation
│   │   └── face_detector.idl   # IDL interface definition
│   ├── generated/              # Auto-generated bindings (C API, WASM, JNI)
│   ├── app/                    # Console application
│   ├── wasm/                   # WebAssembly module
│   └── web/                    # Web demo HTML
├── samples/
│   └── java/                   # Java JNI sample application
└── tests/                      # Unit tests
```

## Code Generation

The project uses a custom IDL (Interface Definition Language) to automatically generate:
- **C API** - Exported functions for the shared library
- **C++ Client** - Wrapper with dynamic library loading
- **WASM Bindings** - Emscripten JavaScript interop
- **Java JNI** - Native bindings for Java applications

### IDL Syntax (C++-like)

```cpp
struct FaceRect {
    int x;
    int y;
    int width;
    int height;
};

interface FaceDetector {
    constructor(const string& cascadePath);
    bool isLoaded() const;
    vector<FaceRect> detectFromFile(const string& imagePath);
};
```

### Generate Bindings

```bash
python tools/generate_bindings.py src/facedetector/face_detector.idl \
    --namespace face_detector \
    --output-dir src/generated \
    --java --java-output-dir samples/java/src/main/java
```

Bindings are auto-regenerated during CMake build when the IDL file changes.

## License

MIT License
