#!/usr/bin/env python3
"""Regenerate retained SDK provenance after the compiled-column reader repair."""

import argparse
import ast
import copy
from pathlib import Path
import subprocess
import types

import reproduce_compiler_business_id_sdks_5_0_6 as business
import reproduce_evaluated_generator_sdks_5_0_6 as preceding

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
BASE = ROOT / "docs/parity/compiler_offline_inputs_sdks_5_0_6.json.gz"
BASE_SHA = "045e1c41dc6d8bf87fdda4fd177958ad3e3a2f2dee069c2b4d43f711ad5e5cdf"
CHANGED = "generator/src/main/java/com/skynet/codegenerator/cpp/CppCompilerModelConfiguration.java"
BEFORE_SHA = "f51cabdc5a7774a394293984fd9d0b70367d49ca486c39712f7568c24314bd22"
RUNTIME_PREVIOUS = "977166b874f4f29244621394229cc776e5c7c83c"


def expected_manifest(before):
    result = copy.deepcopy(before["value"])
    inputs = result["generator_implementation_sha256"]
    preceding.require(inputs.get(CHANGED) == BEFORE_SHA, "exact preceding compiled metadata resolver")
    inputs[CHANGED] = preceding.sha((PRODUCER / CHANGED).read_bytes())
    return result


def load_recipe(name, *, source=None):
    if name != "generate_examples.py":
        return business.load_recipe(name, source=source)
    path = ROOT / "scripts" / name
    current = path.read_text()
    before = subprocess.check_output(["git", "show", RUNTIME_PREVIOUS + ":scripts/" + name], cwd=ROOT, text=True)
    old = "str(ROOT / model), str(output)]"
    new = "str(ROOT / model), str(output), '--qualify-inheritance']"
    preceding.require(before.count(old) == 1 and before.replace(old, new, 1) == current
                      and (source is None or source == current), "one explicit development qualification route")
    hardener = preceding.UnconditionalChecks()
    tree = ast.fix_missing_locations(hardener.visit(ast.parse(current)))
    preceding.require(hardener.count == 0 and not any(isinstance(node, ast.Assert) for node in ast.walk(tree)),
                      "unchanged unconditional example recipe")
    module = types.ModuleType("compiled_column_example_recipe")
    module.__file__ = str(path)
    exec(compile(tree, str(path), "exec"), module.__dict__)
    return module


def validate(record):
    from text_alias_source_admission_5_0_6 import current_input_matches as text_matches, sdk_snapshot_matches

    require = preceding.require
    require(preceding.sha(BASE.read_bytes()) == BASE_SHA and record["baseline_sha256"] == BASE_SHA,
            "immutable preceding eighty-SDK receipt")
    require(record.get("complete") is True and record.get("whole_goal_complete") is False
            and record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
            and type(record["owned_bytes_removed"]) is int and record["owned_bytes_removed"] > 0,
            "finished scoped generation and measured cleanup")
    require(record["driver_source"] == Path(__file__).read_text()
            or text_matches(Path(__file__), preceding.sha(record["driver_source"].encode())), "actual SDK collector")
    before = preceding.read(BASE)["after"]
    require(record["before"] == before and type(record["resume_expected_provenance"]) is bool,
            "exact baseline and explicit provenance resume mode")
    after = record["after"]
    require(len(after) == len(before) == 80 and (after == preceding.snapshot()
            or sdk_snapshot_matches(after, preceding.snapshot())), "all current SDK paths and bytes")
    require(len(record["observed_before"]) == 80 and all(
        observed == old or record["resume_expected_provenance"] and observed == new
        for old, observed, new in zip(before, record["observed_before"], after, strict=True)),
        "only explicitly resumed, exact expected manifest updates precede this run")
    for old, new in zip(before, after, strict=True):
        manifest = Path(old["manifest"]).name
        require(old["manifest"] == new["manifest"] and old["files"].keys() == new["files"].keys()
                and all(new["files"][key] == value for key, value in old["files"].items() if key != manifest)
                and new["value"] == expected_manifest(old), "only the compiled reader provenance changes")
    expected_actions = [("reproduce_cpp_contract_fixtures.py", False), ("generate_examples.py", False),
        ("reproduce_cpp_independent_sdks_5_0_6.py", False), ("reproduce_cpp_contract_fixtures.py", True),
        ("generate_examples.py", True), ("reproduce_cpp_independent_sdks_5_0_6.py", False)]
    require([(row["recipe"], "--check" in row["arguments"]) for row in record["actions"]] == expected_actions
            and all(type(row["exit_code"]) is int and row["exit_code"] == 0
                    for row in [*record["actions"], *record["steps"]]), "actual generation and verification commands")
    require(record["references_before"] == record["references_after"], "unchanged locked references")
    old, new = record["input_hashes_before"], record["input_hashes_after"]
    require(old.keys() == new.keys(), "stable source inventory during generation")
    differences = {key for key in old if old[key] != new[key]}
    expected_differences = {"runtime:" + old["manifest"]
                            for old, new in zip(record["observed_before"], after, strict=True) if old != new}
    require(differences == expected_differences, "only the remaining generated manifest fingerprints change")
    require(sum(len(row["files"]) - 1 for row in after) == 3560, "all retained code and guidance files")


def configure():
    preceding.__file__ = __file__
    preceding.BASE_COMMIT = business.BASE_COMMIT
    preceding.BASE, preceding.BASE_SHA = BASE, BASE_SHA
    preceding.load_recipe = load_recipe
    preceding.expected_manifest = expected_manifest
    preceding.validate = validate


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--execute", action="store_true")
    action.add_argument("--check", action="store_true")
    parser.add_argument("--record", required=True, type=Path)
    parser.add_argument("--resume-expected-provenance", action="store_true")
    args = parser.parse_args()
    configure()
    if args.execute:
        preceding.execute(args.record.resolve(), args.resume_expected_provenance)
    else:
        validate(preceding.read(args.record))
    print("Eighty SDKs regenerated; 3,560 code/guidance files unchanged")


if __name__ == "__main__":
    main()
