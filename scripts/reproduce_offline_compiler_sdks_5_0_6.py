#!/usr/bin/env python3
"""Reproduce retained SDKs after changing only the offline generation entry."""

import argparse
import copy
from pathlib import Path

import reproduce_compiler_business_id_sdks_5_0_6 as business
import reproduce_evaluated_generator_sdks_5_0_6 as preceding

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
BASE = ROOT / "docs/parity/compiler_business_id_storage_sdks_5_0_6.json.gz"
BASE_SHA = "f56c40f4312d0ffebfff689b3dbfa7c54d63b507371212ed0a34b9e292323dfd"
CHANGED = "generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java"
BEFORE_SHA = "5ef179eee07952337693a238fcf4df0da9300fac35e3be26f108d7f544f21fb8"


def expected_manifest(before):
    result = copy.deepcopy(before["value"])
    inputs = result["generator_implementation_sha256"]
    preceding.require(inputs.get(CHANGED) == BEFORE_SHA, "exact preceding offline-entry fingerprint")
    inputs[CHANGED] = preceding.sha((PRODUCER / CHANGED).read_bytes())
    return result


def configure():
    # Retain all six existing generation/check actions and unconditional checks.
    preceding.__file__ = __file__
    preceding.BASE_COMMIT = business.BASE_COMMIT
    preceding.BASE, preceding.BASE_SHA = BASE, BASE_SHA
    preceding.load_recipe = business.load_recipe
    preceding.expected_manifest = expected_manifest


def validate(record):
    configure()
    preceding.validate(record)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--execute", action="store_true")
    action.add_argument("--check", action="store_true")
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--resume-expected-provenance", action="store_true")
    args = parser.parse_args()
    configure()
    if args.execute:
        preceding.execute(args.record.resolve(), args.resume_expected_provenance)
    else:
        validate(preceding.read(args.record))
        print("Verified eighty offline-entry SDKs and unchanged code/Assist files")


if __name__ == "__main__":
    main()
