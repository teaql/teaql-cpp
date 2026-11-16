#!/usr/bin/env python3
"""Generate and verify imported test bindings to native source prerequisites."""

import argparse
import ast
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests/parity"))
from frontend_bindings_5_0_6 import MODULE_BINDING_FIXTURE, binding_properties, test_name


MANIFEST = ROOT / "modules/source_contracts.json"
ALIASES = {
    "module_examples_rust_parity": "runtime_examples_rust_parity",
    "module_canonical_examples_rust_parity": "runtime_examples_rust_parity",
    "module_relation_hydration_replay_rust_parity": "relation_hydration_rust_parity",
    "module_public_facets_rust_parity": "generated_support_rust_parity",
    "module_query_keys_5_0_6_rust_parity": "query_keys_rust_parity",
    "module_sqlite_schema_rust_parity": "provider_sqlite_schema_rust_parity",
    "module_execution_output_optimized_controls": "parity_execution_output_optimized_controls",
    "module_generated_cli_optimized_controls": "parity_generated_cli_optimized_controls",
    "module_deferred_resource_optimized_controls": "parity_deferred_resource_optimized_controls",
    "module_composite_corpus_source_contracts": "parity_composite_corpus_source_contracts",
}
PROGRAM_ALIASES = {
    "teaql_original_runtime_memory_service": "teaql_original_runtime_memory_service_tests",
    "teaql_log_privacy": "teaql_original_log_privacy_tests",
}
FRONTEND_CONTROLS = {
    "module_execution_output_optimized_controls",
    "module_generated_cli_optimized_controls",
    "module_deferred_resource_optimized_controls",
}
REGISTRATION_GUARDS = {
    "parity_generator_child_checks", "parity_executed_helper_validation",
}
COMPANIONS = {"parity_remaining_corpus_source_controls"}


def require(condition, message):
    if not condition:
        raise RuntimeError("Module source contracts: " + message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def properties(row):
    return {item["name"]: item["value"] for item in row["properties"]}


def registry(build):
    value = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(build), "--show-only=json-v1"], text=True))
    rows = {row["name"]: row for row in value["tests"]}
    require(len(rows) == len(value["tests"]), "duplicate actual test names")
    graph = value["backtraceGraph"]
    for row in rows.values():
        node = graph["nodes"][row["backtrace"]]
        declaration = Path(graph["files"][node["file"]])
        row["declaration_file"] = str(declaration if declaration.is_absolute()
                                      else ROOT / declaration)
    return rows


def python_entry(row):
    return any(value.endswith(".py") for value in row.get("command", []))


def encode(value, build):
    if isinstance(value, list):
        return [encode(item, build) for item in value]
    if isinstance(value, dict):
        return {name: encode(item, build) for name, item in value.items()}
    if not isinstance(value, str):
        return value
    value = value.replace(str(build.resolve()), "@BUILD@")
    value = value.replace(str(ROOT), "@SOURCE@")
    value = value.replace(str(ROOT.parent), "@WORKSPACE@")
    if value in (str(Path(sys.executable)), str(Path(sys.executable).resolve())):
        value = "@PYTHON@"
    if value in ("--profile=debug", "--profile=release"):
        value = "--profile=@ASSERT_PROFILE@"
    if value.startswith("--pointer-bytes="):
        value = "--pointer-bytes=@POINTER_BYTES@"
    return value


def expand(value, build, python, profile, pointer_bytes):
    if isinstance(value, list):
        return [expand(item, build, python, profile, pointer_bytes) for item in value]
    if isinstance(value, dict):
        return {name: expand(item, build, python, profile, pointer_bytes)
                for name, item in value.items()}
    if not isinstance(value, str):
        return value
    for token, replacement in {
        "@BUILD@": str(build.resolve()), "@SOURCE@": str(ROOT),
        "@WORKSPACE@": str(ROOT.parent), "@PYTHON@": str(python),
        "@ASSERT_PROFILE@": profile, "@POINTER_BYTES@": str(pointer_bytes),
    }.items():
        value = value.replace(token, replacement)
    require(not any(token in value for token in ("@BUILD@", "@SOURCE@", "@PYTHON@")),
            "unexpanded source-contract argument")
    return value


