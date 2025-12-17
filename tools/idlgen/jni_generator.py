"""JNI Generator - generates Java Native Interface bindings"""

from .types import ParsedIDL, Interface, Method, Member, Param
from .type_mapper import TypeMapper


class JNIGenerator:
    """Generates JNI bindings for Java interop"""

    def __init__(self, idl: ParsedIDL, namespace: str, java_package: str = ""):
        self.idl = idl
        self.namespace = namespace
        self.java_package = java_package or namespace.replace("_", ".")

    def generate_jni_header(self) -> str:
        """Generate JNI C header"""
        guard = f"{self.namespace.upper()}_JNI_H"
        lines = [
            "// AUTO-GENERATED - DO NOT EDIT",
            f"#ifndef {guard}",
            f"#define {guard}",
            "",
            "#include <jni.h>",
            "",
            "#ifdef __cplusplus",
            'extern "C" {',
            "#endif",
            "",
        ]

        for iface in self.idl.interfaces:
            lines.extend(self._jni_method_decls(iface))

        lines.extend([
            "#ifdef __cplusplus",
            "}",
            "#endif",
            "",
            f"#endif // {guard}",
        ])

        return "\n".join(lines)

    def generate_jni_impl(self, impl_header: str) -> str:
        """Generate JNI C++ implementation"""
        lines = [
            "// AUTO-GENERATED - DO NOT EDIT",
            f'#include "{self.namespace}_jni.h"',
            f'#include "{impl_header}"',
            "",
            "#include <memory>",
            "#include <string>",
            "#include <vector>",
            "",
        ]

        # Helper functions
        lines.extend([
            "namespace {",
            "",
            "std::string jstringToString(JNIEnv* env, jstring jstr) {",
            "    if (!jstr) return {};",
            "    const char* chars = env->GetStringUTFChars(jstr, nullptr);",
            "    std::string result(chars);",
            "    env->ReleaseStringUTFChars(jstr, chars);",
            "    return result;",
            "}",
            "",
            "jlong ptrToJlong(void* ptr) {",
            "    return reinterpret_cast<jlong>(ptr);",
            "}",
            "",
            "template<typename T>",
            "T* jlongToPtr(jlong handle) {",
            "    return reinterpret_cast<T*>(handle);",
            "}",
            "",
            "} // namespace",
            "",
        ])

        for iface in self.idl.interfaces:
            lines.extend(self._jni_method_impls(iface))

        return "\n".join(lines)

    def generate_java_class(self, iface: Interface) -> str:
        """Generate Java class for an interface"""
        class_name = iface.name
        lines = [
            "// AUTO-GENERATED - DO NOT EDIT",
            f"package {self.java_package};",
            "",
            "import java.util.ArrayList;",
            "import java.util.List;",
            "",
        ]

        # Generate struct classes first
        for struct in self.idl.structs:
            lines.extend(self._java_struct_class(struct))

        # Main class
        lines.extend([
            f"public class {class_name} implements AutoCloseable {{",
            "",
            "    static {",
            f'        System.loadLibrary("{self.namespace}_jni");',
            "    }",
            "",
            "    private long nativeHandle;",
            "",
        ])

        # Constructor
        ctor = next((m for m in iface.methods if m.is_constructor), None)
        if ctor:
            java_params = ", ".join(self._param_to_java(p) for p in ctor.params)
            native_args = ", ".join(p.name for p in ctor.params)
            lines.extend([
                f"    public {class_name}({java_params}) {{",
                f"        this.nativeHandle = nativeCreate({native_args});",
                "        if (this.nativeHandle == 0) {",
                f'            throw new RuntimeException("Failed to create {class_name}");',
                "        }",
                "    }",
                "",
            ])

        # Close method
        lines.extend([
            "    @Override",
            "    public void close() {",
            "        if (nativeHandle != 0) {",
            "            nativeDestroy(nativeHandle);",
            "            nativeHandle = 0;",
            "        }",
            "    }",
            "",
        ])

        # Public methods
        for method in iface.methods:
            if method.is_constructor:
                continue
            lines.extend(self._java_method(iface, method))

        # Native method declarations
        lines.append("    // Native methods")
        if ctor:
            native_params = ", ".join(self._param_to_java(p) for p in ctor.params)
            lines.append(f"    private static native long nativeCreate({native_params});")
        lines.append("    private static native void nativeDestroy(long handle);")

        for method in iface.methods:
            if method.is_constructor:
                continue
            lines.append(self._native_method_decl(method))

        lines.extend([
            "}",
            "",
        ])

        return "\n".join(lines)

    def _java_struct_class(self, struct) -> list[str]:
        """Generate Java class for a struct"""
        lines = [
            f"class {struct.name} {{",
        ]
        
        for m in struct.members:
            java_type = self._idl_to_java_type(m.type)
            lines.append(f"    public {java_type} {m.name};")
        
        # Constructor
        params = ", ".join(f"{self._idl_to_java_type(m.type)} {m.name}" for m in struct.members)
        lines.append("")
        lines.append(f"    public {struct.name}({params}) {{")
        for m in struct.members:
            lines.append(f"        this.{m.name} = {m.name};")
        lines.append("    }")
        
        lines.extend([
            "}",
            "",
        ])
        return lines

    def _java_method(self, iface: Interface, method: Method) -> list[str]:
        """Generate Java public method"""
        ret_type = self._return_to_java_type(method.return_type)
        params = ", ".join(self._param_to_java(p) for p in method.params)
        native_args = "nativeHandle"
        if method.params:
            native_args += ", " + ", ".join(p.name for p in method.params)

        lines = []
        
        if TypeMapper.is_vector(method.return_type):
            inner = TypeMapper.vector_inner(method.return_type)
            struct = next((s for s in self.idl.structs if s.name == inner), None)
            
            lines.append(f"    public List<{inner}> {method.name}({params}) {{")
            lines.append(f"        return native{method.name[0].upper()}{method.name[1:]}({native_args});")
            lines.append("    }")
        else:
            lines.append(f"    public {ret_type} {method.name}({params}) {{")
            lines.append(f"        return native{method.name[0].upper()}{method.name[1:]}({native_args});")
            lines.append("    }")
        
        lines.append("")
        return lines

    def _native_method_decl(self, method: Method) -> str:
        """Generate native method declaration"""
        ret_type = self._return_to_java_type(method.return_type)
        native_name = f"native{method.name[0].upper()}{method.name[1:]}"
        params = ["long handle"] + [self._param_to_java(p) for p in method.params]
        return f"    private static native {ret_type} {native_name}({', '.join(params)});"

    def _jni_method_decls(self, iface: Interface) -> list[str]:
        """Generate JNI method declarations in header"""
        jni_class = self._jni_class_name(iface.name)
        lines = []

        ctor = next((m for m in iface.methods if m.is_constructor), None)
        if ctor:
            params = ["JNIEnv*", "jclass"] + [self._param_to_jni_type(p) for p in ctor.params]
            lines.append(f"JNIEXPORT jlong JNICALL {jni_class}_nativeCreate({', '.join(params)});")
            lines.append(f"JNIEXPORT void JNICALL {jni_class}_nativeDestroy(JNIEnv*, jclass, jlong);")

        for method in iface.methods:
            if method.is_constructor:
                continue
            ret = self._return_to_jni_type(method.return_type)
            native_name = f"native{method.name[0].upper()}{method.name[1:]}"
            params = ["JNIEnv*", "jclass", "jlong"] + [self._param_to_jni_type(p) for p in method.params]
            lines.append(f"JNIEXPORT {ret} JNICALL {jni_class}_{native_name}({', '.join(params)});")

        lines.append("")
        return lines

    def _jni_method_impls(self, iface: Interface) -> list[str]:
        """Generate JNI method implementations"""
        jni_class = self._jni_class_name(iface.name)
        cpp_class = f"{self.namespace}::{iface.name}"
        lines = []

        ctor = next((m for m in iface.methods if m.is_constructor), None)
        if ctor:
            jni_params = ", ".join(["JNIEnv* env", "jclass"] + 
                                   [f"{self._param_to_jni_type(p)} {p.name}" for p in ctor.params])
            lines.append(f"JNIEXPORT jlong JNICALL {jni_class}_nativeCreate({jni_params}) {{")
            lines.append("    try {")
            
            # Convert parameters
            for p in ctor.params:
                if p.type == "string":
                    lines.append(f"        std::string cpp_{p.name} = jstringToString(env, {p.name});")
            
            cpp_args = ", ".join(f"cpp_{p.name}" if p.type == "string" else p.name for p in ctor.params)
            lines.append(f"        auto* obj = new {cpp_class}({cpp_args});")
            lines.append("        return ptrToJlong(obj);")
            lines.append("    } catch (...) {")
            lines.append("        return 0;")
            lines.append("    }")
            lines.append("}")
            lines.append("")

            lines.append(f"JNIEXPORT void JNICALL {jni_class}_nativeDestroy(JNIEnv*, jclass, jlong handle) {{")
            lines.append(f"    delete jlongToPtr<{cpp_class}>(handle);")
            lines.append("}")
            lines.append("")

        for method in iface.methods:
            if method.is_constructor:
                continue
            lines.extend(self._jni_method_impl(iface, method, jni_class, cpp_class))

        return lines

    def _jni_method_impl(self, iface: Interface, method: Method, jni_class: str, cpp_class: str) -> list[str]:
        """Generate single JNI method implementation"""
        ret = self._return_to_jni_type(method.return_type)
        native_name = f"native{method.name[0].upper()}{method.name[1:]}"
        
        jni_params = ", ".join(
            ["JNIEnv* env", "jclass", "jlong handle"] +
            [f"{self._param_to_jni_type(p)} {p.name}" for p in method.params]
        )
        
        lines = [f"JNIEXPORT {ret} JNICALL {jni_class}_{native_name}({jni_params}) {{"]
        lines.append(f"    auto* obj = jlongToPtr<{cpp_class}>(handle);")
        lines.append("    if (!obj) {")
        
        if TypeMapper.is_vector(method.return_type):
            lines.append("        return nullptr;")
        elif method.return_type == "bool":
            lines.append("        return JNI_FALSE;")
        else:
            lines.append("        return 0;")
        
        lines.append("    }")
        
        # Convert parameters
        cpp_arg_names = []
        for p in method.params:
            if p.type == "string":
                lines.append(f"    std::string cpp_{p.name} = jstringToString(env, {p.name});")
                cpp_arg_names.append(f"cpp_{p.name}")
            elif p.type == "uint8_t" and p.is_pointer:
                # Convert jbyteArray to uint8_t*
                lines.append(f"    jbyte* cpp_{p.name}_ptr = env->GetByteArrayElements({p.name}, nullptr);")
                lines.append(f"    const uint8_t* cpp_{p.name} = reinterpret_cast<const uint8_t*>(cpp_{p.name}_ptr);")
                cpp_arg_names.append(f"cpp_{p.name}")
            else:
                cpp_arg_names.append(p.name)
        
        cpp_args = ", ".join(cpp_arg_names)
        
        if TypeMapper.is_vector(method.return_type):
            inner = TypeMapper.vector_inner(method.return_type)
            struct = next((s for s in self.idl.structs if s.name == inner), None)
            
            lines.append(f"    auto result = obj->{method.name}({cpp_args});")
            
            # Release byte arrays
            for p in method.params:
                if p.type == "uint8_t" and p.is_pointer:
                    lines.append(f"    env->ReleaseByteArrayElements({p.name}, cpp_{p.name}_ptr, JNI_ABORT);")
            
            lines.append("")
            lines.append(f'    jclass listClass = env->FindClass("java/util/ArrayList");')
            lines.append('    jmethodID listCtor = env->GetMethodID(listClass, "<init>", "()V");')
            lines.append('    jmethodID listAdd = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");')
            lines.append("    jobject list = env->NewObject(listClass, listCtor);")
            lines.append("")
            
            if struct:
                java_class_path = self.java_package.replace(".", "/") + "/" + inner
                lines.append(f'    jclass itemClass = env->FindClass("{java_class_path}");')
                
                # Build constructor signature
                sig_parts = []
                for m in struct.members:
                    sig_parts.append(self._java_type_signature(m.type))
                sig = "(" + "".join(sig_parts) + ")V"
                
                lines.append(f'    jmethodID itemCtor = env->GetMethodID(itemClass, "<init>", "{sig}");')
                lines.append("")
                lines.append("    for (const auto& item : result) {")
                
                ctor_args = ", ".join(f"item.{m.name}" for m in struct.members)
                lines.append(f"        jobject jitem = env->NewObject(itemClass, itemCtor, {ctor_args});")
                lines.append("        env->CallBooleanMethod(list, listAdd, jitem);")
                lines.append("    }")
            
            lines.append("    return list;")
        elif method.return_type == "bool":
            lines.append(f"    auto ret = obj->{method.name}({cpp_args});")
            # Release byte arrays
            for p in method.params:
                if p.type == "uint8_t" and p.is_pointer:
                    lines.append(f"    env->ReleaseByteArrayElements({p.name}, cpp_{p.name}_ptr, JNI_ABORT);")
            lines.append("    return ret ? JNI_TRUE : JNI_FALSE;")
        else:
            lines.append(f"    auto ret = obj->{method.name}({cpp_args});")
            # Release byte arrays
            for p in method.params:
                if p.type == "uint8_t" and p.is_pointer:
                    lines.append(f"    env->ReleaseByteArrayElements({p.name}, cpp_{p.name}_ptr, JNI_ABORT);")
            lines.append("    return ret;")
        
        lines.append("}")
        lines.append("")
        return lines

    def _jni_class_name(self, class_name: str) -> str:
        """Convert to JNI class name format.
        
        In JNI, underscores in Java identifiers must be escaped as '_1'
        before converting dots to underscores.
        """
        # First escape underscores in package name (before dot replacement)
        pkg = self.java_package.replace("_", "_1").replace(".", "_")
        # Escape underscores in class name too
        escaped_class = class_name.replace("_", "_1")
        return f"Java_{pkg}_{escaped_class}"

    def _param_to_java(self, param: Param) -> str:
        """Convert param to Java declaration"""
        java_type = self._idl_to_java_type(param.type)
        if param.is_pointer and param.type == "uint8_t":
            java_type = "byte[]"
        return f"{java_type} {param.name}"

    def _param_to_jni_type(self, param: Param) -> str:
        """Convert param to JNI type"""
        if param.type == "string":
            return "jstring"
        if param.type == "int":
            return "jint"
        if param.type == "bool":
            return "jboolean"
        if param.is_pointer and param.type == "uint8_t":
            return "jbyteArray"
        return "jint"

    def _idl_to_java_type(self, idl_type: str) -> str:
        """Convert IDL type to Java type"""
        mapping = {
            "int": "int",
            "bool": "boolean",
            "string": "String",
            "float": "float",
            "double": "double",
            "uint8_t": "byte",
        }
        return mapping.get(idl_type, idl_type)

    def _return_to_java_type(self, idl_type: str) -> str:
        """Convert return type to Java type"""
        if TypeMapper.is_vector(idl_type):
            inner = TypeMapper.vector_inner(idl_type)
            return f"List<{inner}>"
        return self._idl_to_java_type(idl_type)

    def _return_to_jni_type(self, idl_type: str) -> str:
        """Convert return type to JNI type"""
        if TypeMapper.is_vector(idl_type):
            return "jobject"
        if idl_type == "bool":
            return "jboolean"
        if idl_type == "string":
            return "jstring"
        return "jint"

    def _java_type_signature(self, idl_type: str) -> str:
        """Get JNI type signature for Java type"""
        mapping = {
            "int": "I",
            "bool": "Z",
            "float": "F",
            "double": "D",
            "string": "Ljava/lang/String;",
        }
        return mapping.get(idl_type, "I")
