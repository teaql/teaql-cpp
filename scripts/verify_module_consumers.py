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


def canonical_body(text):
    # Paths change in copied fixtures; include position and macro context must
    # remain identical to the original test, rather than being stripped away.
    text = re.sub(r'^#include "[^\"]+"\n', '#include "canonical_fixture"\n', text, flags=re.M)
    return re.sub(r"^(?:#include <[^>]+>|import [^\n]+)\n", "", text, flags=re.M)


def module_executable_targets(build):
    reply = build.resolve() / ".cmake/api/v1/reply"
    indexes = sorted(reply.glob("index-*.json"))
    if not indexes:
        raise SystemExit("Module codemodel is missing; configure the modules_native preset")
    index = json.loads(indexes[-1].read_text())
    models = [row for row in index["objects"]
              if row["kind"] == "codemodel" and row["version"]["major"] == 2]
    if len(models) != 1:
        raise SystemExit("Expected one current module codemodel")
    model = json.loads((reply / models[0]["jsonFile"]).read_text())
    if (Path(model["paths"]["source"]).resolve() != ROOT
            or Path(model["paths"]["build"]).resolve() != build.resolve()
            or len(model["configurations"]) != 1):
        raise SystemExit("Module codemodel source, build or configuration changed")
    required = set()
    for reference in model["configurations"][0]["targets"]:
        target = json.loads((reply / reference["jsonFile"]).read_text())
        if target["type"] != "EXECUTABLE":
            continue
        imported = target["name"].startswith("teaql_module_") or any(
            item["define"] == "TEAQL_USE_MODULES=1"
            for group in target.get("compileGroups", []) for item in group.get("defines", []))
        if imported:
            required.add(target["name"])
    if not required:
        raise SystemExit("Module codemodel contains no imported executable targets")
    return required


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    args = parser.parse_args()
    contracts = json.loads((ROOT / "modules/contracts.json").read_text())
    presets = json.loads((ROOT / "CMakePresets.json").read_text())
    preset = next(row for row in presets["buildPresets"] if row["name"] == "modules_native")
    required = {"teaql_module_" + row["stem"] for row in contracts["contracts"]}
    required.update(module_executable_targets(args.build_dir))
    missing = required - set(preset["targets"])
    if missing:
        raise SystemExit("Module preset omits registered contracts: " + ", ".join(sorted(missing)))
    directory = args.build_dir / "modules/consumers"
    manifest = json.loads((directory / "manifest.json").read_text())
    for contract in contracts["contracts"]:
        entry = contract["stem"] + ".cpp"
        matches = [record for record in manifest
                   if record["consumer"] == entry and record.get("entry")]
        if len(matches) != 1:
            raise SystemExit(f"Missing or duplicate canonical entry view: {entry}")
        consumer = (directory / entry).read_text()
        prologue = consumer.split("// TEAQL canonical consumer begin\n", 1)[0]
        imports = re.findall(r"^import ([a-z_0-9]+);$", prologue, re.M)
        expected_import = contract.get("import_module", "teaql_provider_sqlite")
        if imports != [expected_import]:
            raise SystemExit(f"Incorrect contract import module: {entry}: {imports}")
    for record in manifest:
        original = (ROOT / record["source"]).read_text()
        consumer = (directory / record["consumer"]).read_text()
        if re.search(r"^#include [<\"]teaql/", consumer, re.M):
            raise SystemExit(f"Consumer retains a project declaration include: {record['consumer']}")
        for include in re.findall(r'^#include "([^\"]+)"', consumer, re.M):
            dependency = (directory / record["consumer"]).parent / include
            if not dependency.is_file():
                raise SystemExit(f"Consumer fixture include is missing: {record['consumer']}: {include}")
        expected = canonical_body(original)
        if record.get("entry"):
            consumer = consumer.split("// TEAQL canonical consumer begin\n", 1)[1]
        actual = canonical_body(consumer)
        for name in record.get("qualified_calls", []):
            actual = re.sub(rf"::{name}(?=\s*\()", name, actual)
        if expected != actual:
            raise SystemExit(f"Canonical contract body changed: {record['source']}")
        if record["source"].endswith(".cpp"):
            if hashlib.sha256(strip_prologues(original).encode()).hexdigest() != record["body_sha256"]:
                raise SystemExit(f"Incorrect source-body fingerprint: {record['source']}")
    print(f"Verified {len(manifest)} canonical consumer/fixture bodies; only prologues and recorded qualifiers differ")


if __name__ == "__main__":
    main()
