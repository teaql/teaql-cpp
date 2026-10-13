#!/usr/bin/env python3
"""Qualify target identity, bootstrap and nonempty original audit provenance."""
import argparse
import gzip
import hashlib
import itertools
import json
import subprocess
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def inputs():
    identifiers = [dict(kind="absent"), dict(kind="null"), dict(kind="typed_null"),
        dict(kind="u64", value="0"), dict(kind="u64", value="1"),
        dict(kind="u64", value="18446744073709551615"), dict(kind="i64", value="-1"),
        dict(kind="i64", value="0"), dict(kind="i64", value="7"),
        dict(kind="i64", value="9223372036854775807"), dict(kind="text", value="7"),
        dict(kind="bool", value=True), dict(kind="f64", value="7.0")]
    return [dict(id=identifier, allow=allow, provenance=provenance, bootstrap=bootstrap)
        for identifier, allow, provenance, bootstrap in
        itertools.product(identifiers, (False, True), (False, True), (False, True))]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    parser.add_argument("--cpp", required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    subprocess.run(["python3", str(ROOT / "scripts/verify_upstream.py")], check=True)
    cases = inputs()
    original = observe(args.rust, cases)
    native = observe(args.cpp, cases)
    failures = [i for i, (a, b) in enumerate(zip(original, native)) if not same(a, b)]
    for i in failures[:3]:
        print(f"Case {i}: {cases[i]}\nRust: {original[i]}\nC++: {native[i]}")
    if failures:
        raise SystemExit(f"{len(failures)}/{len(cases)} audit projection observations differ")
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    sources = {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in
        (".reference/teaql-rs/teaql-runtime/src/event.rs",
         ".reference/teaql-rs/teaql-runtime/src/log_privacy.rs")}
    corpus = dict(baseline=lock["commit"], sources=sources,
        note="Unmodified actual original event/privacy modules compiled in a test-only crate; "
             "complete output without normalization. Nonempty invocation provenance, "
             "positive exact integer identities and bootstrap projection, not full governance parity.",
        cases=[dict(input=entry, expected=result) for entry, result in zip(cases, original)])
    target = Path(__file__).with_name("audit_event_projection_5_0_6_cases.json.gz")
    if args.check:
        with gzip.open(target, "rt", encoding="utf-8") as source:
            if not same(corpus, json.load(source)):
                raise SystemExit("Audit projection evidence differs from live original")
    else:
        target.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + "\n").encode(), mtime=0))
    print(f"Compared {len(cases)} original/native audit projection observations")


if __name__ == "__main__":
    main()
