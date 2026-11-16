#!/usr/bin/env python3
"""Reproduce retained SDKs after restoring shared evaluation/model metadata."""

import argparse
import ast
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import types

from reproduce_compiler_projection_sdks_5_0_6 import UnconditionalChecks, snapshot
from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references
from verify_compiler_governed_execution_5_0_6 import maven_command
from verify_primitive_const_governed_execution_5_0_6 import run_owned

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
BASE_COMMIT = "5e5089bcbdcc6bc313bb793146a8aa4a41bc86ad"
BASE = ROOT / "docs/parity/compiler_model_configuration_sdks_5_0_6.json.gz"
BASE_SHA = "161f766f664807c11618e284cd2eb72bebd47c5bb66706f1e0c71dc2c96be469"
MODEL_INPUTS = tuple("model/src/main/java/com/skynet/codegenerator/" + name for name in (
    "meta/Domain.java", "meta/FieldDescriptor.java", "meta/ObjectDescriptor.java",
    "naming/FieldDescriptorNaming.java"))
RECIPES = {"reproduce_cpp_contract_fixtures.py": 45, "generate_examples.py": 0,
           "reproduce_cpp_independent_sdks_5_0_6.py": 9}


def require(value, message):
    if not value:
        raise RuntimeError("Evaluated generator SDK reproduction: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


class RestoreInputs(ast.NodeTransformer):
    def visit_List(self, node):
        self.generic_visit(node)
        node.elts = [item for item in node.elts if not (
            isinstance(item, ast.Constant) and item.value in MODEL_INPUTS)]
        return node

    visit_Tuple = visit_List


def load_recipe(name):
    current = (ROOT / "scripts" / name).read_text()
    before = subprocess.check_output(["git", "show", BASE_COMMIT + ":scripts/" + name], cwd=ROOT, text=True)
    tree = ast.parse(current)
    if name == "reproduce_cpp_contract_fixtures.py":
        target = next(node for node in tree.body if isinstance(node, ast.Assign)
                      and any(isinstance(item, ast.Name) and item.id == "METADATA_HELPER_INPUTS" for item in node.targets))
        require(isinstance(target.value, ast.BinOp) and isinstance(target.value.op, ast.Add)
                and ast.literal_eval(target.value.right) == MODEL_INPUTS, "four explicit shared model inputs")
        target.value = target.value.left
    elif name == "generate_examples.py":
        tree = RestoreInputs().visit(tree)
    else:
        assignment = next(node for node in tree.body if isinstance(node, ast.Assign)
                          and any(isinstance(item, ast.Name) and item.id == "MODEL_INPUTS" for item in node.targets))
        require(ast.literal_eval(assignment.value) == MODEL_INPUTS, "four explicit independent-SDK inputs")
        tree.body.remove(assignment)
        main = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "main")
        removals = []
        for node in ast.walk(main):
            for field, children in ast.iter_fields(node):
                if not isinstance(children, list):
                    continue
                retained = []
                for child in children:
                    source = ast.unparse(child) if isinstance(child, ast.AST) else ""
                    if (isinstance(child, ast.For) and ast.unparse(child.iter) == "MODEL_INPUTS"
                            or isinstance(child, ast.Expr) and source == "inputs.extend((PRODUCER / path for path in MODEL_INPUTS))"):
                        removals.append(source)
                    else:
                        retained.append(child)
                setattr(node, field, retained)
        require(len(removals) == 2, "only input inventory and provenance additions removed")
    require(ast.dump(tree, include_attributes=False) == ast.dump(ast.parse(before), include_attributes=False),
            "all preceding SDK generation and validation bodies remain exact: " + name)
    hardener = UnconditionalChecks()
    executed = ast.fix_missing_locations(hardener.visit(ast.parse(current)))
    require(hardener.count == RECIPES[name] and not any(isinstance(node, ast.Assert) for node in ast.walk(executed)),
            "all historical checks execute under optimized Python")
    module = types.ModuleType("evaluated_sdk_recipe")
    module.__file__ = str(ROOT / "scripts" / name)
    exec(compile(executed, module.__file__, "exec"), module.__dict__)
    return module


def expected_manifest(before):
    result = copy.deepcopy(before["value"])
    for source in MODEL_INPUTS:
        require(source not in result["generator_implementation_sha256"], "new metadata input was absent")
        result["generator_implementation_sha256"][source] = sha((PRODUCER / source).read_bytes())
    return result


def validate_preimage(before, observed, resume):
    require(len(before) == len(observed) == 80, "complete retained preimage inventory")
    for original, actual in zip(before, observed, strict=True):
        name = Path(original["manifest"]).name
        require(original["manifest"] == actual["manifest"]
                and original["files"].keys() == actual["files"].keys()
                and all(actual["files"][key] == value for key, value in original["files"].items() if key != name),
                "preexisting generated code/Assist bytes remain exact")
        require(actual["value"] == original["value"] or resume and actual["value"] == expected_manifest(original),
                "only explicitly resumed four-input provenance is permitted")


