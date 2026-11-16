#!/usr/bin/env python3
"""Validate fresh SDK reproduction and its negative controls in either Python mode."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import sys

import preserve_compiler_model_contracts_5_0_6 as defaults
import reproduce_evaluated_generator_sdks_5_0_6 as retained
from generator_evaluation_source_admission_5_0_6 import RECORDS

ROOT = Path(__file__).resolve().parents[1]


def require(value, message):
    if not value:
        raise RuntimeError("Evaluation reproduction receipts: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(kind):
    name, digest = RECORDS[kind]
    path = ROOT / "docs/parity" / name
    data = path.read_bytes()
    require(sha(data) == digest, "exact fresh reproduction: " + name)
    return path, json.loads(gzip.decompress(data))


def controls():
    path, sdk = bound("sdk")
    default_path, default = bound("default")
    unit_path, _ = bound("unit")
    retained.validate(sdk)
    previous = defaults.SOURCE
    try:
        defaults.SOURCE = unit_path
        defaults.validate(default)
        inherited = defaults.controls(default_path)["controls"]
    finally:
        defaults.SOURCE = previous
    changes = (
        ("incomplete_sdk", lambda r: r.update(complete=False)),
        ("missing_sdk", lambda r: r["after"].pop()),
        ("wrong_sdk_file", lambda r: r["after"][0]["files"].update(invented="0" * 64)),
        ("wrong_model_input", lambda r: r["after"][0]["value"]["generator_implementation_sha256"].update({retained.MODEL_INPUTS[0]: "0" * 64})),
        ("coerced_resume", lambda r: r.update(resume_expected_provenance=1)),
        ("boolean_action", lambda r: r["actions"][0].update(exit_code=False)),
        ("wrong_recipe", lambda r: r["recipes"].update({"generate_examples.py": "pass\n"})),
        ("live_outputs", lambda r: r.update(owned_directory_removed=False)),
    )
    rejected = []
    for name, mutate in changes:
        changed = copy.deepcopy(sdk)
        mutate(changed)
        try:
            retained.validate(changed)
        except (RuntimeError, KeyError) as error:
            rejected.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted SDK reproduction corruption: " + name)
    return dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize,
                retained_controls=rejected, default_controls=inherited,
                retained_sha256=sha(path.read_bytes()), default_sha256=sha(default_path.read_bytes()),
                driver_source=Path(__file__).read_text())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    require(not args.output.exists(), "preserve corruption evidence")
    result = controls()
    args.output.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(), mtime=0))
    print("Verified retained/default SDK controls:", len(result["retained_controls"]),
          len(result["default_controls"]), "optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
