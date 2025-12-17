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

    def generate_java_types(self) -> str:
        """Generate shared Java types file (structs and callbacks)"""
        lines = [
            "// AUTO-GENERATED - DO NOT EDIT",
            f"package {self.java_package};",
            "",
        ]

        # Generate callback functional interfaces
        for cb in self.idl.callbacks:
            lines.extend(self._java_callback_interface(cb))

        # Generate struct classes
        for struct in self.idl.structs:
            lines.extend(self._java_struct_class(struct))

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

        # Main class (no longer include shared types - they go in Types.java)
        lines.extend([
            f"public class {class_name} implements AutoCloseable {{",
            "",
            "    static {{",
            f'        System.loadLibrary("{self.namespace}_jni");',
            "    }}",
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

    def _java_callback_interface(self, cb) -> list[str]:
        """Generate Java functional interface for a callback"""
        params = ", ".join(f"{self._idl_to_java_type(p.type)} {p.name}" for p in cb.params)
        ret_type = self._idl_to_java_type(cb.return_type)
        
        lines = [
            "@FunctionalInterface",
            f"interface {cb.name} {{",
            f"    {ret_type} invoke({params});",
            "}",
            "",
        ]
        return lines

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

    def _is_callback_type(self, type_name: str) -> bool:
        """Check if type is a callback"""
        return any(cb.name == type_name for cb in self.idl.callbacks)

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

    def _is_struct_type(self, type_name: str) -> bool:
        """Check if a type is a struct defined in IDL"""
        return any(s.name == type_name for s in self.idl.structs)

    def _get_struct(self, type_name: str):
        """Get struct definition by name"""
        return next((s for s in self.idl.structs if s.name == type_name), None)

    def _get_callback(self, type_name: str):
        """Get callback definition by name"""
        return next((cb for cb in self.idl.callbacks if cb.name == type_name), None)

    def _generate_jni_callback_wrapper(self, param: Param, cb) -> list[str]:
        """Generate JNI code to wrap a Java callback into a C++ callback"""
        lines = []
        
        # Store the JNI env and callback object for use in the wrapper
        lines.append(f"    // Create wrapper for Java callback {param.name}")
        lines.append(f"    jobject g_{param.name} = env->NewGlobalRef({param.name});")
        lines.append(f"    jclass {param.name}Class = env->GetObjectClass({param.name});")
        
        # Build method signature
        jni_sig = self._build_callback_signature(cb)
        lines.append(f'    jmethodID {param.name}Method = env->GetMethodID({param.name}Class, "invoke", "{jni_sig}");')
        
        # Generate the C callback wrapper
        c_params = ", ".join(f"{TypeMapper.to_c(p.type)} {p.name}" for p in cb.params)
        c_ret = TypeMapper.to_c(cb.return_type)
        
        # Create a capturing lambda that calls the Java method
        # Note: For simplicity, we use a static variable approach
        lines.append(f"    static thread_local JNIEnv* s_env_{param.name} = nullptr;")
        lines.append(f"    static thread_local jobject s_callback_{param.name} = nullptr;")
        lines.append(f"    static thread_local jmethodID s_method_{param.name} = nullptr;")
        lines.append(f"    s_env_{param.name} = env;")
        lines.append(f"    s_callback_{param.name} = g_{param.name};")
        lines.append(f"    s_method_{param.name} = {param.name}Method;")
        
        # Create the wrapper lambda
        lines.append(f"    auto cpp_{param.name} = []({c_params}) -> {c_ret} {{")
        
        # Call the Java method
        call_args = ", ".join(p.name for p in cb.params)
        jni_call_method = self._get_jni_call_method(cb.return_type)
        
        if cb.return_type == 'void':
            lines.append(f"        s_env_{param.name}->{jni_call_method}(s_callback_{param.name}, s_method_{param.name}, {call_args});")
        else:
            lines.append(f"        return s_env_{param.name}->{jni_call_method}(s_callback_{param.name}, s_method_{param.name}, {call_args});")
        
        lines.append("    };")
        
        return lines

    def _build_callback_signature(self, cb) -> str:
        """Build JNI method signature for callback"""
        param_sigs = "".join(self._java_type_signature(p.type) for p in cb.params)
        ret_sig = self._java_type_signature(cb.return_type) if cb.return_type != 'void' else 'V'
        return f"({param_sigs}){ret_sig}"

    def _get_jni_call_method(self, return_type: str) -> str:
        """Get the JNI CallXxxMethod name for return type"""
        mapping = {
            'void': 'CallVoidMethod',
            'int': 'CallIntMethod',
            'bool': 'CallBooleanMethod',
            'float': 'CallFloatMethod',
            'double': 'CallDoubleMethod',
        }
        return mapping.get(return_type, 'CallIntMethod')

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
        
        # Determine null return value
        if TypeMapper.is_vector(method.return_type) or self._is_struct_type(method.return_type):
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
            elif self._is_callback_type(p.type):
                # Convert Java callback to C++ callback wrapper
                cb = self._get_callback(p.type)
                lines.extend(self._generate_jni_callback_wrapper(p, cb))
                cpp_arg_names.append(f"cpp_{p.name}")
            elif self._is_struct_type(p.type):
                # Convert Java object to C++ struct
                # Structs are defined at global scope in C API header (not in namespace)
                struct = self._get_struct(p.type)
                java_class_path = self.java_package.replace(".", "/") + "/" + p.type
                lines.append(f'    jclass {p.name}Class = env->GetObjectClass({p.name});')
                lines.append(f"    ::{p.type} cpp_{p.name};")  # Use global scope
                for m in struct.members:
                    field_id = f"{p.name}_{m.name}_fid"
                    jni_sig = self._java_type_signature(m.type)
                    getter = self._jni_field_getter(m.type)
                    lines.append(f'    jfieldID {field_id} = env->GetFieldID({p.name}Class, "{m.name}", "{jni_sig}");')
                    lines.append(f"    cpp_{p.name}.{m.name} = env->{getter}({p.name}, {field_id});")
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
        elif self._is_struct_type(method.return_type):
            # Return struct - convert C++ struct to Java object
            struct = self._get_struct(method.return_type)
            lines.append(f"    auto ret = obj->{method.name}({cpp_args});")
            # Release byte arrays
            for p in method.params:
                if p.type == "uint8_t" and p.is_pointer:
                    lines.append(f"    env->ReleaseByteArrayElements({p.name}, cpp_{p.name}_ptr, JNI_ABORT);")
            
            java_class_path = self.java_package.replace(".", "/") + "/" + method.return_type
            lines.append(f'    jclass retClass = env->FindClass("{java_class_path}");')
            
            # Build constructor signature
            sig_parts = []
            for m in struct.members:
                sig_parts.append(self._java_type_signature(m.type))
            sig = "(" + "".join(sig_parts) + ")V"
            
            lines.append(f'    jmethodID retCtor = env->GetMethodID(retClass, "<init>", "{sig}");')
            ctor_args = ", ".join(f"ret.{m.name}" for m in struct.members)
            lines.append(f"    return env->NewObject(retClass, retCtor, {ctor_args});")
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
        # Callbacks use their interface type
        if self._is_callback_type(param.type):
            return f"{param.type} {param.name}"
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
        if param.type == "double":
            return "jdouble"
        if param.type == "float":
            return "jfloat"
        if param.is_pointer and param.type == "uint8_t":
            return "jbyteArray"
        # Check if it's a callback type
        if self._is_callback_type(param.type):
            return "jobject"
        # Check if it's a struct type
        if any(s.name == param.type for s in self.idl.structs):
            return "jobject"
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
        if idl_type == "double":
            return "jdouble"
        if idl_type == "float":
            return "jfloat"
        # Check if it's a struct type
        if any(s.name == idl_type for s in self.idl.structs):
            return "jobject"
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

    def _jni_field_getter(self, idl_type: str) -> str:
        """Get JNI field getter method name for a type"""
        mapping = {
            "int": "GetIntField",
            "bool": "GetBooleanField",
            "float": "GetFloatField",
            "double": "GetDoubleField",
        }
        return mapping.get(idl_type, "GetIntField")
