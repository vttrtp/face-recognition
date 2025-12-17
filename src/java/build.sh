#!/bin/bash
# Build and run Java face detector sample

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "=== Java Face Detector Sample Build Script ==="
echo

# Check for Java
if ! command -v java &> /dev/null; then
    echo "Error: Java not found. Please install JDK 11 or later."
    exit 1
fi

if ! command -v javac &> /dev/null; then
    echo "Error: javac not found. Please install JDK 11 or later."
    exit 1
fi

echo "Java version:"
java -version
echo

# Generate JNI bindings
echo "Generating JNI bindings..."
cd "$PROJECT_ROOT/tools"
python3 generate_bindings.py \
    "$PROJECT_ROOT/src/facedetector/face_detector.idl" \
    -o "$PROJECT_ROOT/src/generated" \
    -n face_detector \
    --impl-header face_detector.hpp \
    --java \
    --java-package face_detector \
    --java-output-dir "$SCRIPT_DIR/src/main/java"

echo

# Compile Java sources
echo "Compiling Java sources..."
cd "$SCRIPT_DIR"
mkdir -p out
javac -d out src/main/java/face_detector/*.java

echo "Compilation successful!"
echo

# Check if native library exists
JNI_LIB="$BUILD_DIR/lib/libface_detector_jni.so"
if [ ! -f "$JNI_LIB" ]; then
    echo "Warning: Native JNI library not found at $JNI_LIB"
    echo "Please build the native library first with: cmake --build build"
    echo
    echo "To run the sample after building:"
    echo "  java -Djava.library.path=$BUILD_DIR/lib -cp out face_detector.FaceDetectorDemo <cascade> <image>"
    exit 0
fi

# Run if cascade file exists
CASCADE="$BUILD_DIR/data/haarcascade_frontalface_default.xml"
if [ ! -f "$CASCADE" ]; then
    echo "Warning: Cascade file not found at $CASCADE"
    exit 0
fi

# Check for sample image
if [ -n "$1" ]; then
    IMAGE="$1"
else
    echo "Usage: $0 <image_path>"
    echo
    echo "Example:"
    echo "  $0 /path/to/test/image.jpg"
    exit 0
fi

echo "Running Face Detector..."
java -Djava.library.path="$BUILD_DIR/lib" \
     -cp out \
     face_detector.FaceDetectorDemo \
     "$CASCADE" \
     "$IMAGE"