def normalized_command(command, build, module_name):
    result = []
    index = 0
    while index < len(command):
        value = command[index]
        if value == "--cpp-emulator=":
            index += 1
            continue
        if value == "--cpp-emulator" and command[index + 1:index + 2] == [""]:
            index += 2
            continue
        if value == "--cpp-prefix":
            require(Path(command[1]).name == "compare_inherited_checker_helpers_5_0_6.py"
                    and command[index + 1] == "teaql_module_inherited_checker_helpers_",
                    "undeclared inherited helper program prefix")
            index += 2
            continue
        if value == "--profile" and module_name in FRONTEND_CONTROLS:
            require(command[index + 1] == "module", "wrong control frontend")
            index += 2
            continue
        path = Path(value)
        if path == build.resolve() or path == build.resolve() / "modules":
            result.append("@PROGRAM_DIRECTORY@")
        elif path.is_relative_to(build.resolve()):
            name = path.name.replace("teaql_module_", "teaql_", 1)
            result.append("@PROGRAM@/" + PROGRAM_ALIASES.get(name, name))
        elif index == 0:
            result.append("@PYTHON@")
        else:
            result.append(value)
        index += 1
    return result


def source_environment(peer, module):
    native_values = dict(value.split("=", 1)
                         for value in properties(peer).get("ENVIRONMENT", []))
    module_values = dict(value.split("=", 1)
                         for value in properties(module).get("ENVIRONMENT", []))
    for name, value in module_values.items():
        if name in native_values:
            require(value == native_values[name], "conflicting native/module environment: " + name)
        else:
            require((name, value) == ("TEAQL_LOG_ENDPOINT", "off")
                    or (module["name"] == "module_log_formatter_rust_parity"
                        and (name, value) == ("TEAQL_PARITY_CPP_PREFIX", "teaql_module_")),
                    "undeclared module environment adaptation: " + name)
    return [name + "=" + value for name, value in sorted(module_values.items())]


def fixture_command(row, build, module_build):
    command = []
    values = row["command"]
    index = 0
    while index < len(values):
        if values[index] == "--ctest-build-dir":
            require(index + 1 < len(values), "source registration argument needs its value")
            index += 2
            continue
        command.append(values[index])
        index += 1
    if row["name"] in REGISTRATION_GUARDS:
        position = command.index("--build-dir") + 1
        command[position] = str(module_build.resolve())
        command.extend(["--frontend", "module"])
    require(not any(str(build.resolve()) in part for part in command),
            "source fixture still requires the native build directory: " + row["name"])
    return encode(command, module_build)


def projected_properties(row, module_build, environment=None, module_timeout=None):
    result = {item["name"]: item["value"]
              for item in binding_properties(row["properties"], module_build, "module")}
    if environment:
        merged = dict(value.split("=", 1) for value in environment)
        merged.update(dict(value.split("=", 1)
                           for value in properties(row).get("ENVIRONMENT", [])))
        result["ENVIRONMENT"] = [name + "=" + merged[name] for name in sorted(merged)]
    if "TIMEOUT" not in result and module_timeout is not None:
        result["TIMEOUT"] = module_timeout
    return encode(result, module_build)


