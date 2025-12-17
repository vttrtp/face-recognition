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
import sys
import time
from pathlib import Path

# Add parent directory to path so idlgen package can be found
sys.path.insert(0, str(Path(__file__).parent.parent))

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
    parser.add_argument("idl_file", nargs="?", help="Path to IDL file (positional)")
    parser.add_argument("--idl", help="Path to IDL file (alternative)")
    parser.add_argument("--output-dir", "-o", default="generated", help="Output directory")
    parser.add_argument("--namespace", "-n", default="", help="C++ namespace")
    parser.add_argument("--header", default="", help="Implementation header to include (alternative)")
    parser.add_argument("--impl-header", default="", help="Implementation header to include")
    parser.add_argument("--api-macro", default="", help="API export macro name")
    parser.add_argument("--java", action="store_true", help="Generate Java/JNI bindings")
    parser.add_argument("--java-package", default="", help="Java package name")
    parser.add_argument("--java-output-dir", default="", help="Java source output directory")
    parser.add_argument("--java-output", default="", help="Java source output directory (alternative)")
    # Explicit output file options (ignored, for compatibility)
    parser.add_argument("--c-api", default="", help="C API header output (ignored)")
    parser.add_argument("--c-api-impl", default="", help="C API impl output (ignored)")
    parser.add_argument("--client", default="", help="Client header output (ignored)")
    parser.add_argument("--client-impl", default="", help="Client impl output (ignored)")
    parser.add_argument("--jni-header", default="", help="JNI header output (ignored)")
    parser.add_argument("--jni-impl", default="", help="JNI impl output (ignored)")
    args = parser.parse_args()

    # Support both positional and --idl argument
    idl_file = args.idl_file or args.idl
    if not idl_file:
        parser.error("IDL file is required (positional or --idl)")
    
    idl_path = Path(idl_file)
    namespace = args.namespace or idl_path.stem.replace("-", "_")
    impl_header = args.impl_header or args.header or f"{namespace}.hpp"
    
    # Extract just the filename from the header path
    impl_header = Path(impl_header).name

    idl = IDLParser(idl_path.read_text()).parse()

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    api_macro = args.api_macro or f"{namespace.upper()}_API"

    c_api = CAPIGenerator(idl, namespace, api_macro)
    client = ClientGenerator(idl, namespace)
    wasm = WASMGenerator(idl, namespace)

    files = {
        f"{namespace}_c_api.h": c_api.generate_header(),
        f"{namespace}_c_api.cpp": c_api.generate_impl(impl_header),
        f"{namespace}_client.hpp": client.generate_header(),
        f"{namespace}_client.cpp": client.generate_impl(),
        f"{namespace}_wasm_bindings.cpp": wasm.generate(impl_header),
    }

    # Generate JNI bindings if requested (or if java-package/java-output is provided)
    java_output_dir = args.java_output_dir or args.java_output
    generate_java = args.java or args.java_package or java_output_dir
    
    if generate_java:
        java_package = args.java_package or namespace.replace("_", ".")
        jni = JNIGenerator(idl, namespace, java_package)
        
        files[f"{namespace}_jni.h"] = jni.generate_jni_header()
        files[f"{namespace}_jni.cpp"] = jni.generate_jni_impl(impl_header)
        
        # Generate Java classes
        java_output = Path(java_output_dir) if java_output_dir else output_dir / "java"
        # Always add the package path subdirectory
        java_pkg_dir = java_output / java_package.replace(".", "/")
        java_pkg_dir.mkdir(parents=True, exist_ok=True)
        
        # Generate shared types file (structs and callbacks)
        if idl.structs or idl.callbacks:
            types_path = java_pkg_dir / "Types.java"
            types_path.write_text(jni.generate_java_types())
            print(f"Generated: {types_path}")
        
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
