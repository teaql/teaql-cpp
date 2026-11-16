#!/usr/bin/env python3
"""Execute qualified constant-function projections through generated SQLite APIs."""

import argparse
import ast
import gzip
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import types

from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import same

EVIDENCE = ROOT / "docs/parity"
BASE = ROOT / "scripts/verify_ap02_reverse_relation_sql_5_0_6.py"
BASE_SHA = "179b50cb8c72592d95c213d10eabb1fe859845226aeed66900fb0e432945cb6a"
SOURCE = EVIDENCE / "ap02_const_function_repaired_generated_5_0_6.json.gz"
SOURCE_SHA = "e6d6873b686877e83290892d7a99606bc2ca8388bece23edf29e2aae28a444e5"
NAMES = ("core_size_of", "core_align_of", "fn_parameter", "fn_recursive",
         "body_function_over_parameter", "generic_item_forward",
         "function_item_as_value_value", "argument_return")


def require(value, message):
    if not value:
        raise RuntimeError("Constant-function SQLite: " + message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def selected_cases():
    require(digest(SOURCE) == SOURCE_SHA, "immutable actual complete compiler pairs")
    source = read(SOURCE)
    require(source["complete"] is True and source["parity_passed"] is True,
            "qualified function source expressions")
    by_name = {row["name"]: row for row in source["cases"]}
    cases = []
    for name in NAMES:
        row = by_name[name]
        require(row["original_exit_code"] == row["cpp_exit_code"] == 0
                and row["original_projection_count"] == 1, "actual accepted function witness")
        expression = row["expression"]
        projected = "<[crate::child::Child; " + expression + "] as IntoIterator>::Item"
        attribute = ('17, reverse_relation(name="links",target="Child",'
                     'foreign_key="owner",many,json_type="' + projected + '"), audit_value_max_len=21')
        cases.append(dict(name=name, many=True, input=attribute,
                          source_expression=expression, source_type=row["type"], derived_type=projected))
    return cases


def validate(record):
    cases = selected_cases()
    source = record["recipe_source"]
    require(hashlib.sha256(source.encode()).hexdigest() == record["recipe_sha256"],
            "retained execution controller identity")
    current = Path(__file__).read_text()
    if source != current:
        require(record["recipe_sha256"]
                == "81cd1630132be453e99d8d6178cd9634a0fa9b9596af11fa447ca40720c9bb82",
                "only the exact completed SQL recipe may be admitted")
        trees = [ast.parse(text) for text in (source, current)]
        for tree in trees:
            functions = [node for node in tree.body if isinstance(node, ast.FunctionDef)
                         and node.name == "validate"]
            require(len(functions) == 1, "one complete admission function")
            functions[0].body = [ast.Pass()]
        require(ast.dump(trees[0], include_attributes=False)
                == ast.dump(trees[1], include_attributes=False),
                "all generation, model, compiler, execution and ownership bodies are identical")
    require(digest(BASE) == BASE_SHA, "unchanged complete source execution recipe")
    from xml.sax.saxutils import escape
    functions = [node for node in ast.parse(BASE.read_text()).body
                 if isinstance(node, ast.FunctionDef) and node.name == "model"]
    require(len(functions) == 1, "one original model derivation")
    model_scope = {"escape": escape}
    exec(compile(ast.Module(body=functions, type_ignores=[]), str(BASE), "exec"), model_scope)
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["const_function_source_sha256"] == SOURCE_SHA
            and record["historical_recipe_sha256"] == BASE_SHA
            and record["differences"] == []
            and len(record["cases"]) == len(cases) == 8,
            "eight complete actual generated runtime consumers")
    for actual, expected in zip(record["cases"], cases, strict=True):
        require(all(actual[key] == value for key, value in expected.items()),
                "exact source-derived configured model")
        require(actual["model"] == model_scope["model"](expected["input"])
                and same(actual["original_metadata"], actual["java_metadata"]),
                "exact complete model and original/current metadata effects")
        require(len(actual["results"]) == 5
                and [row["input"]["name"] for row in actual["results"]]
                    == ["unselected", "loaded", "empty", "minimal", "no_parent"]
                and all(row["match"] is True and same(row["original"], row["cpp"])
                        for row in actual["results"]), "query, relation, E, JSON and SQL outcomes")
    require(all(type(row["exit_code"]) is int and row["exit_code"] == 0
                for row in record["steps"]), "every actual compiler and executable succeeded")
    labels = [row["label"] for row in record["steps"]]
    require(len(labels) == len(set(labels))
            and sum(label.endswith("_native_build") for label in labels) == 8
            and sum(label.endswith("_original_build") for label in labels) == 8
            and sum(label.endswith("_native_run") for label in labels) == 8
            and sum(label.endswith("_original_run") for label in labels) == 8,
            "honest native/original compilation and execution membership")
    require(record["input_hashes_before"] == record["input_hashes_after"]
            and record["inputs_unchanged"] is True
            and record["global_inputs_before"] == record["global_inputs_after"]
            and record["references_before"] == record["references_after"]
            and record["owned_directory_removed"] is True,
            "stable source/reference frame and complete owned output cleanup")
    require(all(Path(name).is_file() and digest(Path(name)) == value
                for name, value in record["input_hashes_after"].items()),
            "actual unchanged compilation inputs")


