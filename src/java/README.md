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

## Notes

- Always use try-with-resources or call `close()` to release native resources
- The native library must be in the library path (`-Djava.library.path=...`)
- Ensure the cascade file exists and is readable
