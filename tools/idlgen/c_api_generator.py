"""C API Generator - generates C header and implementation for shared library export"""

from pathlib import Path
from .types import ParsedIDL, Interface, Method, Member, Param
from .type_mapper import TypeMapper


class CAPIGenerator:
    """Generates C API header and implementation"""

    def __init__(self, idl: ParsedIDL, namespace: str):
        self.idl = idl
        self.namespace = namespace
        self.api_macro = f"{namespace.upper()}_API"
        self.export_macro = f"{namespace.upper()}_EXPORTS"

    def generate_header(self) -> str:
        lines = self._header_preamble()
        lines.extend(self._generate_structs())
        lines.extend(self._generate_interface_decls())
        lines.extend(self._header_postamble())
        return "\n".join(lines)

    def generate_impl(self, impl_header: str) -> str:
        lines = [
            "// AUTO-GENERATED - DO NOT EDIT",
            f'#include "{Path(impl_header).name}"',
            f'#include "{self.namespace}_c_api.h"',
            "",
        ]
        for iface in self.idl.interfaces:
            lines.extend(self._generate_interface_impl(iface))
        return "\n".join(lines)

    def _header_preamble(self) -> list[str]:
        guard = f"{self.namespace.upper()}_C_API_H"
        return [
            "// AUTO-GENERATED - DO NOT EDIT",
            f"#ifndef {guard}",
            f"#define {guard}",
            "",
            "#include <stdint.h>",
            "",
            "#ifdef __cplusplus",
            'extern "C" {',
            "#endif",
            "",
            "#ifdef _WIN32",
            f"    #ifdef {self.export_macro}",
            f"        #define {self.api_macro} __declspec(dllexport)",
            "    #else",
            f"        #define {self.api_macro} __declspec(dllimport)",
            "    #endif",
            "#else",
            f'    #define {self.api_macro} __attribute__((visibility("default")))',
            "#endif",
            "",
        ]

    def _header_postamble(self) -> list[str]:
        return [
            "#ifdef __cplusplus",
            "}",
            "#endif",
            "",
            f"#endif // {self.namespace.upper()}_C_API_H",
        ]

    def _generate_structs(self) -> list[str]:
        lines = []
        for d in self.idl.structs:
            lines.append(f"typedef struct {d.name} {{")
            for m in d.members:
                lines.append(f"    {TypeMapper.to_c(m.type)} {m.name};")
            lines.append(f"}} {d.name};")
            lines.append("")
        return lines

    def _generate_interface_decls(self) -> list[str]:
        lines = []
        for iface in self.idl.interfaces:
            handle = f"{iface.name}Handle"
            result = f"{iface.name}Result"

            lines.append(f"typedef struct {handle} {handle};")
            lines.append(f"typedef struct {result} {result};")
            lines.append("")

            for method in iface.methods:
                lines.extend(self._method_decl(iface, method))

            # Result accessors (once per interface if any method returns vector)
            vec_methods = [m for m in iface.methods if TypeMapper.is_vector(m.return_type)]
            if vec_methods:
                inner = TypeMapper.vector_inner(vec_methods[0].return_type)
                lines.append(f"{self.api_macro} int {iface.name}_getResultCount(const {result}* result);")
                lines.append(f"{self.api_macro} const {inner}* {iface.name}_getResultData(const {result}* result);")
                lines.append(f"{self.api_macro} void {iface.name}_freeResult({result}* result);")

            for member in iface.members:
                lines.append(self._attr_getter_decl(iface, member))

            lines.append("")
        return lines

    def _method_decl(self, iface: Interface, method: Method) -> list[str]:
        h = f"{iface.name}Handle"
        r = f"{iface.name}Result"
        prefix = iface.name
        lines = []

        if method.is_constructor:
            c_params = self._c_params_str(method.params)
            lines.append(f"{self.api_macro} {h}* {prefix}_create({c_params});")
            lines.append(f"{self.api_macro} void {prefix}_destroy({h}* handle);")
        else:
            ret = self._c_return_type(method.return_type, r)
            params = [f"{h}* handle"] + [self._param_to_c(p) for p in method.params]
            lines.append(f"{self.api_macro} {ret} {prefix}_{method.name}({', '.join(params)});")

        return lines

    def _attr_getter_decl(self, iface: Interface, member: Member) -> str:
        h = f"{iface.name}Handle"
        ret = "int" if member.type == "bool" else TypeMapper.to_c(member.type)
        getter = self._getter_name(member)
        return f"{self.api_macro} {ret} {iface.name}_{getter}({h}* handle);"

    def _generate_interface_impl(self, iface: Interface) -> list[str]:
        h = f"{iface.name}Handle"
        r = f"{iface.name}Result"
        cpp_class = f"{self.namespace}::{iface.name}"
        lines = []

        # Handle struct
        lines.append(f"struct {h} {{")
        lines.append(f"    std::unique_ptr<{cpp_class}> impl;")
        lines.append("};")
        lines.append("")

        # Result struct (if needed)
        vec_types = [TypeMapper.vector_inner(m.return_type) for m in iface.methods if TypeMapper.is_vector(m.return_type)]
        if vec_types:
            inner = TypeMapper.to_cpp(vec_types[0])
            lines.append(f"struct {r} {{")
            lines.append(f"    std::vector<{inner}> data;")
            lines.append("};")
            lines.append("")

        lines.append('extern "C" {')
        lines.append("")

        for method in iface.methods:
            lines.extend(self._method_impl(iface, method, cpp_class))

        # Result accessors (once per interface if any method returns vector)
        vec_methods = [m for m in iface.methods if TypeMapper.is_vector(m.return_type)]
        if vec_methods:
            inner = TypeMapper.vector_inner(vec_methods[0].return_type)
            lines.extend([
                f"int {iface.name}_getResultCount(const {r}* result) {{",
                "    return result ? static_cast<int>(result->data.size()) : -1;",
                "}",
                "",
                f"const {inner}* {iface.name}_getResultData(const {r}* result) {{",
                "    return (result && !result->data.empty()) ? result->data.data() : nullptr;",
                "}",
                "",
                f"void {iface.name}_freeResult({r}* result) {{",
                "    delete result;",
                "}",
                "",
            ])

        for member in iface.members:
            lines.extend(self._attr_getter_impl(iface, member))

        lines.append("} // extern \"C\"")
        lines.append("")
        return lines

    def _method_impl(self, iface: Interface, method: Method, cpp_class: str) -> list[str]:
        h = f"{iface.name}Handle"
        r = f"{iface.name}Result"
        prefix = iface.name
        lines = []

        if method.is_constructor:
            c_params = self._c_params_str(method.params)
            lines.append(f"{h}* {prefix}_create({c_params}) {{")

            for p in method.params:
                if TypeMapper.is_string(p.type):
                    lines.append(f"    if (!{p.name}) return nullptr;")

            lines.append("    try {")
            lines.append(f"        auto handle = new {h}();")
            cpp_args = ", ".join(p.name for p in method.params)
            lines.append(f"        handle->impl = std::make_unique<{cpp_class}>({cpp_args});")
            lines.append("        return handle;")
            lines.append("    } catch (...) { return nullptr; }")
            lines.append("}")
            lines.append("")

            lines.append(f"void {prefix}_destroy({h}* handle) {{")
            lines.append("    delete handle;")
            lines.append("}")
            lines.append("")
        else:
            ret = self._c_return_type(method.return_type, r)
            params = [f"{h}* handle"] + [self._param_to_c(p) for p in method.params]

            lines.append(f"{ret} {prefix}_{method.name}({', '.join(params)}) {{")

            null_checks = ["!handle", "!handle->impl"]
            for p in method.params:
                if TypeMapper.is_string(p.type):
                    null_checks.append(f"!{p.name}")

            null_ret = "nullptr" if ret.endswith("*") else ("-1" if ret == "int" else "0")
            lines.append(f"    if ({' || '.join(null_checks)}) return {null_ret};")

            cpp_args = ", ".join(p.name for p in method.params)
            if TypeMapper.is_vector(method.return_type):
                lines.append(f"    auto result = new {r}();")
                lines.append(f"    result->data = handle->impl->{method.name}({cpp_args});")
                lines.append("    return result;")
            else:
                lines.append(f"    return handle->impl->{method.name}({cpp_args});")

            lines.append("}")
            lines.append("")

        return lines

    def _attr_getter_impl(self, iface: Interface, member: Member) -> list[str]:
        h = f"{iface.name}Handle"
        ret = "int" if member.type == "bool" else TypeMapper.to_c(member.type)
        getter = self._getter_name(member)
        cpp_getter = f"is{member.name[0].upper()}{member.name[1:]}" if member.type == "bool" else f"get{member.name[0].upper()}{member.name[1:]}"

        return [
            f"{ret} {iface.name}_{getter}({h}* handle) {{",
            f"    return (handle && handle->impl) ? handle->impl->{cpp_getter}() : 0;",
            "}",
            "",
        ]

    def _param_to_c(self, param: Param) -> str:
        """Convert param to C declaration"""
        if param.type == 'string':
            return f'const char* {param.name}'
        
        base = TypeMapper.to_c(param.type)
        if param.is_const:
            base = f'const {base}'
        if param.is_pointer:
            return f'{base}* {param.name}'
        return f'{base} {param.name}'

    def _c_params_str(self, params: list[Param]) -> str:
        return ", ".join(self._param_to_c(p) for p in params) or "void"

    def _c_return_type(self, idl_type: str, result_type: str) -> str:
        if TypeMapper.is_vector(idl_type):
            return f"{result_type}*"
        if idl_type == "void":
            return "void"
        if idl_type == "bool":
            return "int"
        return TypeMapper.to_c(idl_type)

    def _getter_name(self, member: Member) -> str:
        prefix = "is" if member.type == "bool" else "get"
        return f"{prefix}{member.name[0].upper()}{member.name[1:]}"
