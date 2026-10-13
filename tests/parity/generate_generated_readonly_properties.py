#!/usr/bin/env python3
"""Compare generated underscore-key property lookup with actual Rust 5.0.6."""

import argparse
import gzip
import json
import subprocess
from pathlib import Path

from compare_values import observe, same


ROOT = Path(__file__).resolve().parents[2]


def inputs():
    pool = [
        dict(kind="null"), dict(kind="typed_null"),
        dict(kind="json", value=None), dict(kind="json", value=False),
        dict(kind="json", value=0), dict(kind="json", value=""),
        dict(kind="json", value=[]), dict(kind="json", value={"nested": None}),
        dict(kind="bool", value=False), dict(kind="bool", value=True),
        dict(kind="i64", value="0"), dict(kind="i64", value="-9223372036854775808"),
        dict(kind="u64", value="18446744073709551615"),
        dict(kind="f64", value="-0.0"), dict(kind="decimal", value="1.20"),
        dict(kind="text", value=""), dict(kind="text", value="雪😀a\u0000b"),
        dict(kind="text", value="long" * 1024),
        dict(kind="list", value=[]), dict(kind="object", value={}),
        dict(kind="date", value="2024-02-29"), dict(kind="timestamp", value="-1"),
    ]
    keys = ["_", "__", "_total", "_雪", "_comment", "_dirty_fields",
            "plain", "#code", "", "雪", "id", "name", "version", "_missing"]
    yield dict(fields={}, keys=keys)
    for key in keys[:10]:
        for payload in pool:
            yield dict(fields={key: payload}, keys=keys)
    for payload in pool:
        yield dict(fields={key: payload for key in keys[:10]}, keys=keys)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust", required=True)
    parser.add_argument("--cpp", required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    subprocess.run(["python3", str(ROOT / "scripts/verify_upstream.py")], check=True)
    cases = list(inputs())
    original = observe(args.rust, cases)
    cpp = observe(args.cpp, cases)
    differences = [index for index, (a, b) in enumerate(zip(original, cpp)) if not same(a, b)]
    for index in differences[:3]:
        print(f"Case {index}: {cases[index]}\nRust: {original[index]}\nC++: {cpp[index]}")
    if differences:
        raise SystemExit(f"{len(differences)}/{len(cases)} readonly observations differ")
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    corpus = dict(baseline=lock["commit"], version=lock["version"],
                  note="Unchanged generated Rust School versus regenerated C++ Product; "
                       "model-independent readonly property protocol only. Complete values "
                       "and presence observations, no normalization.",
                  cases=[dict(input=entry, expected=result)
                         for entry, result in zip(cases, original)])
    target = Path(__file__).with_name("generated_readonly_properties_5_0_6_cases.json.gz")
    if args.check:
        with gzip.open(target, "rt", encoding="utf-8") as source:
            if not same(corpus, json.load(source)):
                raise SystemExit("Versioned readonly property corpus differs from live observations")
    else:
        target.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + "\n").encode(), mtime=0))
    print(f"Compared {len(cases)} actual generated readonly property observations")


if __name__ == "__main__":
    main()
