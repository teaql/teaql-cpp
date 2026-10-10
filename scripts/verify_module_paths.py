#!/usr/bin/env python3
"""Verify workspace moves and unchanged production bodies against the move ledger."""
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def body_digest(text):
    stripped = re.sub(r"^\s*#\s*include[^\n]*", "", text, flags=re.M)
    return hashlib.sha256(re.sub(r"\s+", "", stripped).encode()).hexdigest()


def main():
    ledger = json.loads((ROOT / "docs/module_path_migration.json").read_text())
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    if ledger["baseline"] != lock["commit"]:
        raise SystemExit("Workspace move ledger must use the pinned baseline")
    destinations = set()
    production = 0
    for record in ledger["files"]:
        old, new = record["old"], record["new"]
        destination = (ROOT / new).resolve()
        if new in destinations or not destination.is_relative_to(ROOT):
            raise SystemExit(f"Duplicate or invalid destination: {new}")
        destinations.add(new)
        if (ROOT / old).exists() or not destination.is_file():
            raise SystemExit(f"Missing destination or retained original: {old} -> {new}")
        if any("-" in part for part in Path(new).parts):
            raise SystemExit(f"Owned destination uses a hyphen: {new}")
        if record["kind"] == "production":
            production += 1
            text = destination.read_text()
            if body_digest(text) != record["non_include_tokens_sha256"]:
                raise SystemExit(f"Production content changed beyond includes/formatting: {new}")
            content = re.sub(r"^[ \t]*#[ \t]*include[^\n]*(?:\n|$)", "", text, flags=re.M)
            if hashlib.sha256(content.encode()).hexdigest() != record["non_include_content_sha256"]:
                raise SystemExit(f"Production non-include bytes differ from the checkpoint: {new}")
    print(f"Verified {len(destinations)} moved files and {production} unchanged production bodies")


if __name__ == "__main__":
    main()
