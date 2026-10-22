#!/usr/bin/env python3
"""Check named-source coverage, not semantic acceptance, for resolved.rs."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
SOURCE = "teaql-runtime/src/data_service/resolved.rs"
DEFINITION = re.compile(
    r"^\s*(?:pub(?:\([^\n)]*\))?\s+)?(?:async\s+)?fn\s+(\w+)", re.M
)


def verify(inventory_path):
    subprocess.run([sys.executable, str(ROOT / "scripts/verify_upstream.py")], check=True)
    inventory = json.loads(inventory_path.read_text())
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    if inventory["baseline"] != lock["commit"] or inventory["source"] != SOURCE:
        raise ValueError("Inventory baseline/source differs from the active lock")
    raw = (ROOT / ".reference/teaql-rs" / SOURCE).read_bytes()
    if hashlib.sha256(raw).hexdigest() != inventory["source_sha256"]:
        raise ValueError("Original source hash differs from the inventory")
    source = raw.decode()
    actual = [
        (match.group(1), source.count("\n", 0, match.start(1)) + 1)
        for match in DEFINITION.finditer(source)
    ]
    declared = [(item["name"], item["line"]) for item in inventory["functions"]]
    if actual != declared or inventory["function_count"] != len(actual):
        raise ValueError("Function inventory is incomplete, reordered or stale")
    assignments = {}
    group_ids = set()
    for group in inventory["groups"]:
        if group["id"] in group_ids:
            raise ValueError("Duplicate responsibility group")
        group_ids.add(group["id"])
        for key in ("candidate_entry", "evidence_entry"):
            path = (ROOT / group[key]).resolve()
            if not path.is_relative_to(ROOT) or not path.is_file():
                raise ValueError(f"Missing or external {key}: {group[key]}")
        if not group["functions"]:
            raise ValueError("Empty responsibility group")
        for name in group["functions"]:
            if name in assignments:
                raise ValueError(f"Multiple primary groups for {name}")
            assignments[name] = group["id"]
    if len(assignments) != len(actual):
        raise ValueError("Responsibility assignments do not cover the source")
    for item in inventory["functions"]:
        if assignments.get(item["name"]) != item["group"]:
            raise ValueError(f"Missing or inconsistent group for {item['name']}")
    print(
        f"Verified {len(actual)} definitions in {len(group_ids)} responsibility groups; "
        "this is coverage evidence, not semantic acceptance"
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--inventory",
        type=Path,
        default=ROOT / "docs/parity/resolved_responsibility_inventory_5_0_6.json",
    )
    args = parser.parse_args()
    try:
        verify(args.inventory)
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Resolved inventory verification failed: {error}\n")


if __name__ == "__main__":
    main()
