#!/usr/bin/env python3
"""Reproduce retained SDKs after the strong Business ID compiler mapping change."""

import argparse
import ast
import copy
from pathlib import Path
import subprocess
import types

import reproduce_evaluated_generator_sdks_5_0_6 as preceding

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
BASE_COMMIT = "7d7c689f33927aab14bd8d0c93d79184a4065399"
BASE = ROOT / "docs/parity/compiler_scalar_storage_sdks_5_0_6.json.gz"
BASE_SHA = "345007068e127260f0977eda88d506adb4d027ace0cce1c506562b73d613c460"
CHANGED = "generator/src/main/java/com/skynet/codegenerator/cpp/CppCompilerModelConfiguration.java"


def load_recipe(name, *, source=None):
    path = ROOT / "scripts" / name
    current = path.read_text()
    if source is not None:
        preceding.require(source == current, "unchanged retained SDK recipe")
    before = subprocess.check_output(["git", "show", BASE_COMMIT + ":scripts/" + name], cwd=ROOT, text=True)
    preceding.require(ast.dump(ast.parse(current), include_attributes=False)
                      == ast.dump(ast.parse(before), include_attributes=False),
                      "every existing generation and verification body remains exact: " + name)
    hardener = preceding.UnconditionalChecks()
    executed = ast.fix_missing_locations(hardener.visit(ast.parse(current)))
    preceding.require(hardener.count == preceding.RECIPES[name]
                      and not any(isinstance(node, ast.Assert) for node in ast.walk(executed)),
                      "all preceding generation and verification checks remain unconditional")
    module = types.ModuleType("business_id_sdk_recipe")
    module.__file__ = str(path)
    exec(compile(executed, str(path), "exec"), module.__dict__)
    return module


def expected_manifest(before):
    result = copy.deepcopy(before["value"])
    inputs = result["generator_implementation_sha256"]
    preceding.require(CHANGED in inputs, "existing compiled model resolver fingerprint")
    inputs[CHANGED] = preceding.sha((PRODUCER / CHANGED).read_bytes())
    return result


def configure():
    preceding.__file__ = __file__
    preceding.BASE_COMMIT, preceding.BASE, preceding.BASE_SHA = BASE_COMMIT, BASE, BASE_SHA
    preceding.load_recipe, preceding.expected_manifest = load_recipe, expected_manifest


def validate(record):
    configure()
    preceding.validate(record)


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
        print("Verified eighty Business ID producer SDKs and unchanged code/Assist files")


if __name__ == "__main__":
    main()