def execute(destination):
    require(not destination.exists(), "preserve prior execution receipts")
    require(digest(BASE) == BASE_SHA, "unchanged historical execution recipe")
    tree = ast.parse(BASE.read_text())
    require(not any(isinstance(node, ast.Assert) for node in ast.walk(tree)),
            "execution recipe checks are unconditional")
    selected = selected_cases()
    module = types.ModuleType("const_function_relation_sql")
    module.__file__ = str(BASE)
    exec(compile(tree, str(BASE), "exec"), module.__dict__)
    runtime = json.loads((ROOT / "upstream.lock.json").read_text())["commit"]
    producer = ROOT.parent / "teaql_code_gen_cpp"
    generator = json.loads((producer / "cpp_generator.lock.json").read_text())["generator_baseline_commit"]
    module.selected_cases = lambda: (
        dict(runtime_commit=runtime, generator_commit=generator),
        dict(source_sha256=SOURCE_SHA, selected_names=list(NAMES)), selected)
    module.CORPUS = SOURCE
    package = read(EVIDENCE / "ap02_primitive_const_path_installed_5_0_6.json.gz")
    maven = package["steps"][0]["command"][0]

    def run(command, *, cwd=None, env=None, input=None, text=False,
            capture_output=False, timeout=None, check=False):
        command = list(map(str, command))
        if command[0] == "mvn":
            command[0] = maven
        child = subprocess.Popen(command, cwd=cwd, env=env, text=text, stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
        try:
            out, err = child.communicate(input=input, timeout=timeout or 900)
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
        result = subprocess.CompletedProcess(command, child.returncode, out, err)
        if check:
            result.check_returncode()
        return result

    module.subprocess = types.SimpleNamespace(run=run, check_output=subprocess.check_output)
    before, ref = capture_inputs(), references()
    previous_argv = sys.argv
    error = None
    try:
        sys.argv = [str(BASE), "--record", str(destination)]
        require(module.main() == 0, "complete fresh generated runtime execution")
    except BaseException as caught:
        error = caught
        raise
    finally:
        sys.argv = previous_argv
        if destination.exists():
            record = read(destination)
            record.update(const_function_source_sha256=SOURCE_SHA, historical_recipe_sha256=BASE_SHA,
                          recipe_source=Path(__file__).read_text(), recipe_sha256=digest(Path(__file__)),
                          global_inputs_before=before, global_inputs_after=capture_inputs(),
                          references_before=ref, references_after=references(),
                          actual_process_launcher="Exact local Maven path; owned session children are stopped/reaped before output cleanup")
            record["complete"] = record.get("complete") is True and error is None
            destination.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    validate(read(destination))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", required=True, type=Path)
    execute(parser.parse_args().record.resolve())


if __name__ == "__main__":
    main()
