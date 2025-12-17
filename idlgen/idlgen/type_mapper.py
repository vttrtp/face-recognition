"""Type mapping from C++-like IDL types to C/C++ types"""

import re
from typing import Optional
from .types import Param


class TypeMapper:
    """Maps C++-like IDL types to C and C++ types"""

    # Direct C++ type mappings
    CPP_TYPES = {
        'void': 'void',
        'bool': 'bool',
        'int': 'int',
        'uint8_t': 'uint8_t',
        'int8_t': 'int8_t',
        'int16_t': 'int16_t',
        'uint16_t': 'uint16_t',
        'int32_t': 'int32_t',
        'uint32_t': 'uint32_t',
        'int64_t': 'int64_t',
        'uint64_t': 'uint64_t',
        'float': 'float',
        'double': 'double',
        'char': 'char',
        'string': 'std::string',
    }

    # C type mappings (for C API)
    C_TYPES = {
        'void': 'void',
        'bool': 'int',  # C89 doesn't have bool
        'int': 'int',
        'uint8_t': 'uint8_t',
        'int8_t': 'int8_t',
        'int16_t': 'int16_t',
        'uint16_t': 'uint16_t',
        'int32_t': 'int32_t',
        'uint32_t': 'uint32_t',
        'int64_t': 'int64_t',
        'uint64_t': 'uint64_t',
        'float': 'float',
        'double': 'double',
        'char': 'char',
        'string': 'const char*',
    }

    @classmethod
    def to_cpp(cls, idl_type: str) -> str:
        """Convert IDL type to C++ type"""
        # Handle vector<T>
        if m := re.match(r'vector<(.+)>', idl_type):
            inner = m.group(1)
            return f'std::vector<{cls.to_cpp(inner)}>'
        
        return cls.CPP_TYPES.get(idl_type, idl_type)

    @classmethod
    def to_c(cls, idl_type: str) -> str:
        """Convert IDL type to C type"""
        # Handle vector<T> - returns pointer to first element
        if m := re.match(r'vector<(.+)>', idl_type):
            inner = m.group(1)
            return f'{cls.to_c(inner)}*'
        
        return cls.C_TYPES.get(idl_type, idl_type)

    @classmethod
    def to_c_param(cls, idl_type: str) -> str:
        """Convert parameter type for C API"""
        if idl_type == 'string':
            return 'const char*'
        return cls.to_c(idl_type)

    @classmethod
    def param_to_cpp(cls, param: Param) -> str:
        """Convert parameter to C++ declaration"""
        base_type = cls.to_cpp(param.type)
        
        if param.is_const:
            base_type = f'const {base_type}'
        
        if param.is_pointer:
            return f'{base_type}* {param.name}'
        elif param.is_reference:
            return f'{base_type}& {param.name}'
        else:
            return f'{base_type} {param.name}'

    @classmethod
    def param_to_c(cls, param: Param) -> str:
        """Convert parameter to C declaration"""
        base_type = cls.to_c(param.type)
        
        # For string references/values, C uses const char*
        if param.type == 'string':
            return f'const char* {param.name}'
        
        if param.is_const:
            base_type = f'const {base_type}'
        
        if param.is_pointer:
            return f'{base_type}* {param.name}'
        elif param.is_reference:
            # References become pointers in C, but const refs of primitives are just values
            if param.is_const and param.type in ['int', 'bool', 'float', 'double']:
                return f'{param.type} {param.name}'
            return f'{base_type}* {param.name}'
        else:
            return f'{base_type} {param.name}'

    @classmethod
    def is_string(cls, idl_type: str) -> bool:
        """Check if type is a string"""
        return idl_type == 'string'

    @classmethod
    def is_vector(cls, idl_type: str) -> bool:
        """Check if type is a vector"""
        return idl_type.startswith('vector<') and idl_type.endswith('>')

    @classmethod
    def vector_inner(cls, idl_type: str) -> Optional[str]:
        """Get inner type of vector<T>"""
        if m := re.match(r'vector<(.+)>', idl_type):
            return m.group(1)
        return None

    @classmethod
    def is_primitive(cls, idl_type: str) -> bool:
        """Check if type is a primitive (not struct/class)"""
        return idl_type in cls.CPP_TYPES or cls.is_vector(idl_type)

    @classmethod
    def is_callback(cls, idl_type: str, callbacks: list) -> bool:
        """Check if type is a callback type"""
        return any(cb.name == idl_type for cb in callbacks)
