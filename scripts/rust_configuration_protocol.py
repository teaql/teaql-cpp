#!/usr/bin/env python3
"""Versioned, generation-time observations from the pinned Rust MIR format.

This prototype reads compiler facts. It does not interpret Rust configuration
expressions or add a dependency to generated C++ libraries.
"""

import json
import re

SCHEMA = "teaql.rust_configuration.projections.v1"
RUNTIME = "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3"
GENERATOR = "45e1572c4956ca963410d8ea436ead3c8857087a"
MIR_PREFIX = (
    "// WARNING: This output format is intended for human consumers only\n"
    "// and is subject to change without notice. Knock yourself out.\n"
    "// HINT: See also -Z dump-mir for MIR at specific points during compilation.\n"
)
CALL_MARKER = "EntityRuntimeState::resolve_relation_"
CALL = re.compile(
    r"\s*(_\d+) = EntityRuntimeState::resolve_relation_(list|option)"
    r"::<(.+)>\((.+)\) -> \[return: bb\d+, unwind(?:: bb\d+| continue| unreachable| terminate\([^)]*\))\];"
)
FUNCTION = re.compile(
    r"fn (.+?)::<impl at (.+\.rs):\d+:\d+: \d+:\d+>::([^ (]+)\(.*"
)
BLOCK = re.compile(r"\s*bb(\d+)(?: \(cleanup\))?: \{")
ASSIGNMENT = re.compile(r"\s*(_\d+) = (.+);")
STRING = re.compile(r'const ("(?:[^"\\]|\\.)*")')
LOCAL = re.compile(r"(?:move|copy) (_\d+)")


class ProtocolError(RuntimeError):
    """The compiler output cannot establish the requested source fact."""


def require(condition, message):
    if not condition:
        raise ProtocolError(message)


def verify_toolchain(actual, expected):
    require(isinstance(actual, dict) and isinstance(expected, dict), "toolchain objects required")
    require(actual == expected, "unqualified Rust compiler, target, or sysroot identity")
    require(actual.get("host") == actual.get("target") == "x86_64-unknown-linux-gnu",
            "this protocol has only been qualified for the native target")


def projection_facts(mir, entity_sources):
    """Collect every borrowed-JSON projection and inventory the other calls.

    entity_sources is supplied by trusted generation assembly, keyed by the
    actual generated Rust module. Each value binds its original source path,
    canonical entity name, and independently known relation getter methods.
    Constant operands must be defined in the same
    basic block and remain unmodified at their use. An unfamiliar shape fails
    explicitly instead of producing a partial list of supposedly resolved types.
    """
    require(type(mir) is str and mir.startswith(MIR_PREFIX), "unknown MIR format header")
    require(isinstance(entity_sources, dict) and entity_sources, "generated entity inventory required")
    for module, item in entity_sources.items():
        require(type(module) is str and isinstance(item, dict)
                and set(item) == {"path", "owner", "getter_methods"}
                and all(type(item[key]) is str and item[key] for key in ("path", "owner"))
                and type(item["getter_methods"]) is list
                and all(type(method) is str and method for method in item["getter_methods"])
                and len(item["getter_methods"]) == len(set(item["getter_methods"])),
                "complete generated entity source identity required")
    facts, other_calls, seen, getters_seen = [], [], set(), set()
    function, block, constants = "", None, {}
    call_count = 0
    for number, line in enumerate(mir.splitlines(), 1):
        if line.startswith("fn "):
            function, block, constants = line, None, {}
        if line == "}":
            function, block, constants = "", None, {}
        found = BLOCK.fullmatch(line)
        if found:
            block, constants = int(found[1]), {}
        if CALL_MARKER in line:
            call_count += 1
            call = CALL.fullmatch(line)
            require(call is not None and block is not None, "unrecognized relation call syntax")
            source = FUNCTION.fullmatch(function)
            require(source is not None and source[1] in entity_sources,
                    "relation call outside a known generated entity function")
            identity = entity_sources[source[1]]
            require(source[2] == identity["path"], "relation call source path mismatch")
            method = source[3]
            if method != "borrowed_json::{closure#0}":
                getter = re.fullmatch(r"([^:{]+)::\{closure#0\}", method)
                require(getter is not None and getter[1] in identity["getter_methods"]
                        and call[2] == "option",
                        "unknown non-projection relation consumer")
                getter_key = (source[1], getter[1])
                require(getter_key not in getters_seen, "duplicate direct relation getter call")
                getters_seen.add(getter_key)
                other_calls.append(dict(line=number, function=function, call=line.strip(),
                                        source_module=source[1], getter_method=getter[1]))
                continue
            arguments = call[4].split(", ")
            require(len(arguments) == 4, "relation projection must have four operands")
            operands = []
            for argument in (arguments[1], arguments[3]):
                local = LOCAL.fullmatch(argument)
                require(local is not None and local[1] in constants,
                        "projection identity lacks an unmodified same-block constant")
                operands.append(constants[local[1]])
            (owner, owner_line), (relation, relation_line) = operands
            require(owner == identity["owner"], "projection owner differs from its generated entity")
            require(relation != "", "empty projection relation name")
            key = (owner, relation)
            require(key not in seen, "duplicate projection owner/relation identity")
            seen.add(key)
            facts.append(dict(owner=owner, relation=relation, cardinality=call[2],
                              resolved_type=call[3], source_module=source[1],
                              source_path=source[2], function=function, block=block,
                              line=number, owner_constant_line=owner_line,
                              relation_constant_line=relation_line, call=line.strip()))
        assignment = ASSIGNMENT.fullmatch(line)
        if assignment:
            constants.pop(assignment[1], None)
            value = STRING.fullmatch(assignment[2])
            if value:
                try:
                    decoded = json.loads(value[1])
                except (ValueError, TypeError) as error:
                    raise ProtocolError("unrecognized MIR string escape") from error
                constants[assignment[1]] = (decoded, number)
        dead = re.fullmatch(r"\s*StorageDead\((_\d+)\);", line)
        if dead:
            constants.pop(dead[1], None)
    require(call_count == len(facts) + len(other_calls), "incomplete relation-call inventory")
    require(getters_seen == {(module, method) for module, identity in entity_sources.items()
                             for method in identity["getter_methods"]},
            "incomplete direct relation getter inventory")
    require(facts, "no compiler-resolved JSON projections")
    return dict(schema=SCHEMA, runtime_commit=RUNTIME, generator_commit=GENERATOR,
                projections=sorted(facts, key=lambda item: (item["owner"], item["relation"])),
                other_relation_calls=other_calls, relation_call_count=call_count)


def verify_membership(protocol, expected, absent):
    """Check an independent fixture contract against the complete compiler facts."""
    actual = [(item["owner"], item["relation"], item["cardinality"], item["resolved_type"])
              for item in protocol["projections"]]
    require(len(actual) == len(set(actual)) and set(actual) == set(map(tuple, expected)),
            "compiler projection membership, cardinality, or resolved type differs")
    keys = {(item[0], item[1]) for item in actual}
    require(all(tuple(key) not in keys for key in absent),
            "an omitted or ignored projection acquired a compiler use")
