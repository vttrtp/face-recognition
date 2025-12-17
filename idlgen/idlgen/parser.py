"""C++-like IDL parser"""

import re
from .types import Param, Member, Method, Interface, Struct, Callback, ParsedIDL


class IDLParser:
    """Parses C++-like IDL syntax"""

    def __init__(self, content: str):
        self.content = self._strip_comments(content)

    def _strip_comments(self, content: str) -> str:
        content = re.sub(r'//.*$', '', content, flags=re.MULTILINE)
        content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
        return content

    def parse(self) -> ParsedIDL:
        result = ParsedIDL()
        result.structs = self._parse_structs()
        result.callbacks = self._parse_callbacks()
        result.interfaces = self._parse_interfaces()
        return result

    def _parse_structs(self) -> list[Struct]:
        structs = []
        pattern = r'struct\s+(\w+)\s*\{([^}]*)\}'
        for match in re.finditer(pattern, self.content):
            name, body = match.groups()
            members = []
            for m in re.finditer(r'(\w+)\s+(\w+)\s*;', body):
                members.append(Member(name=m.group(2), type=m.group(1)))
            structs.append(Struct(name=name, members=members))
        return structs

    def _parse_callbacks(self) -> list[Callback]:
        """Parse callback declarations like: callback ProgressCallback(int current, int total) -> void;"""
        callbacks = []
        # Match: callback Name(params) -> returnType;
        pattern = r'callback\s+(\w+)\s*\(([^)]*)\)\s*->\s*(\w+)\s*;'
        for match in re.finditer(pattern, self.content):
            name = match.group(1)
            params_str = match.group(2)
            return_type = match.group(3)
            params = self._parse_params(params_str)
            callbacks.append(Callback(name=name, return_type=return_type, params=params))
        return callbacks

    def _parse_interfaces(self) -> list[Interface]:
        interfaces = []
        pattern = r'interface\s+(\w+)\s*\{([^}]*)\}'
        for match in re.finditer(pattern, self.content):
            name, body = match.groups()
            iface = Interface(name=name)
            self._parse_interface_body(body, iface)
            interfaces.append(iface)
        return interfaces

    def _parse_interface_body(self, body: str, iface: Interface):
        for line in body.strip().split(';'):
            line = line.strip()
            if not line:
                continue

            # Check for constructor: ClassName(params)
            if m := re.match(rf'{iface.name}\s*\(([^)]*)\)', line):
                params = self._parse_params(m.group(1))
                iface.methods.append(Method(
                    name="constructor",
                    return_type="void",
                    params=params,
                    is_constructor=True
                ))
            # Check for method: type name(params) [const]
            elif m := re.match(r'(.+?)\s+(\w+)\s*\(([^)]*)\)\s*(const)?', line):
                return_type = m.group(1).strip()
                method_name = m.group(2)
                params = self._parse_params(m.group(3))
                is_const = m.group(4) is not None
                iface.methods.append(Method(
                    name=method_name,
                    return_type=return_type,
                    params=params,
                    is_const=is_const
                ))

    def _parse_params(self, params_str: str) -> list[Param]:
        params = []
        if not params_str.strip():
            return params
            
        for p in params_str.split(','):
            p = p.strip()
            if not p:
                continue
            
            is_const = False
            is_pointer = False
            is_reference = False
            
            # Check for const
            if p.startswith('const '):
                is_const = True
                p = p[6:].strip()
            
            # Check for pointer or reference
            if '*' in p:
                is_pointer = True
                p = p.replace('*', ' ').strip()
            elif '&' in p:
                is_reference = True
                p = p.replace('&', ' ').strip()
            
            # Split type and name
            parts = p.rsplit(None, 1)
            if len(parts) == 2:
                param_type, param_name = parts
                params.append(Param(
                    type=param_type.strip(),
                    name=param_name.strip(),
                    is_const=is_const,
                    is_pointer=is_pointer,
                    is_reference=is_reference
                ))
        
        return params
