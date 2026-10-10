#!/usr/bin/env python3
"""Check pilot export/lookup coverage; behavioral acceptance is independent."""
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def names(text):
    return set(re.findall(r"^using ::([^;]+);", text, re.M))


def main():
    ledger = json.loads((ROOT / "docs/module_exports.json").read_text())
    lookup = (ROOT / "modules/tests/lookup.cpp").read_text()
    total = 0
    imports = ledger["imports"]
    visiting = set()
    visited = set()

    def visit(package):
        if package in visiting:
            raise SystemExit(f"Cyclic production module imports at {package}")
        if package in visited:
            return
        visiting.add(package)
        for dependency in imports[package]:
            if dependency not in ledger["packages"]:
                raise SystemExit(f"Unknown module dependency: {dependency}")
            visit(dependency)
        visiting.remove(package)
        visited.add(package)

    for package, namespaces in ledger["packages"].items():
        interface = (ROOT / package / "src/lib.cppm").read_text()
        if re.findall(r"^export import (\w+);", interface, re.M) != imports[package]:
            raise SystemExit(f"Module imports differ from the reviewed graph: {package}")
        visit(package)
        if not re.search(rf"^export module {package};$", interface, re.M):
            raise SystemExit(f"Incorrect primary module name: {package}")
        expected = {namespace + "::" + name
                    for namespace, declarations in namespaces.items() for name in declarations}
        if names(interface) != expected:
            raise SystemExit(f"Pilot exports differ from the reviewed catalog: {package}")
        if package in {"teaql_memory", "teaql_async", "teaql_core"} and not expected <= names(lookup):
            raise SystemExit(f"Missing combined pilot lookup coverage: {package}")
        isolated = (ROOT / "modules/tests" / f"{package}_lookup.cpp").read_text()
        if names(isolated) != expected or re.findall(r"^import (\w+);", isolated, re.M) != [package]:
            raise SystemExit(f"Incorrect isolated-import coverage: {package}")
        for declarations in namespaces.values():
            for declaration in declarations.values():
                if not (ROOT / declaration["header"]).is_file():
                    raise SystemExit(f"Stale declaration locator: {declaration}")
        total += len(expected)
    if re.search(r"^#include [<\"]teaql/", lookup, re.M):
        raise SystemExit("Lookup consumers must not include project declaration headers")
    print(f"Verified {total} catalog exports and isolated lookup across {len(ledger['packages'])} modules")


if __name__ == "__main__":
    main()
