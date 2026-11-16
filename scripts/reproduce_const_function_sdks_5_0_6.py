#!/usr/bin/env python3
"""Reproduce the retained SDKs after the qualified constant-function repair."""

import argparse
import ast
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import traceback
import types

from reproduce_cpp_contract_fixtures import metadata_manifest_paths
from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
PARSER = "generator/src/main/java/com/skynet/codegenerator/cpp/CppRustLiteralMetadata.java"
BASE = "ap02_primitive_const_path_sdks_5_0_6.json.gz"
BASE_SHA = "99363b549281aab35bca4fc7b9bd2f0ddc974013c49df39eee2b33440f7255c6"
PROPOSAL = "ap02_const_function_source_proposal_5_0_6.json.gz"
PROPOSAL_SHA = "f9114c0d2785eaf91270fa53db2228563d526b4b9004ccc08da18226cddcf0e4"
RECIPES = {
    "reproduce_cpp_contract_fixtures.py": (
        "84d4cc2049ecc33969185df4c2f9a3aa32fa1519ca246ef905b3c54c59843618", 45),
    "generate_examples.py": (
        "ed3745751852fc3344eb93be939c78046e33afc2ca45bd489d07ac5c07df19f0", 0),
    "reproduce_cpp_independent_sdks_5_0_6.py": (
        "70b2b098cafae810b196ee057632678eb461e044a159ed1dedb112d94ef88d3b", 9),
}


