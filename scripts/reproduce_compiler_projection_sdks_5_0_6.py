#!/usr/bin/env python3
"""Regenerate eighty SDKs after explicit compiler resolver integration."""

import argparse
import ast
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import types

from reproduce_const_function_sdks_5_0_6 import snapshot, UnconditionalChecks
from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references
from verify_primitive_const_governed_execution_5_0_6 import run_owned

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
BASE = ROOT / "docs/parity/ap02_const_function_sdks_5_0_6.json.gz"
BASE_SHA = "2480fa3b0782fdf392fe16f3e43a434ee1ce895b6eb0894c488c3e3c2736e984"
TRANSITION = ROOT / "docs/parity/compiler_projection_source_transition_5_0_6.json.gz"
TRANSITION_SHA = "77eab8ca41092e1828ec38c69366d78a45b38074b7472a6742c88b034255a814"
CORE = "generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java"
HELPERS = tuple("generator/src/main/java/com/skynet/codegenerator/cpp/" + name + ".java"
                for name in ("CppProjectionResolver", "CppCompilerProjectionResolver"))
RECIPES = {"reproduce_cpp_contract_fixtures.py": 45, "generate_examples.py": 0,
           "reproduce_cpp_independent_sdks_5_0_6.py": 9}


def require(value, message):
    if not value:
        raise RuntimeError("Compiler resolver SDK reproduction: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


class RestoreProvenanceInputs(ast.NodeTransformer):
    def __init__(self):
        self.removed = 0

    def visit_List(self, node):
        self.generic_visit(node)
        retained = []
        for child in node.elts:
            if isinstance(child, ast.Constant) and child.value in (*HELPERS, "CppProjectionResolver", "CppCompilerProjectionResolver"):
                self.removed += 1
            else:
                retained.append(child)
        node.elts = retained
        return node

    visit_Tuple = visit_List


def load_recipe(name, transition, *, historical=False):
    from compiler_model_configuration_source_admission_5_0_6 import registered_epoch_matches as model_epoch_matches

    entry = transition["paths"]["runtime:scripts/" + name]
    current = (ROOT / "scripts" / name).read_text()
    if historical:
        require(model_epoch_matches(ROOT / "scripts" / name, entry["after_sha256"]), "qualified current recipe epoch")
        current = entry["after_source"]
    require(current == entry["after_source"] and sha(current.encode()) == entry["after_sha256"],
            "exact active recipe source")
    restored = RestoreProvenanceInputs()
    tree = restored.visit(ast.parse(current))
    require(restored.removed == 2 and ast.dump(tree, include_attributes=False)
            == ast.dump(ast.parse(entry["before_source"]), include_attributes=False),
            "only two producer provenance inputs added; every historical recipe body preserved")
    hardener = UnconditionalChecks()
    tree = ast.fix_missing_locations(hardener.visit(ast.parse(current)))
    require(hardener.count == RECIPES[name] and not any(isinstance(n, ast.Assert) for n in ast.walk(tree)),
            "all inherited recipe decisions remain unconditional")
    module = types.ModuleType("compiler_resolver_sdk_recipe")
    module.__file__ = str(ROOT / "scripts" / name)
    exec(compile(tree, module.__file__, "exec"), module.__dict__)
    return module


def expected_manifest(before, transition):
    result = copy.deepcopy(before["value"])
    inputs = result["generator_implementation_sha256"]
    core = transition["paths"]["producer:" + CORE]
    require(inputs[CORE] == core["before_sha256"], "exact preceding core generator fingerprint")
    inputs[CORE] = core["after_sha256"]
    for name in HELPERS:
        require(name not in inputs, "compiler resolver was absent from the old SDK epoch")
        inputs[name] = transition["paths"]["producer:" + name]["after_sha256"]
    return result


def validate(record):
    from compiler_model_configuration_source_admission_5_0_6 import current_input_matches as model_input_matches

    require(sha(BASE.read_bytes()) == BASE_SHA and sha(TRANSITION.read_bytes()) == TRANSITION_SHA,
            "immutable prior SDK and source transition")
    transition = read(TRANSITION)
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["baseline_sha256"] == BASE_SHA and record["transition_sha256"] == TRANSITION_SHA
            and model_input_matches(Path(__file__), sha(record["driver_source"].encode())), "exact completed reproduction")
    require(record["before"] == read(BASE)["after"] and len(record["after"]) == len(record["cases"]) == 80,
            "eighty complete preceding/current SDKs")
    for old, new, case in zip(record["before"], record["after"], record["cases"], strict=True):
        name = Path(old["manifest"]).name
        require(old["manifest"] == new["manifest"] == case["manifest"]
                and old["files"].keys() == new["files"].keys()
                and case["changed_files"] == [name] and case["code_and_assist_identical"] is True
                and all(new["files"][key] == value for key, value in old["files"].items() if key != name),
                "all generated code/guidance files remain identical")
        require(new["value"] == expected_manifest(old, transition)
                and sha(new["text"].encode()) == new["files"][name], "only core fingerprint and two resolver inputs change")
        directory = (ROOT / new["manifest"]).parent
        require(all(model_input_matches(directory / key, value) for key, value in new["files"].items()),
                "actual current complete retained SDK bytes")
    require(sum(len(row["files"]) - 1 for row in record["after"]) == 3560,
            "all 3560 retained code and guidance files")
    require(len(record["actions"]) == 6 and all(type(a["exit_code"]) is int and a["exit_code"] == 0 for a in record["actions"]),
            "actual refresh and strict check actions")
    require(record["assertions_hardened"] == RECIPES and record["owned_directory_removed"] is True,
            "unconditional old checks and owned cleanup")
    before, after = record["input_hashes_before"], record["input_hashes_after"]
    allowed = {"runtime:" + row["manifest"] for row in record["after"]}
    require(before.keys() == after.keys() and {k for k in before if before[k] != after[k]} <= allowed
            and record["references_before"] == record["references_after"], "only declared SDK manifests change during execution")
    for name in RECIPES:
        load_recipe(name, transition, historical=True)
    for key, entry in transition["paths"].items():
        root = PRODUCER if key.startswith("producer:") else ROOT
        require(model_input_matches(root / key.split(":", 1)[1], entry["after_sha256"]), "exact tested source transition")


