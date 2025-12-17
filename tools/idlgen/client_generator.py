"""Client Generator - generates C++ wrapper for dynamic library loading"""

from .types import ParsedIDL, Interface, Method, Member, Param
from .type_mapper import TypeMapper


class ClientGenerator:
    """Generates C++ client wrapper for dynamic loading"""

    def __init__(self, idl: ParsedIDL, namespace: str):
        self.idl = idl
        self.namespace = namespace

    def generate_header(self) -> str:
        lines = [
            "// AUTO-GENERATED - DO NOT EDIT",
            "#pragma once",
            "",
            "#include <string>",
            "#include <vector>",
            "#include <memory>",
            "#include <functional>",
            f'#include "{self.namespace}_c_api.h"',
            "",
            f"namespace {self.namespace}_client {{",
            "",
            "bool initialize(const std::string& libraryPath);",
            "bool isInitialized();",
            "",
        ]

        for d in self.idl.structs:
            lines.append(f"using {d.name} = ::{d.name};")
        if self.idl.structs:
            lines.append("")

        for iface in self.idl.interfaces:
            lines.extend(self._interface_header(iface))

        lines.append(f"}} // namespace {self.namespace}_client")
        return "\n".join(lines)

    def generate_impl(self) -> str:
        lines = [
            "// AUTO-GENERATED - DO NOT EDIT",
            f'#include "{self.namespace}_client.hpp"',
            "",
            "#ifdef _WIN32",
            "#include <windows.h>",
            "#else",
            "#include <dlfcn.h>",
            "#endif",
            "",
            "#include <stdexcept>",
            "",
            f"namespace {self.namespace}_client {{",
            "",
            "namespace {",
            "",
            "void* g_library = nullptr;",
            "",
        ]

        for iface in self.idl.interfaces:
            lines.extend(self._fn_pointer_types(iface))

        lines.append("")

        for iface in self.idl.interfaces:
            lines.extend(self._fn_pointer_vars(iface))

        lines.extend([
            "",
            "void* loadSymbol(const char* name) {",
            "#ifdef _WIN32",
            "    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(g_library), name));",
            "#else",
            "    return dlsym(g_library, name);",
            "#endif",
            "}",
            "",
            "} // namespace",
            "",
        ])

        lines.extend(self._initialize_fn())

        for iface in self.idl.interfaces:
            lines.extend(self._interface_impl(iface))

        lines.append(f"}} // namespace {self.namespace}_client")
        return "\n".join(lines)

    def _interface_header(self, iface: Interface) -> list[str]:
        h = f"{iface.name}Handle"
        r = f"{iface.name}Result"
        client_result = "DetectionResult"
        lines = []

        # Result class
        vec_methods = [m for m in iface.methods if TypeMapper.is_vector(m.return_type)]
        if vec_methods:
            inner = TypeMapper.vector_inner(vec_methods[0].return_type)
            lines.extend([
                f"class {client_result} {{",
                "public:",
                f"    {client_result}();",
                f"    explicit {client_result}(::{r}* result);",
                f"    ~{client_result}() = default;",
                f"    {client_result}({client_result}&&) noexcept = default;",
                f"    {client_result}& operator=({client_result}&&) noexcept = default;",
                "",
                "    [[nodiscard]] int count() const;",
                f"    [[nodiscard]] const {inner}* data() const;",
                f"    [[nodiscard]] std::vector<{inner}> toVector() const;",
                "",
                "private:",
                f"    std::unique_ptr<::{r}, std::function<void(::{r}*)>> result_;",
                "};",
                "",
            ])

        # Main class
        lines.append(f"class {iface.name} {{")
        lines.append("public:")

        ctor = next((m for m in iface.methods if m.is_constructor), None)
        if ctor:
            cpp_params = ", ".join(self._param_to_cpp_decl(p) for p in ctor.params)
            lines.append(f"    explicit {iface.name}({cpp_params});")

        lines.extend([
            f"    ~{iface.name}() = default;",
            "",
            f"    {iface.name}(const {iface.name}&) = delete;",
            f"    {iface.name}& operator=(const {iface.name}&) = delete;",
            f"    {iface.name}({iface.name}&&) noexcept = default;",
            f"    {iface.name}& operator=({iface.name}&&) noexcept = default;",
            "",
        ])

        for member in iface.members:
            ret = TypeMapper.to_cpp(member.type)
            getter = self._getter_name(member)
            lines.append(f"    [[nodiscard]] {ret} {getter}() const noexcept;")

        for method in iface.methods:
            if method.is_constructor:
                continue
            ret = self._cpp_return_type(iface.name, method.return_type)
            params = ", ".join(self._param_to_cpp_decl(p) for p in method.params)
            const_q = " const" if method.is_const else ""
            lines.append(f"    [[nodiscard]] {ret} {method.name}({params}){const_q};")

        lines.extend([
            "",
            "private:",
            f"    std::unique_ptr<::{h}, std::function<void(::{h}*)>> handle_;",
            "};",
            "",
        ])

        return lines

    def _fn_pointer_types(self, iface: Interface) -> list[str]:
        h = f"{iface.name}Handle"
        r = f"{iface.name}Result"
        prefix = iface.name
        lines = []

        ctor = next((m for m in iface.methods if m.is_constructor), None)
        if ctor:
            c_params = ", ".join(self._param_to_c_type(p) for p in ctor.params) or "void"
            lines.append(f"using {prefix}CreateFn = {h}*(*)({c_params});")
            lines.append(f"using {prefix}DestroyFn = void(*)({h}*);")

        for method in iface.methods:
            if method.is_constructor:
                continue
            ret = self._c_return_type(method.return_type, r)
            params = [f"{h}*"] + [self._param_to_c_type(p) for p in method.params]
            fn_name = method.name[0].upper() + method.name[1:]
            lines.append(f"using {prefix}{fn_name}Fn = {ret}(*)({', '.join(params)});")

        vec_methods = [m for m in iface.methods if TypeMapper.is_vector(m.return_type)]
        if vec_methods:
            inner = TypeMapper.vector_inner(vec_methods[0].return_type)
            lines.append(f"using {prefix}GetResultCountFn = int(*)(const {r}*);")
            lines.append(f"using {prefix}GetResultDataFn = const {inner}*(*)(const {r}*);")
            lines.append(f"using {prefix}FreeResultFn = void(*)({r}*);")

        for member in iface.members:
            ret = "int" if member.type == "bool" else TypeMapper.to_c(member.type)
            getter = self._getter_name(member)
            fn_name = getter[0].upper() + getter[1:]
            lines.append(f"using {prefix}{fn_name}Fn = {ret}(*)({h}*);")

        return lines

    def _fn_pointer_vars(self, iface: Interface) -> list[str]:
        prefix = iface.name
        lines = []

        ctor = next((m for m in iface.methods if m.is_constructor), None)
        if ctor:
            lines.append(f"{prefix}CreateFn g_{prefix}_create = nullptr;")
            lines.append(f"{prefix}DestroyFn g_{prefix}_destroy = nullptr;")

        for method in iface.methods:
            if method.is_constructor:
                continue
            fn_name = method.name[0].upper() + method.name[1:]
            lines.append(f"{prefix}{fn_name}Fn g_{prefix}_{method.name} = nullptr;")

        vec_methods = [m for m in iface.methods if TypeMapper.is_vector(m.return_type)]
        if vec_methods:
            lines.append(f"{prefix}GetResultCountFn g_{prefix}_getResultCount = nullptr;")
            lines.append(f"{prefix}GetResultDataFn g_{prefix}_getResultData = nullptr;")
            lines.append(f"{prefix}FreeResultFn g_{prefix}_freeResult = nullptr;")

        for member in iface.members:
            getter = self._getter_name(member)
            fn_name = getter[0].upper() + getter[1:]
            lines.append(f"{prefix}{fn_name}Fn g_{prefix}_{getter} = nullptr;")

        return lines

    def _initialize_fn(self) -> list[str]:
        lines = [
            "bool initialize(const std::string& libraryPath) {",
            "    if (g_library) return true;",
            "",
            "#ifdef _WIN32",
            "    g_library = LoadLibraryA(libraryPath.c_str());",
            "#else",
            "    g_library = dlopen(libraryPath.c_str(), RTLD_NOW);",
            "#endif",
            "    if (!g_library) return false;",
            "",
        ]

        for iface in self.idl.interfaces:
            prefix = iface.name

            ctor = next((m for m in iface.methods if m.is_constructor), None)
            if ctor:
                lines.append(f'    g_{prefix}_create = reinterpret_cast<{prefix}CreateFn>(loadSymbol("{prefix}_create"));')
                lines.append(f'    g_{prefix}_destroy = reinterpret_cast<{prefix}DestroyFn>(loadSymbol("{prefix}_destroy"));')

            for method in iface.methods:
                if method.is_constructor:
                    continue
                fn_name = method.name[0].upper() + method.name[1:]
                lines.append(f'    g_{prefix}_{method.name} = reinterpret_cast<{prefix}{fn_name}Fn>(loadSymbol("{prefix}_{method.name}"));')

            vec_methods = [m for m in iface.methods if TypeMapper.is_vector(m.return_type)]
            if vec_methods:
                lines.append(f'    g_{prefix}_getResultCount = reinterpret_cast<{prefix}GetResultCountFn>(loadSymbol("{prefix}_getResultCount"));')
                lines.append(f'    g_{prefix}_getResultData = reinterpret_cast<{prefix}GetResultDataFn>(loadSymbol("{prefix}_getResultData"));')
                lines.append(f'    g_{prefix}_freeResult = reinterpret_cast<{prefix}FreeResultFn>(loadSymbol("{prefix}_freeResult"));')

            for member in iface.members:
                getter = self._getter_name(member)
                fn_name = getter[0].upper() + getter[1:]
                lines.append(f'    g_{prefix}_{getter} = reinterpret_cast<{prefix}{fn_name}Fn>(loadSymbol("{prefix}_{getter}"));')

        lines.extend([
            "",
            "    return true;",
            "}",
            "",
            "bool isInitialized() { return g_library != nullptr; }",
            "",
        ])

        return lines

    def _interface_impl(self, iface: Interface) -> list[str]:
        prefix = iface.name
        h = f"{iface.name}Handle"
        r = f"{iface.name}Result"
        client_result = "DetectionResult"
        lines = []

        # Result class impl
        vec_methods = [m for m in iface.methods if TypeMapper.is_vector(m.return_type)]
        if vec_methods:
            inner = TypeMapper.vector_inner(vec_methods[0].return_type)
            lines.extend([
                f"{client_result}::{client_result}() : result_(nullptr, nullptr) {{}}",
                "",
                f"{client_result}::{client_result}(::{r}* result)",
                f"    : result_(result, [](::{r}* r) {{ if (r && g_{prefix}_freeResult) g_{prefix}_freeResult(r); }}) {{}}",
                "",
                f"int {client_result}::count() const {{",
                f"    return result_ ? g_{prefix}_getResultCount(result_.get()) : 0;",
                "}",
                "",
                f"const {inner}* {client_result}::data() const {{",
                f"    return result_ ? g_{prefix}_getResultData(result_.get()) : nullptr;",
                "}",
                "",
                f"std::vector<{inner}> {client_result}::toVector() const {{",
                f"    std::vector<{inner}> vec;",
                "    int n = count();",
                "    auto* d = data();",
                "    if (n > 0 && d) vec.assign(d, d + n);",
                "    return vec;",
                "}",
                "",
            ])

        # Main class impl
        ctor = next((m for m in iface.methods if m.is_constructor), None)
        if ctor:
            cpp_params = ", ".join(self._param_to_cpp_decl(p) for p in ctor.params)
            c_args = ", ".join(self._to_c_arg(p) for p in ctor.params)

            lines.extend([
                f"{iface.name}::{iface.name}({cpp_params})",
                "    : handle_(nullptr, nullptr) {",
                '    if (!isInitialized()) throw std::runtime_error("Library not initialized");',
                f"    auto* h = g_{prefix}_create({c_args});",
                f"    handle_ = std::unique_ptr<::{h}, std::function<void(::{h}*)>>(h,",
                f"        [](::{h}* p) {{ if (p && g_{prefix}_destroy) g_{prefix}_destroy(p); }});",
                "}",
                "",
            ])

        for member in iface.members:
            ret = TypeMapper.to_cpp(member.type)
            getter = self._getter_name(member)
            default = "false" if member.type == "bool" else "0"
            lines.extend([
                f"{ret} {iface.name}::{getter}() const noexcept {{",
                f"    return handle_ && g_{prefix}_{getter} ? g_{prefix}_{getter}(handle_.get()) : {default};",
                "}",
                "",
            ])

        for method in iface.methods:
            if method.is_constructor:
                continue
            ret = self._cpp_return_type(iface.name, method.return_type)
            params = ", ".join(self._param_to_cpp_decl(p) for p in method.params)
            c_args = ", ".join(self._to_c_arg(p) for p in method.params)
            const_q = " const" if method.is_const else ""

            lines.append(f"{ret} {iface.name}::{method.name}({params}){const_q} {{")
            lines.append(f"    if (!handle_) return {ret}();")
            if c_args:
                lines.append(f"    return {ret}(g_{prefix}_{method.name}(handle_.get(), {c_args}));")
            else:
                lines.append(f"    return {ret}(g_{prefix}_{method.name}(handle_.get()));")
            lines.append("}")
            lines.append("")

        return lines

    def _param_to_cpp_decl(self, param: Param) -> str:
        """Convert param to C++ declaration for method signature"""
        base_type = TypeMapper.to_cpp(param.type)
        
        if param.is_const:
            base_type = f'const {base_type}'
        
        if param.is_pointer:
            return f'{base_type}* {param.name}'
        elif param.is_reference:
            return f'{base_type}& {param.name}'
        else:
            # For strings and complex types, use const ref
            if param.type == 'string':
                return f'const std::string& {param.name}'
            return f'{base_type} {param.name}'

    def _param_to_c_type(self, param: Param) -> str:
        """Convert param type to C type (without name)"""
        if param.type == 'string':
            return 'const char*'
        
        base = TypeMapper.to_c(param.type)
        if param.is_const:
            base = f'const {base}'
        if param.is_pointer:
            return f'{base}*'
        return base

    def _to_c_arg(self, param: Param) -> str:
        if TypeMapper.is_string(param.type):
            return f"{param.name}.c_str()"
        return param.name

    def _getter_name(self, member: Member) -> str:
        prefix = "is" if member.type == "bool" else "get"
        return f"{prefix}{member.name[0].upper()}{member.name[1:]}"

    def _cpp_return_type(self, iface_name: str, idl_type: str) -> str:
        if TypeMapper.is_vector(idl_type):
            return "DetectionResult"
        return TypeMapper.to_cpp(idl_type)

    def _c_return_type(self, idl_type: str, result_type: str) -> str:
        if TypeMapper.is_vector(idl_type):
            return f"{result_type}*"
        return TypeMapper.to_c(idl_type)
