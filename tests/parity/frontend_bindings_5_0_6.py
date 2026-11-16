"""Explicit test-registration adaptations for header and imported consumers."""

import copy
from pathlib import Path


MODULE_BINDING_FIXTURE = "module_source_contract_bindings"


def test_name(name, frontend):
    if frontend == "native":
        return name
    if frontend != "module":
        raise ValueError("Unknown test frontend: " + frontend)
    if name == "parity_composite_corpus_source_contracts":
        return "module_composite_corpus_source_contracts"
    return "module_" + name


def fixture_name(name, frontend):
    if frontend == "native":
        return name
    if frontend != "module":
        raise ValueError("Unknown fixture frontend: " + frontend)
    return "module_" + name


def binding_command(command, frontend):
    """Project the three retained executed-helper contracts, not tool output."""
    if frontend == "native":
        return list(command)
    test_name("", frontend)
    result = []
    for part in command:
        if part == "--cpp-emulator=":
            continue
        result.append(part.replace("${BINARY_DIR}/teaql_",
                                   "${BINARY_DIR}/modules/teaql_module_"))
    return result


def binding_properties(properties, build, frontend):
    if frontend == "native":
        return copy.deepcopy(properties)
    test_name("", frontend)
    result = {row["name"]: copy.deepcopy(row["value"]) for row in properties}
    result["WORKING_DIRECTORY"] = str(Path(build).resolve() / "modules")
    for key in ("DEPENDS",):
        if key in result:
            result[key] = [test_name(value, frontend) for value in result[key]]
    for key in ("FIXTURES_SETUP", "FIXTURES_REQUIRED", "FIXTURES_CLEANUP"):
        if key in result:
            result[key] = [fixture_name(value, frontend) for value in result[key]]
    result["FIXTURES_REQUIRED"] = sorted(set(
        result.get("FIXTURES_REQUIRED", []) + [MODULE_BINDING_FIXTURE]))
    result["DEPENDS"] = sorted(set(result.get("DEPENDS", []) + [MODULE_BINDING_FIXTURE]))
    return [{"name": name, "value": result[name]} for name in sorted(result)]