def execute(path):
    require(not path.exists() and sha(BASE.read_bytes()) == BASE_SHA and sha(TRANSITION.read_bytes()) == TRANSITION_SHA,
            "fresh receipt and immutable preimages")
    before, transition = read(BASE)["after"], read(TRANSITION)
    observed = snapshot()
    for current, old in zip(observed, before, strict=True):
        require(current["manifest"] == old["manifest"] and all(current["files"].get(k) == v for k, v in old["files"].items()
                if k != Path(old["manifest"]).name) and current["value"] in (old["value"], expected_manifest(old, transition)),
                "resume only known old/current provenance; preserve every SDK body")
    for name in RECIPES:
        load_recipe(name, transition)
    maven_record = read(ROOT / "docs/parity/ap02_primitive_const_path_installed_5_0_6.json.gz")
    maven = maven_record["steps"][0]["command"][0]
    record = dict(complete=False, whole_goal_complete=False, baseline_sha256=BASE_SHA,
                  transition_sha256=TRANSITION_SHA, before=before, observed_before=observed,
                  driver_source=Path(__file__).read_text(), assertions_hardened=RECIPES,
                  input_hashes_before=capture_inputs(), references_before=references(),
                  actions=[], steps=[], cases=[], independent_receipts={})

    def save():
        write(path, record)

    def run(command, *, cwd=None, text=False, capture_output=False, input=None, check=False,
            env=None, timeout=900):
        historical = list(map(str, command))
        actual = list(historical)
        if actual[0] == "mvn":
            actual = [maven, "-o", "-Dexec.skip=true", *actual[1:]]
        value = input if text or input is None else input.decode()
        outcome = run_owned(actual, cwd=cwd, env=env, input=value, text=True, capture_output=True, timeout=timeout)
        record["steps"].append(dict(command=actual, source_command=historical, cwd=str(cwd or ROOT),
                                    exit_code=outcome.returncode, stdout=outcome.stdout, stderr=outcome.stderr))
        save()
        if check:
            outcome.check_returncode()
        return subprocess.CompletedProcess(actual, outcome.returncode,
            (outcome.stdout if text else outcome.stdout.encode()) if capture_output else None,
            (outcome.stderr if text else outcome.stderr.encode()) if capture_output else None)

    def check_output(command, **kwargs):
        outcome = run(command, text=kwargs.pop("text", False), capture_output=True, check=True, **kwargs)
        return outcome.stdout

    previous_directory, previous_argv = tempfile.tempdir, sys.argv
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_projection_sdks_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            tempfile.tempdir = directory
            actions = [("reproduce_cpp_contract_fixtures.py", []), ("generate_examples.py", []),
                       ("reproduce_cpp_independent_sdks_5_0_6.py", ["--refresh-provenance", "--record", str(owned / "refresh.json.gz")]),
                       ("reproduce_cpp_contract_fixtures.py", ["--check"]), ("generate_examples.py", ["--check"]),
                       ("reproduce_cpp_independent_sdks_5_0_6.py", ["--record", str(owned / "check.json.gz")])]
            try:
                for name, args in actions:
                    module = load_recipe(name, transition)
                    module.subprocess = types.SimpleNamespace(run=run, check_output=check_output)
                    sys.argv = [str(ROOT / "scripts" / name), *args]
                    print("Reproduce:", name, "check" if "--check" in args else "refresh", flush=True)
                    result = module.main()
                    require(result in (None, 0), "active recipe returned success")
                    record["actions"].append(dict(recipe=name, args=args, exit_code=0))
                    if "--record" in args:
                        child = Path(args[args.index("--record") + 1])
                        record["independent_receipts"][child.name] = read(child)
                    save()
                record["after"] = snapshot()
                for old, new in zip(before, record["after"], strict=True):
                    record["cases"].append(dict(manifest=old["manifest"],
                        changed_files=[k for k, value in old["files"].items() if new["files"].get(k) != value],
                        code_and_assist_identical=all(new["files"].get(k) == v for k, v in old["files"].items()
                                                     if k != Path(old["manifest"]).name)))
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
        record.update(complete=True, owned_directory_removed=not Path(record["owned_directory"]).exists(),
                      input_hashes_after=capture_inputs(), references_after=references())
        validate(record)
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        tempfile.tempdir, sys.argv = previous_directory, previous_argv
        if "owned_directory" in record:
            record["owned_directory_removed"] = not Path(record["owned_directory"]).exists()
        save()


