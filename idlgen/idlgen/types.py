"""Data types for IDL parsing"""

from dataclasses import dataclass, field


@dataclass
class Param:
    """Method parameter"""
    type: str
    name: str
    is_const: bool = False
    is_pointer: bool = False
    is_reference: bool = False


@dataclass
class Member:
    """Interface or struct member"""
    name: str
    type: str
    is_const: bool = False


@dataclass
class Method:
    """Interface method"""
    name: str
    return_type: str
    params: list[Param] = field(default_factory=list)
    is_constructor: bool = False
    is_const: bool = False


@dataclass
class Callback:
    """Callback function type definition"""
    name: str
    return_type: str
    params: list[Param] = field(default_factory=list)


@dataclass
class Interface:
    """IDL interface definition"""
    name: str
    members: list[Member] = field(default_factory=list)
    methods: list[Method] = field(default_factory=list)


@dataclass
class Struct:
    """IDL struct definition"""
    name: str
    members: list[Member] = field(default_factory=list)


@dataclass
class ParsedIDL:
    """Complete parsed IDL result"""
    structs: list[Struct] = field(default_factory=list)
    interfaces: list[Interface] = field(default_factory=list)
    callbacks: list[Callback] = field(default_factory=list)