def create_manifest(native, module, native_build, module_build):
    selected = [row for row in module.values() if python_entry(row)
                and row["name"].startswith("module_")
                and not Path(row["declaration_file"]).is_relative_to(module_build.resolve())]
    require(selected, "nonempty original module Python registration set")
    groups = set()
    bindings = []
    script_hashes = {}
    for row in selected:
        name = row["name"]
        peer_name = ALIASES.get(name, name.removeprefix("module_"))
        require(peer_name in native, "unclassified module Python registration: " + name)
        peer = native[peer_name]
        require(normalized_command(peer["command"], native_build, name)
                == normalized_command(row["command"], module_build, name),
                "unqualified command/selector difference: " + name)
        groups.update(properties(peer).get("FIXTURES_REQUIRED", []))
        bindings.append({"module": name, "native": peer_name,
                         "command": encode(row["command"], module_build),
                         "properties": projected_properties(
                             peer, module_build, source_environment(peer, row),
                             properties(row).get("TIMEOUT"))})
    owners = {}
    for row in native.values():
        for group in properties(row).get("FIXTURES_SETUP", []):
            require(group not in owners, "multiple source fixture owners: " + group)
            owners[group] = row
    pending = list(groups)
    while pending:
        group = pending.pop()
        require(group in owners, "missing native source fixture: " + group)
        for parent in properties(owners[group]).get("FIXTURES_REQUIRED", []):
            if parent not in groups:
                groups.add(parent)
                pending.append(parent)
    fixtures = []
    for name in sorted({owners[group]["name"] for group in groups}):
        peer = native[name]
        fixtures.append({"test": test_name(name, "module"), "native": name,
                         "command": fixture_command(peer, native_build, module_build),
                         "properties": projected_properties(peer, module_build)})
    companions = [{"test": test_name(name, "module"), "native": name,
                   "command": fixture_command(native[name], native_build, module_build),
                   "properties": projected_properties(native[name], module_build)}
                  for name in sorted(COMPANIONS)]
    for row in [*bindings, *fixtures, *companions]:
        for value in row["command"]:
            if value.startswith("@SOURCE@/") and value.endswith(".py"):
                spelling = value.removeprefix("@SOURCE@/")
                script_hashes[spelling] = digest(ROOT / spelling)
    for spelling in ("scripts/module_source_contracts.py", "scripts/verify_module_consumers.py",
                     "tests/parity/frontend_bindings_5_0_6.py"):
        script_hashes[spelling] = digest(ROOT / spelling)
    baseline = json.loads((ROOT / "upstream.lock.json").read_text())
    data = {
        "schema": 1, "runtime_commit": baseline["commit"],
        "native_registration_sha256": digest(ROOT / "tests/CMakeLists.txt"),
        "canonical_contracts_sha256": digest(ROOT / "modules/contracts.json"),
        "aliases": ALIASES, "bindings": sorted(bindings, key=lambda row: row["module"]),
        "fixtures": fixtures, "companions": companions,
        "source_groups": sorted(groups), "source_programs": script_hashes,
        "non_python_tests": sorted(name for name, row in module.items()
                                   if name.startswith("module_") and not python_entry(row)),
    }
    validate_shape(data)
    return data


def refresh(native_build, module_build, path):
    data = create_manifest(registry(native_build), registry(module_build), native_build, module_build)
    path.write_text(json.dumps(data, indent=2) + "\n")
    print(f"Generated {len(data['bindings'])} module bindings and {len(data['fixtures'])} source fixtures")


def validate_shape(data):
    require(data["schema"] == 1 and data["aliases"] == ALIASES, "manifest schema and declared aliases")
    names = [row["module"] for row in data["bindings"]]
    require(names and len(names) == len(set(names))
            and all(name.startswith("module_") for name in names),
            "complete unique Python bindings in the module preset namespace")
    require({row["native"] for row in data["companions"]} == COMPANIONS,
            "complete executed-helper companion control")
    fixtures = {row["test"]: row for row in data["fixtures"]}
    require(len(fixtures) == len(data["fixtures"]), "unique source fixture names")
    setup = {}
    for row in fixtures.values():
        for label in row["properties"].get("FIXTURES_SETUP", []):
            require(label not in setup, "unique imported source fixture owner")
            setup[label] = row["test"]
    require(setup.keys() == {"module_" + name for name in data["source_groups"]},
            "complete declared source group closure")
    for row in [*data["bindings"], *data["fixtures"], *data["companions"]]:
        require(MODULE_BINDING_FIXTURE in row["properties"].get("FIXTURES_REQUIRED", []),
                "source registration guard required")
        for label in row["properties"].get("FIXTURES_REQUIRED", []):
            require(label == MODULE_BINDING_FIXTURE or label in setup,
                    "missing required imported fixture: " + label)
        require(row["command"] and all(isinstance(part, str) and "\x00" not in part
                                       for part in row["command"]), "well-formed source command")
    for row in data["fixtures"]:
        require("--ctest-build-dir" not in row["command"]
                and not any("build_native_current" in value for value in row["command"]),
                "module source checks are independent of a native build tree")
        if row["native"] in REGISTRATION_GUARDS:
            require(row["command"][-2:] == ["--frontend", "module"],
                    "explicit frontend for registration-sensitive source guards")
    graph = {name: {setup[label] for label in row["properties"].get("FIXTURES_REQUIRED", [])
                    if label != MODULE_BINDING_FIXTURE} for name, row in fixtures.items()}
    def visit(name, active, done):
        require(name not in active, "source fixture dependency cycle")
        if name not in done:
            for parent in graph[name]:
                visit(parent, active | {name}, done)
            done.add(name)
    done = set()
    for name in graph:
        visit(name, set(), done)


