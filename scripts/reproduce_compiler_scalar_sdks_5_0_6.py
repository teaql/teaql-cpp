#!/usr/bin/env python3
"""Preserve all retained SDK bodies after adding compiler scalar storage facts."""

import argparse
import ast
import copy
from pathlib import Path
import subprocess
import types

import reproduce_evaluated_generator_sdks_5_0_6 as preceding

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
BASE_COMMIT = "a252c7ac60504366789af6c014c6ebdecebb1ea6"
BASE = ROOT / "docs/parity/generator_evaluation_sdks_current_5_0_6.json.gz"
BASE_SHA = "b68be40d0767ee1b77f18bf1eb7513e08eaa22d75e0a82c1b78396b42a239749"
HELPER = "generator/src/main/java/com/skynet/codegenerator/cpp/CppCompilerScalarStorage.java"
CHANGED = tuple("generator/src/main/java/com/skynet/codegenerator/cpp/" + name + ".java" for name in (
    "CppCoreGenerator", "CppModelConfiguration", "CppCompilerModelConfiguration"))


class RestoreInput(ast.NodeTransformer):
    def __init__(self):
        self.removed = 0

    def visit_List(self, node):
        self.generic_visit(node)
        kept = []
        for item in node.elts:
            if isinstance(item, ast.Constant) and item.value in (HELPER, "CppCompilerScalarStorage"):
                self.removed += 1
            else:
                kept.append(item)
        node.elts = kept
        return node

    visit_Tuple = visit_List


def load_recipe(name):
    path = ROOT / "scripts" / name
    current = path.read_text()
    before = subprocess.check_output(["git", "show", BASE_COMMIT + ":scripts/" + name], cwd=ROOT, text=True)
    restore = RestoreInput()
    tree = restore.visit(ast.parse(current))
    preceding.require(restore.removed == 1 and ast.dump(tree, include_attributes=False)
                      == ast.dump(ast.parse(before), include_attributes=False), "only scalar helper input added to recipe: " + name)
    hardener = preceding.UnconditionalChecks()
    executed = ast.fix_missing_locations(hardener.visit(ast.parse(current)))
    preceding.require(hardener.count == preceding.RECIPES[name]
                      and not any(isinstance(node, ast.Assert) for node in ast.walk(executed)), "all preceding generation/check bodies remain unconditional")
    module = types.ModuleType("scalar_sdk_recipe")
    module.__file__ = str(path)
    exec(compile(executed, str(path), "exec"), module.__dict__)
    return module


def expected_manifest(before):
    result = copy.deepcopy(before["value"])
    inputs = result["generator_implementation_sha256"]
    preceding.require(HELPER not in inputs and all(name in inputs for name in CHANGED), "three existing inputs and one new helper")
    for name in (*CHANGED, HELPER):
        inputs[name] = preceding.sha((PRODUCER / name).read_bytes())
    return result


def configure():
    # Reuse the complete six-action execution/validation engine and owned process handling.
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
        print("Verified all eighty scalar-producer SDKs and unchanged code/Assist files")


if __name__ == "__main__":
    main()
