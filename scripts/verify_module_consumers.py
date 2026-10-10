#!/usr/bin/env python3
"""Verify imported canonical contracts retain assertions and implementation bodies."""
import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def strip_prologues(text):
    return re.sub(r"^(?:#include|import) [^\n]+\n", "", text, flags=re.M)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    args = parser.parse_args()
    directory = args.build_dir / "modules/consumers"
    manifest = json.loads((directory / "manifest.json").read_text())
    for record in manifest:
        original = (ROOT / record["source"]).read_text()
        consumer = (directory / record["consumer"]).read_text()
        if re.search(r"^#include [<\"]teaql/", consumer, re.M):
            raise SystemExit(f"Consumer retains a project declaration include: {record['consumer']}")
        expected = strip_prologues(original)
        if record.get("entry"):
            consumer = consumer.split("// TEAQL canonical consumer begin\n", 1)[1]
        actual = strip_prologues(consumer)
        for name in record.get("qualified_calls", []):
            actual = re.sub(rf"::{name}(?=\s*\()", name, actual)
        if expected != actual:
            raise SystemExit(f"Canonical contract body changed: {record['source']}")
        if record["source"].endswith(".cpp"):
            if hashlib.sha256(expected.encode()).hexdigest() != record["body_sha256"]:
                raise SystemExit(f"Incorrect source-body fingerprint: {record['source']}")
    print(f"Verified {len(manifest)} canonical consumer/fixture bodies; only prologues and recorded qualifiers differ")


if __name__ == "__main__":
    main()
