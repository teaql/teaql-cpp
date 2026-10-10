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
    # Keep the original move checkpoint intact. Only the specifically reviewed
    # extra C++ adapter can retire checkpoint destinations; no Rust layer is exempt.
    retirement = json.loads((ROOT / "docs/retired_compatibility.json").read_text())
    allowed_retirements = {
        "compatibility/teaql_legacy_sqlite/" + suffix for suffix in (
            "include/teaql/compatibility/legacy_sqlite_compiler.hpp",
            "include/teaql/compatibility/sqlite.hpp",
            "include/teaql/runtime/legacy_sqlite_compiler.hpp",
            "include/teaql/runtime/sqlite.hpp",
            "src/legacy_sqlite_compiler.cpp", "src/sqlite.cpp", "tests/sqlite_tests.cpp")}
    retired = {record["new"]: record for record in retirement["files"]}
    if (retirement["baseline"] != lock["commit"] or set(retired) != allowed_retirements or
            len(retired) != len(retirement["files"]) or
            retirement["review"] != "docs/sqlite_compatibility.md" or
            not (ROOT / retirement["review"]).is_file()):
        raise SystemExit("Invalid compatibility retirement review")
    structure_path = ROOT / "docs/module_structure_migration.json"
    structure = json.loads(structure_path.read_text()) if structure_path.exists() else {
        "baseline": lock["commit"], "relocations": []}
    if structure["baseline"] != lock["commit"]:
        raise SystemExit("Structural relocation ledger must use the pinned baseline")
    relocations = {record["source"]: record for record in structure["relocations"]}
    if len(relocations) != len(structure["relocations"]):
        raise SystemExit("Duplicate structural relocation source")
    relocated_destinations = [record["destination"] for record in relocations.values()]
    if len(set(relocated_destinations)) != len(relocated_destinations):
        raise SystemExit("Duplicate canonical relocation destination")
    used_relocations = set()
    destinations = set()
    production = 0
    used_retirements = set()
    for record in ledger["files"]:
        old, new = record["old"], record["new"]
        destination = (ROOT / new).resolve()
        if new in destinations or not destination.is_relative_to(ROOT):
            raise SystemExit(f"Duplicate or invalid destination: {new}")
        destinations.add(new)
        if new in retired:
            if record != retired[new] or destination.exists() or (ROOT / old).exists():
                raise SystemExit(f"Retirement changed a checkpoint or retained a file: {new}")
            used_retirements.add(new)
            continue
        if (ROOT / old).exists() or not destination.is_file():
            raise SystemExit(f"Missing destination or retained original: {old} -> {new}")
        if any("-" in part for part in Path(new).parts):
            raise SystemExit(f"Owned destination uses a hyphen: {new}")
        if record["kind"] == "production":
            production += 1
            checkpoint_separator = ""
            if new in relocations:
                relocation = relocations[new]
                used_relocations.add(new)
                for digest in ("non_include_tokens_sha256", "non_include_content_sha256"):
                    if relocation[digest] != record[digest]:
                        raise SystemExit(f"Relocation changed the original checkpoint: {new}")
                facade = re.sub(r"^[ \t]*#[ \t]*include[^\n]*(?:\n|$)", "",
                                destination.read_text(), flags=re.M)
                if facade.strip() != "#pragma once":
                    raise SystemExit(f"Relocation facade retains declarations or bodies: {new}")
                include_root = ROOT / Path(new).parts[0] / "include"
                destination = (ROOT / relocation["destination"]).resolve()
                if not destination.is_relative_to(include_root) or not destination.is_file():
                    raise SystemExit(f"Invalid canonical relocation destination: {destination}")
                spelling = destination.relative_to(include_root).as_posix()
                if f"#include <{spelling}>" not in (ROOT / new).read_text():
                    raise SystemExit(f"Relocation facade does not include its canonical body: {new}")
                # The old body was followed by an include block separated by one
                # blank line. clang-format removes that separator at the new EOF.
                checkpoint_separator = "\n"
            text = destination.read_text()
            if body_digest(text) != record["non_include_tokens_sha256"]:
                raise SystemExit(f"Production content changed beyond includes/formatting: {new}")
            content = re.sub(r"^[ \t]*#[ \t]*include[^\n]*(?:\n|$)", "", text, flags=re.M)
            content += checkpoint_separator
            if hashlib.sha256(content.encode()).hexdigest() != record["non_include_content_sha256"]:
                raise SystemExit(f"Production non-include bytes differ from the checkpoint: {new}")
    if used_relocations != set(relocations):
        raise SystemExit("Structural relocation does not match a production checkpoint")
    if used_retirements != set(retired):
        raise SystemExit("Retirement does not match the original move checkpoint")
    print(f"Verified {len(destinations) - len(retired)} retained moved files and {production} "
          f"unchanged canonical production contents ({len(used_relocations)} structural "
          f"relocations, {len(retired)} explicitly retired compatibility files)")


if __name__ == "__main__":
    main()