def quote(value):
    if isinstance(value, bool):
        value = "TRUE" if value else "FALSE"
    if isinstance(value, list):
        value = ";".join(map(str, value))
    text = str(value)
    delimiter = "="
    while "]" + delimiter + "]" in text:
        delimiter += "="
    return "[" + delimiter + "[" + text + "]" + delimiter + "]"


def definitions(data, build, python, profile, pointer_bytes):
    result = {}
    for row in [*data["bindings"], *data["fixtures"], *data["companions"]]:
        name = row.get("module", row.get("test"))
        result[name] = expand(row, build, python, profile, pointer_bytes)
    return result


def emit(data, args):
    validate_shape(data)
    rows = definitions(data, args.build_dir, args.python, args.assert_profile, args.pointer_bytes)
    lines = ["# Generated from modules/source_contracts.json; regenerate instead of editing."]
    for row in [*data["fixtures"], *data["companions"]]:
        name = row["test"]
        command = rows[name]["command"]
        lines.extend([f"if(NOT TEST {name})", "    add_test(NAME " + name + " COMMAND "
                      + " ".join(quote(value) for value in command) + ")", "endif()"])
    for name, row in rows.items():
        lines.extend([f"if(NOT TEST {name})", f'    message(FATAL_ERROR "Missing module source binding: {name}")',
                      "endif()", "set_tests_properties(" + name + " PROPERTIES"])
        lines.extend("    " + prop + " " + quote(value) for prop, value in row["properties"].items())
        lines.append(")")
    command = [args.python, "-B", str(Path(__file__).resolve()), "verify", "--build-dir",
               str(args.build_dir.resolve()), "--python", args.python, "--assert-profile",
               args.assert_profile, "--pointer-bytes", str(args.pointer_bytes)]
    lines.append("add_test(NAME " + MODULE_BINDING_FIXTURE + " COMMAND "
                 + " ".join(quote(value) for value in command) + ")")
    lines.append("set_tests_properties(" + MODULE_BINDING_FIXTURE + " PROPERTIES FIXTURES_SETUP "
                 + MODULE_BINDING_FIXTURE + ")")
    text = "\n".join(lines) + "\n"
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if not args.output.exists() or args.output.read_text() != text:
        args.output.write_text(text)


def validate_registry(data, args, tests):
    validate_shape(data)
    expected = definitions(data, args.build_dir, args.python, args.assert_profile, args.pointer_bytes)
    require(set(tests) == set(expected) | set(data["non_python_tests"]) | {MODULE_BINDING_FIXTURE},
            "every current module registration has an explicit disposition")
    for name, row in expected.items():
        require(tests[name].get("command") == row["command"], "actual module command: " + name)
        require(properties(tests[name]) == row["properties"], "actual source fixture properties: " + name)
    guard = tests[MODULE_BINDING_FIXTURE]
    command = [args.python, "-B", str(Path(__file__).resolve()), "verify", "--build-dir",
               str(args.build_dir.resolve()), "--python", args.python, "--assert-profile",
               args.assert_profile, "--pointer-bytes", str(args.pointer_bytes)]
    require(guard.get("command") == command
            and properties(guard).get("FIXTURES_SETUP") == [MODULE_BINDING_FIXTURE]
            and not properties(guard).get("FIXTURES_REQUIRED"),
            "registration guard fixture is installed")


