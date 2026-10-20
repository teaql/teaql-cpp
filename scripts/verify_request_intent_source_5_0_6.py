#!/usr/bin/env python3
"""Replay unchanged intent observations with exact reference dependencies."""

import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / "tests/parity"
sys.path.insert(0, str(PARITY))
from compare_values import observe, same
from generate_intent_trace import cases

PROJECT = PARITY / "rust_document_services_reference_5_0_6"
DRIVER = PROJECT / "src/bin/request_intent_reference_5_0_6.rs"
HISTORICAL_DRIVER = PARITY / "rust_field_layout/src/main.rs"
CORPUS = PARITY / "intent_trace_5_0_6_cases.json"
FUNCTIONS = ("data_type", "native_value", "mutation_envelope_probe",
             "request_envelope_probe", "request_intent_probe", "trace_tokens_probe")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def controls():
    # Complete Rust Unicode White_Space set, plus nearby non-whitespace states.
    spaces = list(range(9, 14)) + [0x20, 0x85, 0xA0, 0x1680]
    spaces += list(range(0x2000, 0x200B)) + [0x2028, 0x2029, 0x202F, 0x205F, 0x3000]
    assert len(spaces) == len(set(spaces)) == 25
    base = dict(kind="request_intent", comment="what", purpose="why",
                extra_query=True, extra_field="purpose")
    for code in spaces:
        whitespace = chr(code)
        yield dict(base, comment=whitespace)
        yield dict(base, purpose=whitespace)
        yield dict(base, comment=whitespace + "unchanged" + whitespace,
                   purpose=whitespace + "purpose" + whitespace)
    for code in (0, 0x1C, 0x180E, 0x200B, 0x200C, 0x200D, 0x2060, 0xFEFF):
        yield dict(base, comment=chr(code), purpose=chr(code))
    yield dict(base, comment="what\u0085\u2028\u2029next",
               purpose="why\u0085\u2028\u2029next")


def functions(text):
    result = {}
    for name in FUNCTIONS:
        found = re.findall(rf"(?ms)^fn {name}\(.*?(?=^fn |\Z)", text)
        if len(found) != 1:
            raise RuntimeError(f"Missing or duplicate historical function: {name}")
        result[name] = found[0].rstrip() + "\n"
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust", type=Path, required=True)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_modules_native")
    parser.add_argument("--record", type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error("Use a fresh record path; preserve previous evidence")
    record = dict(scope="Whole request_intent.rs model responsibility and existing envelope/token observations; not whole graph, error, executor or G17 acceptance",
                  complete=False, steps=[], observations={})

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + "\n").encode(), mtime=0))

    command = [sys.executable, str(ROOT / "scripts/verify_upstream.py")]
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
    record["steps"].append(dict(command=command, exit_code=result.returncode,
                                 stdout=result.stdout, stderr=result.stderr))
    save()
    result.check_returncode()
    baseline = json.loads((ROOT / "upstream.lock.json").read_text())["commit"]
    corpus = json.loads(CORPUS.read_text())
    if corpus["baseline"] != baseline:
        raise RuntimeError("Intent observations do not use the active runtime baseline")
    reference_lock = ROOT / ".reference/teaql-rs/Cargo.lock"
    if not registry_identities(PROJECT / "Cargo.lock") <= registry_identities(reference_lock):
        raise RuntimeError("Intent observer registry identities differ from the reference")
    historical = functions(HISTORICAL_DRIVER.read_text())
    isolated = functions(DRIVER.read_text())
    if historical != isolated:
        raise RuntimeError("The isolated observer changed a historical function body")
    record["preserved_observer_functions"] = {
        name:hashlib.sha256(value.encode()).hexdigest() for name, value in historical.items()}
    record["registry_identities"] = sorted(registry_identities(PROJECT / "Cargo.lock"))
    inputs = cases()
    if not same(inputs, [item["input"] for item in corpus["cases"]]):
        raise RuntimeError("Historical intent inputs changed")
    if len(inputs) != 4571:
        raise RuntimeError("Historical intent observation count changed")
    for source, expected_hash in corpus["source_sha256"].items():
        if digest(ROOT / ".reference/teaql-rs" / source) != expected_hash:
            raise RuntimeError(f"Historical intent source changed: {source}")
    extra = list(controls())
    if len(extra) != 84:
        raise RuntimeError("Complete whitespace control geometry changed")
    all_inputs = inputs + extra
    original = observe(str(args.rust.resolve()), all_inputs)
    if not same(original[:len(inputs)], [item["expected"] for item in corpus["cases"]]):
        raise RuntimeError("The exact-dependency original differs from historical observations")
    # These assertions are independent of C++ and preserve untrimmed valid text.
    for input, output in zip(extra, original[len(inputs):]):
        for kind in ("query", "mutation"):
            value = output[kind]
            if "comment" in value and value["comment"] != input["comment"]:
                raise RuntimeError("Original valid intent was unexpectedly trimmed")
        if "purpose" in output["query"] and output["query"]["purpose"] != input["purpose"]:
            raise RuntimeError("Original valid purpose was unexpectedly trimmed")
    record["baseline"] = baseline
    record["historical_count"] = len(inputs)
    record["additional_controls"] = [dict(input=input, expected=expected)
                                      for input, expected in zip(extra, original[len(inputs):])]
    record["observations"]["rust"] = dict(binary=str(args.rust.resolve()),
        sha256=digest(args.rust), values=original, count=len(original))
    save()
    for name, executable in (
            ("header", args.build_dir / "teaql_loaded_snapshot_probe"),
            ("module", args.build_dir / "modules/teaql_module_loaded_snapshot_probe")):
        actual = observe(str(executable.resolve()), all_inputs)
        differences = [dict(index=index, input=input, expected=expected, actual=value)
                       for index, (input, expected, value) in enumerate(zip(all_inputs, original, actual))
                       if not same(expected, value)]
        record["observations"][name] = dict(binary=str(executable.resolve()),
            sha256=digest(executable), values=actual, count=len(actual), differences=differences)
        save()
        if differences:
            raise RuntimeError(f"{name}: {len(differences)} intent observation differences")
    paths = [Path(__file__), DRIVER, HISTORICAL_DRIVER, CORPUS,
             PROJECT / "Cargo.toml", PROJECT / "Cargo.lock", reference_lock,
             ROOT / "teaql_core/include/teaql/core/request_intent.hpp",
             ROOT / "teaql_runtime/tests/request_trace_scope_tests.cpp"]
    paths += [ROOT / ".reference/teaql-rs" / source for source in corpus["source_sha256"]]
    record["hashes"] = {str(path.relative_to(ROOT)):digest(path) for path in paths}
    record["complete"] = True
    save()
    print(f"Original/header/module intent replay: {len(inputs)} unchanged observations + {len(extra)} controls each")


if __name__ == "__main__":
    main()
