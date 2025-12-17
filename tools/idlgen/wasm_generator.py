"""WASM Generator - generates Emscripten bindings for WebAssembly"""

from .types import ParsedIDL, Interface, Method, Member, Param
from .type_mapper import TypeMapper


class WASMGenerator:
    """Generates Emscripten bindings"""

    def __init__(self, idl: ParsedIDL, namespace: str):
        self.idl = idl
        self.namespace = namespace

    def generate(self, impl_header: str) -> str:
        lines = [
            "// AUTO-GENERATED - DO NOT EDIT",
            "#include <emscripten/bind.h>",
            "#include <emscripten/val.h>",
            f'#include "{impl_header}"',
            "#include <vector>",
            "#include <memory>",
            "#include <string>",
            "#include <cstdint>",
            "",
            "using namespace emscripten;",
            "",
        ]

        for iface in self.idl.interfaces:
            lines.extend(self._interface_wrapper(iface))
            lines.extend(self._interface_bindings(iface))

        return "\n".join(lines)

    def _interface_wrapper(self, iface: Interface) -> list[str]:
        cpp_class = f"{self.namespace}::{iface.name}"
        wasm_class = f"Wasm{iface.name}"
        lines = [
            f"class {wasm_class} {{",
            "public:",
            f"    {wasm_class}() = default;",
            "",
        ]

        # Constructor
        ctor = next((m for m in iface.methods if m.is_constructor), None)
        if ctor:
            lines.extend(self._wasm_constructor(iface, ctor, cpp_class))

        # Attribute getters
        for member in iface.members:
            lines.extend(self._wasm_attribute(member))

        # Methods
        for method in iface.methods:
            if method.is_constructor:
                continue
            lines.extend(self._wasm_method(iface, method))

        lines.extend([
            "private:",
            f"    std::unique_ptr<{cpp_class}> impl_;",
            "};",
            "",
        ])

        return lines

    def _wasm_constructor(self, iface: Interface, ctor: Method, cpp_class: str) -> list[str]:
        params = ", ".join(f"{self._wasm_param_type(p)} {p.name}" for p in ctor.params)
        args = ", ".join(p.name for p in ctor.params)
        
        lines = [
            f"    bool create({params}) {{",
            "        try {",
            f"            impl_ = std::make_unique<{cpp_class}>({args});",
            "            return impl_ != nullptr;",
            "        } catch (...) {",
            "            return false;",
            "        }",
            "    }",
            "",
        ]
        return lines

    def _wasm_attribute(self, member: Member) -> list[str]:
        ret = self._wasm_return_type(member.type)
        if member.type == "bool":
            getter = f"is{member.name[0].upper()}{member.name[1:]}"
            default = "false"
        else:
            getter = f"get{member.name[0].upper()}{member.name[1:]}"
            default = self._wasm_default(member.type)
        
        return [
            f"    {ret} {getter}() const {{",
            f"        return impl_ ? impl_->{getter}() : {default};",
            "    }",
            "",
        ]

    def _wasm_method(self, iface: Interface, method: Method) -> list[str]:
        ret = self._wasm_return_type(method.return_type)
        params = ", ".join(f"{self._wasm_param_type(p)} {p.name}" for p in method.params)
        
        # Check if we have uint8_t* parameter that needs special handling
        has_uint8_ptr = any(p.type == "uint8_t" and p.is_pointer for p in method.params)
        
        # Build argument conversion
        args = []
        for p in method.params:
            if p.type == "uint8_t" and p.is_pointer:
                args.append(f"{p.name}Vec.data()")
            else:
                args.append(p.name)
        args_str = ", ".join(args)

        lines = [f"    {ret} {method.name}({params}) {{"]
        
        # Add vector conversion for uint8_t* parameters
        if has_uint8_ptr:
            for p in method.params:
                if p.type == "uint8_t" and p.is_pointer:
                    lines.append(f"        auto {p.name}Vec = vecFromJSArray<uint8_t>({p.name});")
        
        if TypeMapper.is_vector(method.return_type):
            inner = TypeMapper.vector_inner(method.return_type)
            struct_def = next((d for d in self.idl.structs if d.name == inner), None)
            
            lines.append("        val result = val::array();")
            lines.append("        if (!impl_) return result;")
            lines.append(f"        auto items = impl_->{method.name}({args_str});")
            lines.append("        for (const auto& item : items) {")
            
            if struct_def:
                lines.append("            val obj = val::object();")
                for m in struct_def.members:
                    lines.append(f'            obj.set("{m.name}", item.{m.name});')
                lines.append('            result.call<void>("push", obj);')
            else:
                lines.append('            result.call<void>("push", item);')
            
            lines.append("        }")
            lines.append("        return result;")
        else:
            default = self._wasm_default(method.return_type)
            lines.append(f"        if (!impl_) return {default};")
            lines.append(f"        return impl_->{method.name}({args_str});")
        
        lines.append("    }")
        lines.append("")
        return lines

    def _wasm_param_type(self, param: Param) -> str:
        """Convert param to WASM-compatible type"""
        if param.type == "uint8_t" and param.is_pointer:
            return "val"
        if param.type == "string":
            return "const std::string&"
        if param.type == "int":
            return "int"
        if param.type == "bool":
            return "bool"
        return TypeMapper.to_cpp(param.type)

    def _wasm_return_type(self, idl_type: str) -> str:
        if TypeMapper.is_vector(idl_type):
            return "val"
        if idl_type == "bool":
            return "bool"
        if idl_type == "int":
            return "int"
        if idl_type == "string":
            return "std::string"
        return TypeMapper.to_cpp(idl_type)

    def _wasm_default(self, idl_type: str) -> str:
        if idl_type == "bool":
            return "false"
        if idl_type in ("int", "float", "double", "uint8_t"):
            return "0"
        if idl_type == "string":
            return '""'
        if TypeMapper.is_vector(idl_type):
            return "val::array()"
        return "{}"

    def _interface_bindings(self, iface: Interface) -> list[str]:
        wasm_class = f"Wasm{iface.name}"
        lines = [
            f"EMSCRIPTEN_BINDINGS({self.namespace}_{iface.name.lower()}) {{",
            f'    class_<{wasm_class}>("{iface.name}")',
            "        .constructor<>()",
        ]

        ctor = next((m for m in iface.methods if m.is_constructor), None)
        if ctor:
            lines.append(f'        .function("create", &{wasm_class}::create)')

        for member in iface.members:
            if member.type == "bool":
                getter = f"is{member.name[0].upper()}{member.name[1:]}"
            else:
                getter = f"get{member.name[0].upper()}{member.name[1:]}"
            lines.append(f'        .function("{getter}", &{wasm_class}::{getter})')

        for method in iface.methods:
            if method.is_constructor:
                continue
            lines.append(f'        .function("{method.name}", &{wasm_class}::{method.name})')

        lines.append("    ;")
        lines.append("}")
        lines.append("")

        return lines
