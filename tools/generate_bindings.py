#!/usr/bin/env python3
"""
Generic IDL Code Generator

Parses C++-like IDL definitions and generates:
  1. C API exports (header + implementation)
  2. C++ client wrapper for dynamic loading
  3. Emscripten WASM bindings
  4. JNI bindings for Java interop (optional)

Usage:
    python generate_bindings.py input.idl --output-dir generated/
    python generate_bindings.py input.idl --output-dir generated/ --java --java-package com.example
"""

import argparse
import time
from pathlib import Path

from idlgen import (
    IDLParser,
    CAPIGenerator,
    ClientGenerator,
    WASMGenerator,
    JNIGenerator,
)


def main():
    start_time = time.perf_counter()
    
    parser = argparse.ArgumentParser(description="Generate bindings from IDL")
    parser.add_argument("idl_file", help="Path to IDL file")
    parser.add_argument("--output-dir", "-o", default="generated", help="Output directory")
    parser.add_argument("--namespace", "-n", default="", help="C++ namespace")
    parser.add_argument("--impl-header", default="", help="Implementation header to include")
    parser.add_argument("--java", action="store_true", help="Generate Java/JNI bindings")
    parser.add_argument("--java-package", default="", help="Java package name")
    parser.add_argument("--java-output-dir", default="", help="Java source output directory")
    args = parser.parse_args()

    idl_path = Path(args.idl_file)
    namespace = args.namespace or idl_path.stem.replace("-", "_")
    impl_header = args.impl_header or f"{namespace}.hpp"

    idl = IDLParser(idl_path.read_text()).parse()

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    c_api = CAPIGenerator(idl, namespace)
    client = ClientGenerator(idl, namespace)
    wasm = WASMGenerator(idl, namespace)

    files = {
        f"{namespace}_c_api.h": c_api.generate_header(),
        f"{namespace}_c_api.cpp": c_api.generate_impl(impl_header),
        f"{namespace}_client.hpp": client.generate_header(),
        f"{namespace}_client.cpp": client.generate_impl(),
        f"{namespace}_wasm_bindings.cpp": wasm.generate(impl_header),
    }

    # Generate JNI bindings if requested
    if args.java:
        java_package = args.java_package or namespace.replace("_", ".")
        jni = JNIGenerator(idl, namespace, java_package)
        
        files[f"{namespace}_jni.h"] = jni.generate_jni_header()
        files[f"{namespace}_jni.cpp"] = jni.generate_jni_impl(impl_header)
        
        # Generate Java classes
        java_output = Path(args.java_output_dir) if args.java_output_dir else output_dir / "java"
        java_pkg_dir = java_output / java_package.replace(".", "/")
        java_pkg_dir.mkdir(parents=True, exist_ok=True)
        
        for iface in idl.interfaces:
            java_path = java_pkg_dir / f"{iface.name}.java"
            java_path.write_text(jni.generate_java_class(iface))
            print(f"Generated: {java_path}")

    for filename, content in files.items():
        path = output_dir / filename
        path.write_text(content)
        print(f"Generated: {path}")

    elapsed = time.perf_counter() - start_time
    print(f"Generation completed in {elapsed*1000:.2f} ms")


if __name__ == "__main__":
    main()
