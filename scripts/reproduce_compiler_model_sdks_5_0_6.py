#!/usr/bin/env python3
"""Reproduce retained SDKs with only reviewed compiled-metadata fingerprints changed."""

import argparse
import ast
import copy
import gzip
import hashlib
import json
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import types

from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references
from verify_primitive_const_governed_execution_5_0_6 import run_owned
from reproduce_compiler_projection_sdks_5_0_6 import UnconditionalChecks, snapshot

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
BASE = ROOT / "docs/parity/compiler_source_service_sdks_current_5_0_6.json.gz"
BASE_SHA = "e9fafcf61cf5c794902740efe606e585a62ee8b8e3275f520665b7bd5c50f625"
TRANSITION = ROOT / "docs/parity/compiler_model_configuration_transition_5_0_6.json.gz"
TRANSITION_SHA = "96f9b65d30e21ff94ff9f90988e8d4c4918784307f74ad5f201d60fae403fbef"
PREFIX = "generator/src/main/java/com/skynet/codegenerator/cpp/"
CHANGED = tuple(PREFIX + name + ".java" for name in ("CppCoreGenerator", "CppCompilerProjectionResolver"))
ADDED = tuple(PREFIX + name + ".java" for name in ("CppModelConfiguration", "CppCompilerModelConfiguration"))
RECIPES = {"reproduce_cpp_contract_fixtures.py": 45, "generate_examples.py": 0,
           "reproduce_cpp_independent_sdks_5_0_6.py": 9}


def require(value, message):
    if not value:
        raise RuntimeError("Compiled model SDK reproduction: " + message)


def sha(value):
    return hashlib.sha256(value).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


class RestoreInputs(ast.NodeTransformer):
    def __init__(self):
        self.removed = 0

    def visit_List(self, node):
        self.generic_visit(node)
        retained = []
        for item in node.elts:
            if isinstance(item, ast.Constant) and item.value in (*ADDED, "CppModelConfiguration", "CppCompilerModelConfiguration"):
                self.removed += 1
            else:
                retained.append(item)
        node.elts = retained
        return node

    visit_Tuple = visit_List


def load_recipe(name, transition):
    row = transition["paths"]["runtime:scripts/" + name]
    current = (ROOT / "scripts" / name).read_text()
    require(current == row["after_source"] and sha(current.encode()) == row["after_sha256"], "exact current SDK recipe")
    restored = RestoreInputs()
    tree = restored.visit(ast.parse(current))
    require(restored.removed == 2 and ast.dump(tree, include_attributes=False)
            == ast.dump(ast.parse(row["before_source"]), include_attributes=False), "only two provenance inputs added; every other recipe body preserved")
    hardener = UnconditionalChecks()
    tree = ast.fix_missing_locations(hardener.visit(ast.parse(current)))
    require(hardener.count == RECIPES[name] and not any(isinstance(node, ast.Assert) for node in ast.walk(tree)), "all existing recipe checks are unconditional")
    module = types.ModuleType("compiled_model_sdk_recipe")
    module.__file__ = str(ROOT / "scripts" / name)
    exec(compile(tree, module.__file__, "exec"), module.__dict__)
    return module


def expected_manifest(before, transition):
    result = copy.deepcopy(before["value"])
    inputs = result["generator_implementation_sha256"]
    for name in CHANGED:
        row = transition["paths"]["producer:" + name]
        require(inputs[name] == row["before_sha256"], "exact preceding generator fingerprint")
        inputs[name] = row["after_sha256"]
    for name in ADDED:
        require(name not in inputs, "compiled metadata capability was absent")
        inputs[name] = transition["paths"]["producer:" + name]["after_sha256"]
    return result


def validate(record):
    require(sha(BASE.read_bytes()) == BASE_SHA and sha(TRANSITION.read_bytes()) == TRANSITION_SHA, "immutable baseline and exact source transition")
    transition = read(TRANSITION)
    require(record["complete"] is True and record["whole_goal_complete"] is False and record["driver_source"] == Path(__file__).read_text()
            and record["baseline_sha256"] == BASE_SHA and record["transition_sha256"] == TRANSITION_SHA, "completed current reproduction")
    require(record["before"] == read(BASE)["after"] and record["observed_before"] == record["before"]
            and len(record["cases"]) == len(record["after"]) == 80, "eighty exact preceding SDKs")
    for old, new, case in zip(record["before"], record["after"], record["cases"], strict=True):
        name = Path(old["manifest"]).name
        require(old["manifest"] == new["manifest"] == case["manifest"] and old["files"].keys() == new["files"].keys()
                and case["changed_files"] == [name] and case["code_and_assist_identical"] is True
                and all(new["files"][key] == value for key, value in old["files"].items() if key != name), "all retained SDK code and guidance unchanged")
        require(new["value"] == expected_manifest(old, transition) and sha(new["text"].encode()) == new["files"][name], "only four reviewed generator fingerprints change")
        directory = (ROOT / new["manifest"]).parent
        require(all(sha((directory / key).read_bytes()) == value for key, value in new["files"].items()), "actual retained SDK bytes")
    require(sum(len(row["files"]) - 1 for row in record["after"]) == 3560, "all retained code/Assist file membership")
    require(len(record["actions"]) == 6 and all(type(action["exit_code"]) is int and action["exit_code"] == 0 for action in record["actions"])
            and record["assertions_hardened"] == RECIPES and record["owned_directory_removed"] is True, "actual complete regeneration/check actions and cleanup")
    before, after = record["input_hashes_before"], record["input_hashes_after"]
    allowed = {"runtime:" + row["manifest"] for row in record["after"]}
    require(before.keys() == after.keys() and {name for name in before if before[name] != after[name]} <= allowed
            and record["references_before"] == record["references_after"], "only declared manifest changes during reproduction")
    for name in RECIPES:
        load_recipe(name, transition)