def controls(record):
    result = []
    for name in ("incomplete", "missing_sdk", "changed_code", "wrong_provenance", "missing_resolver_input", "missing_action", "lost_hardening", "cleanup"):
        changed = copy.deepcopy(record)
        if name == "incomplete": changed["complete"] = False
        elif name == "missing_sdk": changed["after"].pop()
        elif name == "changed_code": changed["after"][0]["files"][next(k for k in changed["after"][0]["files"] if k.endswith(".hpp"))] = "0" * 64
        elif name == "wrong_provenance": changed["after"][0]["value"]["generator_implementation_sha256"][CORE] = "0" * 64
        elif name == "missing_resolver_input": changed["after"][0]["value"]["generator_implementation_sha256"].pop(HELPERS[0])
        elif name == "missing_action": changed["actions"].pop()
        elif name == "lost_hardening": changed["assertions_hardened"] = {}
        elif name == "cleanup": changed["owned_directory_removed"] = False
        try:
            validate(changed)
        except RuntimeError as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted SDK corruption: " + name)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--execute", action="store_true")
    action.add_argument("--check", action="store_true")
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    if args.execute:
        def stop(signum, frame):
            raise KeyboardInterrupt("Reap this SDK job's owned child")
        signal.signal(signal.SIGTERM, stop)
        execute(args.record.resolve())
    else:
        record = read(args.record)
        validate(record)
        rejected = controls(record)
        if args.controls:
            require(not args.controls.exists(), "preserve prior controls")
            write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize,
                  execution_sha256=sha(args.record.read_bytes()), controls=rejected))
        print("Compiler resolver SDKs: 80 SDKs, 3560 code/guidance files;", len(rejected), "corruptions rejected; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
