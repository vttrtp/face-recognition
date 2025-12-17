# Java Face Detector Sample

This sample demonstrates how to use the FaceDetector native library from Java using JNI bindings.

## Prerequisites

- Java JDK 11 or later
- The native JNI library (`libface_detector_jni.so` on Linux, `face_detector_jni.dll` on Windows)
- OpenCV cascade file (`haarcascade_frontalface_default.xml`)

## Building

### 1. Generate JNI Bindings

From the project root:

```bash
python tools/generate_bindings.py src/facedetector/face_detector.idl \
    --namespace face_detector \
    --output-dir src/generated \
    --impl-header face_detector.hpp \
    --java \
    --java-output-dir samples/java/src/main/java
```

### 2. Build the Native JNI Library

```bash
cd build
cmake .. -DBUILD_JAVA_JNI=ON
ninja  # or make
```

### 3. Compile Java Sources

```bash
cd samples/java
mkdir -p out
javac -d out src/main/java/face/detector/*.java
```

## Running

```bash
java -Djava.library.path=../../build/lib \
     -cp out \
     face.detector.FaceDetectorDemo \
     ../../build/data/haarcascade_frontalface_default.xml \
     /path/to/test/image.jpg
```

## Example Output

```
Face Detection Demo
==================
Cascade: ../../build/data/haarcascade_frontalface_default.xml
Image: test.jpg

Detector loaded successfully!
Detected 2 face(s):
  Face 1: x=120, y=85, width=150, height=150
  Face 2: x=320, y=95, width=140, height=140
```

## API Reference

### FaceDetector

```java
// Constructor - loads cascade classifier
FaceDetector(String cascadePath)

// Check if detector is loaded
boolean isLoaded()

// Detect faces in image file
List<FaceRect> detectFromFile(String imagePath)

// Detect faces in raw image data
List<FaceRect> detectFromImageData(byte[] data, int width, int height)

// Clean up native resources
void close()
```

### FaceRect

```java
class FaceRect {
    public int x;       // X coordinate of top-left corner
    public int y;       // Y coordinate of top-left corner
    public int width;   // Width of face rectangle
    public int height;  // Height of face rectangle
}
```

## Notes

- Always use try-with-resources or call `close()` to release native resources
- The native library must be in the library path (`-Djava.library.path=...`)
- Ensure the cascade file exists and is readable