def corruption_controls(data, args, tests):
    results = []
    for name in ("missing_binding", "duplicate_binding", "missing_fixture", "missing_group",
                 "fixture_cycle", "native_directory_dependency", "missing_frontend",
                 "missing_companion", "wrong_manifest_command", "wrong_actual_command",
                 "missing_actual_test", "unknown_actual_test", "missing_actual_requirement",
                 "wrong_actual_owner", "missing_registration_guard", "wrong_guard_command"):
        value, current = copy.deepcopy(data), copy.deepcopy(tests)
        first = value["bindings"][0]["module"]
        if name == "missing_binding":
            value["bindings"].pop()
        elif name == "duplicate_binding":
            value["bindings"].append(copy.deepcopy(value["bindings"][0]))
        elif name == "missing_fixture":
            value["fixtures"].pop()
        elif name == "missing_group":
            value["source_groups"].pop()
        elif name == "fixture_cycle":
            row = value["fixtures"][0]
            row["properties"]["FIXTURES_REQUIRED"].extend(row["properties"]["FIXTURES_SETUP"])
        elif name == "native_directory_dependency":
            value["fixtures"][0]["command"].extend(["--ctest-build-dir", "build_native_current"])
        elif name == "missing_frontend":
            row = next(row for row in value["fixtures"] if row["native"] in REGISTRATION_GUARDS)
            row["command"] = row["command"][:-2]
        elif name == "missing_companion":
            value["companions"] = []
        elif name == "wrong_manifest_command":
            value["bindings"][0]["command"].append("--unqualified-selector")
        elif name == "wrong_actual_command":
            current[first]["command"].append("--unqualified-selector")
        elif name == "missing_actual_test":
            current.pop(first)
        elif name == "unknown_actual_test":
            current["module_unclassified_source_comparison"] = copy.deepcopy(current[first])
        elif name == "missing_actual_requirement":
            current[first]["properties"] = [row for row in current[first]["properties"]
                                             if row["name"] != "FIXTURES_REQUIRED"]
        elif name == "wrong_actual_owner":
            row = current[value["fixtures"][0]["test"]]
            next(prop for prop in row["properties"] if prop["name"] == "FIXTURES_SETUP")["value"] = ["wrong"]
        elif name == "missing_registration_guard":
            current.pop(MODULE_BINDING_FIXTURE)
        elif name == "wrong_guard_command":
            current[MODULE_BINDING_FIXTURE]["command"].append("--unqualified")
        try:
            validate_registry(value, args, current)
        except RuntimeError as error:
            results.append({"case": name, "rejected": True, "error": str(error)})
        else:
            raise RuntimeError("Accepted module source-binding corruption: " + name)
    return results


def verify(data, args, tests=None):
    validate_shape(data)
    require(data["runtime_commit"] == json.loads((ROOT / "upstream.lock.json").read_text())["commit"],
            "locked runtime baseline")
    require(data["native_registration_sha256"] == digest(ROOT / "tests/CMakeLists.txt"),
            "native declarations changed; refresh module source contracts")
    require(data["canonical_contracts_sha256"] == digest(ROOT / "modules/contracts.json"),
            "canonical module contracts changed; refresh source contracts")
    for spelling, expected in data["source_programs"].items():
        require(digest(ROOT / spelling) == expected, "source program changed: " + spelling)
    tests = registry(args.build_dir) if tests is None else tests
    tests = {name: row for name, row in tests.items() if name.startswith("module_")}
    with tempfile.TemporaryDirectory(prefix="teaql_module_native_declarations_") as directory:
        native_build = Path(directory)
        configuration = "Debug" if args.assert_profile == "debug" else "Release"
        result = subprocess.run(
            ["cmake", "--preset", "native", "-B", str(native_build),
             "-DCMAKE_BUILD_TYPE=" + configuration], cwd=ROOT, text=True, capture_output=True)
        require(result.returncode == 0, "native declaration reproduction: " + result.stdout + result.stderr)
        reproduced = create_manifest(registry(native_build), tests, native_build, args.build_dir)
    require(data == reproduced, "manifest differs from actual native declarations and module callers")
    subprocess.run([str(args.python), "-B", str(ROOT / "scripts/verify_module_consumers.py"),
                    "--build-dir", str(args.build_dir)], check=True)
    validate_registry(data, args, tests)
    rejected = corruption_controls(data, args, tests)
    print(f"Verified {len(data['bindings'])} module Python bindings, "
          f"{len(data['fixtures'])} source fixtures and {len(data['companions'])} companion controls; "
          f"{len(rejected)} corruptions rejected")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("refresh", "emit", "verify"))
    parser.add_argument("--manifest", type=Path, default=MANIFEST)
    parser.add_argument("--native-build-dir", type=Path, default=ROOT / "build_native_current")
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_modules_native")
    parser.add_argument("--python", default=str(Path(sys.executable).resolve()))
    parser.add_argument("--assert-profile", choices=("debug", "release"), default="debug")
    parser.add_argument("--pointer-bytes", type=int, choices=(4, 8), default=8)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.mode == "refresh":
        refresh(args.native_build_dir, args.build_dir, args.manifest)
        return
    data = json.loads(args.manifest.read_text())
    if args.mode == "emit":
        require(args.output is not None, "generated CMake output is required")
        emit(data, args)
    else:
        verify(data, args)


if __name__ == "__main__":
    main()