def require(value, message):
    if not value:
        raise RuntimeError("Constant-function SDK reproduction: " + message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def bound(name, expected):
    path = EVIDENCE / name
    require(digest(path) == expected, "immutable evidence: " + name)
    return read(path)


def snapshot():
    paths = metadata_manifest_paths(ROOT)
    require(len(paths) == len({path.parent for path in paths}) == 80,
            "all eighty distinct SDKs")
    return [{"manifest": path.relative_to(ROOT).as_posix(),
             "files": {p.relative_to(path.parent).as_posix(): digest(p)
                       for p in sorted(path.parent.rglob("*")) if p.is_file()},
             "text": path.read_text(), "value": json.loads(path.read_text())}
            for path in paths]


class UnconditionalChecks(ast.NodeTransformer):
    def __init__(self):
        self.count = 0

    def visit_Assert(self, node):
        self.count += 1
        failure = ast.Raise(ast.Call(ast.Name("AssertionError", ast.Load()),
                                    [node.msg or ast.Constant("Required SDK check")], []), None)
        return ast.copy_location(ast.If(ast.UnaryOp(ast.Not(), node.test), [failure], []), node)


def load_recipe(name):
    path = ROOT / "scripts" / name
    expected, count = RECIPES[name]
    require(digest(path) == expected, "unchanged historical recipe: " + name)
    transform = UnconditionalChecks()
    tree = ast.fix_missing_locations(transform.visit(ast.parse(path.read_text())))
    require(transform.count == count and not any(isinstance(node, ast.Assert)
                                                for node in ast.walk(tree)),
            "all historical assertions become unconditional checks")
    module = types.ModuleType("const_function_sdk_recipe")
    module.__file__ = str(path)
    exec(compile(tree, str(path), "exec"), module.__dict__)
    return module


def validate(record):
    source = record["controller_source"]
    require(hashlib.sha256(source.encode()).hexdigest() == record["controller_sha256"],
            "retained execution controller identity")
    current = Path(__file__).read_text()
    if source != current:
        require(record["controller_sha256"]
                == "c4aa575a9628f969c35f187c4abd4d04c0a8135bba71b96bf4f00fccc912b29d",
                "only the exact completed execution recipe may be admitted")
        trees = [ast.parse(text) for text in (source, current)]
        for tree in trees:
            functions = [node for node in tree.body if isinstance(node, ast.FunctionDef)
                         and node.name == "validate"]
            require(len(functions) == 1, "one complete validation function")
            functions[0].body = [ast.Pass()]
        require(ast.dump(trees[0], include_attributes=False)
                == ast.dump(trees[1], include_attributes=False),
                "all execution, generation, ownership and hardening bodies are identical")
    baseline = bound(BASE, BASE_SHA)
    proposal = bound(PROPOSAL, PROPOSAL_SHA)
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["baseline_sha256"] == BASE_SHA
            and record["proposal_sha256"] == PROPOSAL_SHA
            and record["before"] == baseline["after"]
            and len(record["before"]) == len(record["after"]) == len(record["cases"]) == 80,
            "complete previous/current SDK epochs")
    for before, after, case in zip(record["before"], record["after"], record["cases"], strict=True):
        name = Path(before["manifest"]).name
        require(before["manifest"] == after["manifest"] == case["manifest"]
                and set(before["files"]) == set(after["files"])
                and case["changed_files"] == [name]
                and case["code_and_assist_identical"] is True
                and all(after["files"][key] == value for key, value in before["files"].items()
                        if key != name), "complete unchanged code and Assist")
        expected = copy.deepcopy(before["value"])
        change = proposal["paths"][PARSER]
        require(expected["generator_implementation_sha256"][PARSER] == change["before_sha256"],
                "exact preceding producer fingerprint")
        expected["generator_implementation_sha256"][PARSER] = change["after_sha256"]
        require(after["value"] == expected
                and hashlib.sha256(after["text"].encode()).hexdigest() == after["files"][name],
                "only the declared parser fingerprint changes")
        directory = (ROOT / after["manifest"]).parent
        require(all(digest(directory / key) == value for key, value in after["files"].items()),
                "actual complete retained SDK bytes")
    require(sum(len(row["files"]) - 1 for row in record["after"]) == 3560,
            "all 3560 retained code and guidance files")
    require(record["recipe_inputs"] == {name: value[0] for name, value in RECIPES.items()}
            and record["assertions_hardened"] == {name: value[1] for name, value in RECIPES.items()},
            "exact historical sources and unconditional transformations")
    require(len(record["actions"]) == 6 and all(type(row["exit_code"]) is int
            and row["exit_code"] == 0 for row in record["actions"])
            and all(type(row["exit_code"]) is int and row["exit_code"] == 0
                    for row in record["steps"]), "all reproduction processes passed")
    expected_actions = [
        ("reproduce_cpp_contract_fixtures.py", []),
        ("generate_examples.py", []),
        ("reproduce_cpp_independent_sdks_5_0_6.py", ["--refresh-provenance", "--record",
            str(EVIDENCE / "ap02_const_function_independent_sdks_5_0_6.json.gz")]),
        ("reproduce_cpp_contract_fixtures.py", ["--check"]),
        ("generate_examples.py", ["--check"]),
        ("reproduce_cpp_independent_sdks_5_0_6.py", ["--record",
            str(EVIDENCE / "ap02_const_function_independent_check_5_0_6.json.gz")]),
    ]
    require([(row["recipe"], row["args"]) for row in record["actions"]] == expected_actions,
            "all six exact refresh/check selections")
    counts = {}
    for row in record["steps"]:
        name = Path(row["command"][0]).name
        counts[name] = counts.get(name, 0) + 1
    require(counts == {"java": 160, "clang-format": 134, "mvn": 8, "javac": 4, "python3": 2},
            "all actual generation, formatting, packaging and prerequisite processes")
    expected_receipts = {"ap02_const_function_independent_sdks_5_0_6.json.gz",
                         "ap02_const_function_independent_check_5_0_6.json.gz"}
    require(set(record["independent_receipts"]) == expected_receipts
            and all(digest(EVIDENCE / name) == value
                    and read(EVIDENCE / name)["complete"] is True
                    for name, value in record["independent_receipts"].items()),
            "both completed independent SDK receipts")
    changed = {"runtime:" + row["manifest"] for row in record["after"]}
    before, after = record["input_hashes_before"], record["input_hashes_after"]
    require(set(before) == set(after)
            and {key for key in before if before[key] != after[key]} == changed
            and record["references_before"] == record["references_after"]
            and record["owned_directory_removed"] is True,
            "only eighty declared manifests change; source/reference/cleanup closure")


