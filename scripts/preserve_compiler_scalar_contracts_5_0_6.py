#!/usr/bin/env python3
"""Reproduce all 406 default SDKs with the scalar-aware Java producer."""

import argparse
from contextlib import contextmanager
import copy
from pathlib import Path

import preserve_compiler_model_contracts_5_0_6 as engine

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "docs/parity/compiler_scalar_storage_units_candidate_5_0_6.json.gz"
SOURCE_SHA = "ea7f7060a1f06477ebbd8b955ad68b5cd701c045ea47503575f116469a063925"


@contextmanager
def configured():
    """Change preparation labels only; keep the complete batch/check bodies."""
    engine.require(engine.sha(SOURCE.read_bytes()) == SOURCE_SHA, "immutable fresh Java preparation receipt")
    saved = engine.__file__, engine.SOURCE, engine.read
    original_read = engine.read

    def read(path):
        record = original_read(path)
        if path == SOURCE:
            record = copy.deepcopy(record)
            for step in record["steps"]:
                step["label"] = {"current_cpp_javac": "cpp_javac", "current_cpp_junit": "cpp_junit"}.get(step["label"], step["label"])
        return record

    engine.__file__, engine.SOURCE, engine.read = __file__, SOURCE, read
    try:
        yield
    finally:
        engine.__file__, engine.SOURCE, engine.read = saved


def validate(record):
    with configured():
        engine.validate(record)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--execute", type=Path)
    action.add_argument("--check", type=Path)
    action.add_argument("--controls", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    with configured():
        if args.execute:
            engine.execute(args.execute.resolve())
        elif args.check:
            engine.validate(engine.read(args.check))
            print("Verified 406 freshly generated default SDKs; previous compiler outcomes reused")
        else:
            engine.require(args.output is not None and not args.output.exists(), "fresh control output required")
            record = engine.controls(args.controls)
            engine.write(args.output, record)
            print("Verified default SDK corruptions:", len(record["controls"]), "optimized=", record["optimized"])


if __name__ == "__main__":
    main()