def execute(path):
    require(not path.exists() and sha(BASE.read_bytes()) == BASE_SHA and sha(TRANSITION.read_bytes()) == TRANSITION_SHA, "fresh output and immutable preimages")
    transition = read(TRANSITION)
    before = read(BASE)["after"]
    observed = snapshot()
    require(observed == before, "preserve preceding complete retained SDK state")
    for name in RECIPES:
        load_recipe(name, transition)
    maven = read(ROOT / "docs/parity/ap02_primitive_const_path_installed_5_0_6.json.gz")["steps"][0]["command"][0]
    record = dict(complete=False, whole_goal_complete=False, baseline_sha256=BASE_SHA, transition_sha256=TRANSITION_SHA,
                  before=before, observed_before=observed, driver_source=Path(__file__).read_text(), assertions_hardened=RECIPES,
                  input_hashes_before=capture_inputs(), references_before=references(), steps=[], actions=[], cases=[], independent_receipts={})

    def run(command, *, cwd=None, text=False, capture_output=False, input=None, check=False,
            env=None, timeout=900):
        historical = list(map(str, command))
        actual = list(historical)
        if actual[0] == "mvn":
            actual = [maven, "-o", "-Dexec.skip=true", *actual[1:]]
        value = input if text or input is None else input.decode()
        result = run_owned(actual, cwd=cwd, env=env, input=value, text=True, capture_output=True, timeout=timeout)
        record["steps"].append(dict(command=actual, source_command=historical, cwd=str(cwd or ROOT), exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        if check:
            result.check_returncode()
        return subprocess.CompletedProcess(actual, result.returncode,
            (result.stdout if text else result.stdout.encode()) if capture_output else None,
            (result.stderr if text else result.stderr.encode()) if capture_output else None)

    def check_output(command, **kwargs):
        result = run(command, text=kwargs.pop("text", False), capture_output=True, check=True, **kwargs)
        return result.stdout

    prior_tempdir = tempfile.tempdir
    prior_argv = sys.argv
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_model_sdks_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            tempfile.tempdir = directory
            actions = [("reproduce_cpp_contract_fixtures.py", []), ("generate_examples.py", []),
                       ("reproduce_cpp_independent_sdks_5_0_6.py", ["--refresh-provenance", "--record", str(owned / "refresh.json.gz")]),
                       ("reproduce_cpp_contract_fixtures.py", ["--check"]), ("generate_examples.py", ["--check"]),
                       ("reproduce_cpp_independent_sdks_5_0_6.py", ["--record", str(owned / "check.json.gz")])]
            try:
                for name, arguments in actions:
                    module = load_recipe(name, transition)
                    module.subprocess = types.SimpleNamespace(run=run, check_output=check_output)
                    sys.argv = [module.__file__, *arguments]
                    print(name, "check" if "--check" in arguments else "refresh", flush=True)
                    require(module.main() in (None, 0), "successful active SDK recipe")
                    record["actions"].append(dict(recipe=name, args=arguments, exit_code=0))
                    if "--record" in arguments:
                        child = Path(arguments[arguments.index("--record") + 1])
                        record["independent_receipts"][child.name] = read(child)
                    write(path, record)
                record["after"] = snapshot()
                for old, new in zip(before, record["after"], strict=True):
                    name = Path(old["manifest"]).name
                    record["cases"].append(dict(manifest=old["manifest"], changed_files=[key for key, value in old["files"].items() if new["files"].get(key) != value],
                        code_and_assist_identical=all(new["files"].get(key) == value for key, value in old["files"].items() if key != name)))
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
        record.update(complete=True, owned_directory_removed=not Path(directory).exists(), input_hashes_after=capture_inputs(), references_after=references())
        validate(record)
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        tempfile.tempdir = prior_tempdir
        sys.argv = prior_argv
        if "owned_directory" in record:
            record["owned_directory_removed"] = not Path(record["owned_directory"]).exists()
        write(path, record)


def controls(path):
    record = read(path)
    validate(record)
    mutations = (
        ("incomplete", lambda r: r.update(complete=False)),
        ("missing_sdk", lambda r: r["after"].pop()),
        ("changed_code", lambda r: r["after"][0]["files"].update(invented="0" * 64)),
        ("changed_fingerprint", lambda r: r["after"][0]["value"]["generator_implementation_sha256"].update(invented="0" * 64)),
        ("lost_action", lambda r: r["actions"].pop()),
        ("coerced_exit", lambda r: r["actions"][0].update(exit_code=False)),
        ("lost_hardening", lambda r: r["assertions_hardened"].update(generate_examples=1)),
        ("unclosed_outputs", lambda r: r.update(owned_directory_removed=False)),
    )
    rejected = []
    for name, mutate in mutations:
        changed = copy.deepcopy(record)
        mutate(changed)
        try:
            validate(changed)
        except RuntimeError as error:
            rejected.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Retained SDK corruption accepted: " + name)
    return dict(complete=True, optimized=sys.flags.optimize, execution_sha256=sha(path.read_bytes()), controls=rejected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", type=Path)
    parser.add_argument("--controls", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    signal.signal(signal.SIGTERM, lambda *_: (_ for _ in ()).throw(KeyboardInterrupt("Reap this SDK job's owned child")))
    if args.execute:
        execute(args.execute)
    elif args.controls and args.output:
        require(not args.output.exists(), "preserve SDK controls")
        result = controls(args.controls)
        write(args.output, result)
        print(json.dumps(dict(complete=True, optimized=result["optimized"], controls=len(result["controls"]))))
    else:
        parser.error("Choose --execute or --controls with --output")


if __name__ == "__main__":
    main()