def execute(destination):
    require(not destination.exists(), "preserve previous receipts")
    require(shutil.disk_usage(ROOT).free > 4 * 1024 ** 3, "space for SDK reproduction")
    baseline = bound(BASE, BASE_SHA)
    proposal = bound(PROPOSAL, PROPOSAL_SHA)
    for name, item in proposal["paths"].items():
        require(digest(PRODUCER / name) == item["after_sha256"], "exact reviewed source")
    before = snapshot()
    require(before == baseline["after"], "all SDKs match the preceding accepted epoch")
    package = read(EVIDENCE / "ap02_primitive_const_path_installed_5_0_6.json.gz")
    maven = Path(package["steps"][0]["command"][0])
    require(maven.is_file(), "existing local Maven")
    record = dict(complete=False, whole_goal_complete=False, prior_turn_classification="progress",
                  scope="Regenerate/check eighty complete SDKs with unconditional historical checks; only parser provenance changes, no new runtime execution",
                  baseline_sha256=BASE_SHA, proposal_sha256=PROPOSAL_SHA, before=before,
                  input_hashes_before=capture_inputs(), references_before=references(),
                  recipe_inputs={name: value[0] for name, value in RECIPES.items()},
                  assertions_hardened={name: value[1] for name, value in RECIPES.items()},
                  controller_source=Path(__file__).read_text(), controller_sha256=digest(Path(__file__)),
                  actions=[], steps=[], cases=[])

    def save():
        destination.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))

    def run(command, *, cwd=None, text=False, capture_output=False, input=None,
            timeout=900, check=False, env=None):
        original = list(map(str, command))
        command = list(original)
        if command[0] == "mvn":
            command[0] = str(maven)
            command[1:1] = ["-o", "-Dexec.skip=true"]
        child = subprocess.Popen(command, cwd=cwd, env=env, text=text, stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
        started = time.monotonic()
        try:
            out, err = child.communicate(input=input, timeout=timeout)
        except BaseException:
            try:
                os.killpg(child.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            try:
                child.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(child.pid, signal.SIGKILL)
                child.communicate()
            raise
        record["steps"].append(dict(command=command, historical_command=original,
                                    cwd=str(cwd or ROOT), exit_code=child.returncode,
                                    stdout=out if text else out.decode(errors="replace"),
                                    stderr=err if text else err.decode(errors="replace"),
                                    duration_seconds=time.monotonic() - started))
        save()
        result = subprocess.CompletedProcess(command, child.returncode,
                                             out if capture_output else None,
                                             err if capture_output else None)
        if check:
            result.check_returncode()
        return result

    def check_output(command, **kwargs):
        return run(command, capture_output=True, check=True, **kwargs).stdout

    owned = None
    previous_tempdir, previous_argv = tempfile.tempdir, sys.argv
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_const_function_sdks_506_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            tempfile.tempdir = directory
            independent = EVIDENCE / "ap02_const_function_independent_sdks_5_0_6.json.gz"
            checked = EVIDENCE / "ap02_const_function_independent_check_5_0_6.json.gz"
            for name, args in (
                ("reproduce_cpp_contract_fixtures.py", []),
                ("generate_examples.py", []),
                ("reproduce_cpp_independent_sdks_5_0_6.py", ["--refresh-provenance", "--record", str(independent)]),
                ("reproduce_cpp_contract_fixtures.py", ["--check"]),
                ("generate_examples.py", ["--check"]),
                ("reproduce_cpp_independent_sdks_5_0_6.py", ["--record", str(checked)]),
            ):
                module = load_recipe(name)
                module.subprocess = types.SimpleNamespace(run=run, check_output=check_output)
                sys.argv = [str(ROOT / "scripts" / name), *args]
                print("Reproduce:", name, "check" if "--check" in args else "refresh", flush=True)
                result = module.main()
                require(result in (None, 0), "historical recipe succeeded")
                record["actions"].append(dict(recipe=name, args=args, exit_code=0))
                save()
            record["after"] = snapshot()
            for old, new in zip(before, record["after"], strict=True):
                record["cases"].append(dict(manifest=old["manifest"],
                    changed_files=[name for name, value in old["files"].items()
                                   if new["files"].get(name) != value],
                    code_and_assist_identical=all(new["files"].get(name) == value
                        for name, value in old["files"].items() if name != Path(old["manifest"]).name)))
            record["independent_receipts"] = {path.name: digest(path) for path in (independent, checked)}
            record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
        record.update(input_hashes_after=capture_inputs(), references_after=references(),
                      owned_directory_removed=not owned.exists(), complete=True)
        validate(record)
    except BaseException:
        record["complete"] = False
        record["error"] = traceback.format_exc()
        raise
    finally:
        tempfile.tempdir, sys.argv = previous_tempdir, previous_argv
        record["owned_directory_removed"] = owned is not None and not owned.exists()
        save()
    print("Eighty SDKs: 3560 unchanged code/Assist files; exactly one parser fingerprint per manifest", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    args = parser.parse_args()
    execute(args.record.resolve())


if __name__ == "__main__":
    main()