def execute(path, resume):
    require(not path.exists() and sha(BASE.read_bytes()) == BASE_SHA, "fresh receipt and exact preceding SDKs")
    before = read(BASE)["after"]
    observed = snapshot()
    validate_preimage(before, observed, resume)
    for name in RECIPES:
        load_recipe(name)
    record = dict(complete=False, whole_goal_complete=False, before=before, observed_before=observed,
                  resume_expected_provenance=resume, steps=[], actions=[],
                  baseline_sha256=BASE_SHA, baseline_commit=BASE_COMMIT,
                  driver_source=Path(__file__).read_text(), recipes={name: (ROOT / "scripts" / name).read_text() for name in RECIPES},
                  input_hashes_before=capture_inputs(), references_before=references(), independent_receipts={})

    def run(command, *, cwd=None, text=False, capture_output=False, input=None, check=False,
            env=None, timeout=900):
        source = list(map(str, command))
        actual = list(source)
        if actual[0] == "mvn":
            actual = [maven_command(), "-o", "-Dexec.skip=true", *actual[1:]]
        payload = input if text or input is None else input.decode()
        result = run_owned(actual, cwd=cwd, env=env, input=payload, text=True, capture_output=True, timeout=timeout)
        record["steps"].append(dict(command=actual, source_command=source, cwd=str(cwd or ROOT),
                                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        if check:
            require(result.returncode == 0, "actual generation/check failed; see " + str(path))
        return subprocess.CompletedProcess(actual, result.returncode,
            (result.stdout if text else result.stdout.encode()) if capture_output else None,
            (result.stderr if text else result.stderr.encode()) if capture_output else None)

    def check_output(command, **kwargs):
        return run(command, text=kwargs.pop("text", False), capture_output=True, check=True, **kwargs).stdout

    prior_tempdir, prior_argv = tempfile.tempdir, sys.argv
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_evaluated_generator_sdks_") as temporary:
            owned = Path(temporary)
            record["owned_directory"] = temporary
            tempfile.tempdir = temporary
            actions = (("reproduce_cpp_contract_fixtures.py", []), ("generate_examples.py", []),
                       ("reproduce_cpp_independent_sdks_5_0_6.py", ["--refresh-provenance", "--record", str(owned / "refresh.json.gz")]),
                       ("reproduce_cpp_contract_fixtures.py", ["--check"]), ("generate_examples.py", ["--check"]),
                       ("reproduce_cpp_independent_sdks_5_0_6.py", ["--record", str(owned / "check.json.gz")]))
            try:
                for name, arguments in actions:
                    module = load_recipe(name)
                    module.subprocess = types.SimpleNamespace(run=run, check_output=check_output)
                    sys.argv = [module.__file__, *arguments]
                    print(name, "check" if "--check" in arguments else "refresh", flush=True)
                    require(module.main() in (None, 0), "actual SDK recipe completed")
                    record["actions"].append(dict(recipe=name, arguments=arguments, exit_code=0))
                    if "--record" in arguments:
                        child = Path(arguments[arguments.index("--record") + 1])
                        record["independent_receipts"][child.name] = read(child)
                    write(path, record)
                record["after"] = snapshot()
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
        record.update(input_hashes_after=capture_inputs(), references_after=references(),
                      owned_directory_removed=not owned.exists(), complete=True)
        validate(record)
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        tempfile.tempdir, sys.argv = prior_tempdir, prior_argv
        if "owned_directory" in record:
            record["owned_directory_removed"] = not Path(record["owned_directory"]).exists()
        write(path, record)


def validate(record):
    require(sha(BASE.read_bytes()) == BASE_SHA and record["baseline_sha256"] == BASE_SHA
            and record["baseline_commit"] == BASE_COMMIT, "exact preceding SDK baseline")
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["owned_directory_removed"] is True
            and record["driver_source"] == Path(__file__).read_text(), "complete actual SDK reproduction")
    require(record["before"] == read(BASE)["after"] and len(record["after"]) == 80
            and record["after"] == snapshot(), "complete actual retained SDK membership")
    require(type(record["resume_expected_provenance"]) is bool, "explicit provenance resume mode")
    validate_preimage(record["before"], record["observed_before"], record["resume_expected_provenance"])
    for before, after in zip(record["before"], record["after"], strict=True):
        name = Path(before["manifest"]).name
        require(before["manifest"] == after["manifest"] and before["files"].keys() == after["files"].keys()
                and all(after["files"][key] == value for key, value in before["files"].items() if key != name),
                "all generated code/Assist bytes are unchanged")
        require(after["value"] == expected_manifest(before), "only four shared metadata source identities added")
    require(sum(len(row["files"]) - 1 for row in record["after"]) == 3560, "complete code/Assist file inventory")
    require(len(record["actions"]) == 6 and all(type(row["exit_code"]) is int and row["exit_code"] == 0
            for row in (*record["actions"], *record["steps"])), "all actual generation/check commands")
    allowed = {"runtime:" + row["manifest"] for row in record["after"]}
    before, after = record["input_hashes_before"], record["input_hashes_after"]
    require(all(before["runtime:" + row["manifest"]] == row["files"][Path(row["manifest"]).name]
                for row in record["observed_before"]), "actual preexisting manifest hashes")
    require(before.keys() == after.keys() and {key for key in before if before[key] != after[key]} <= allowed
            and record["references_before"] == record["references_after"], "only retained provenance changed during execution")
    for name in RECIPES:
        require(record["recipes"][name] == (ROOT / "scripts" / name).read_text(), "actual current recipe source")
        load_recipe(name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", type=Path, required=True)
    parser.add_argument("--resume-expected-provenance", action="store_true")
    args = parser.parse_args()
    execute(args.execute.resolve(), args.resume_expected_provenance)


if __name__ == "__main__":
    main()
