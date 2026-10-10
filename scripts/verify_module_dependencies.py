#!/usr/bin/env python3
"""Reject cycles and upward dependencies in production header/module graphs."""
import argparse
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
PACKAGES = {
    "teaql_memory": set(),
    "teaql_async": {"teaql_memory"},
    "teaql_core": {"teaql_memory"},
    "teaql_data_service": {"teaql_core", "teaql_async"},
    "teaql_sql": {"teaql_data_service"},
    "teaql_runtime": {"teaql_sql"},
    "teaql_provider_sqlite": {"teaql_runtime"},
}


def topological(graph):
    active = []
    complete = set()
    order = []

    def visit(node):
        if node in active:
            cycle = active[active.index(node):] + [node]
            raise SystemExit("Production dependency cycle: " + " -> ".join(cycle))
        if node in complete:
            return
        active.append(node)
        for dependency in sorted(graph[node]):
            if dependency not in graph:
                raise SystemExit(f"Unknown production dependency: {node} -> {dependency}")
            visit(dependency)
        active.pop()
        complete.add(node)
        order.append(node)

    for node in sorted(graph):
        visit(node)
    return order


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="Write the reviewed graph as JSON")
    args = parser.parse_args()
    headers = {}
    owners = {}
    for package in PACKAGES:
        include = ROOT / package / "include"
        for path in sorted(include.rglob("*")):
            if path.suffix not in {".hpp", ".h"}:
                continue
            name = path.relative_to(include).as_posix()
            if name in headers:
                raise SystemExit(f"Duplicate production include path: {name}")
            headers[name] = path
            owners[name] = package
    graph = {}
    for name, path in headers.items():
        dependencies = set()
        for spelling in re.findall(r'^\s*#\s*include\s*[<"]([^>"]+)',
                                   path.read_text(), re.M):
            dependency = spelling
            if dependency not in headers:
                relative = (path.parent / spelling).resolve()
                matches = [key for key, value in headers.items() if value == relative]
                if matches:
                    dependency = matches[0]
                elif spelling.startswith("teaql/"):
                    raise SystemExit(f"Unresolved production include: {name} -> {spelling}")
                else:
                    continue
            dependencies.add(dependency)
        graph[name] = sorted(dependencies)
    order = topological(graph)

    def lower_layers(package):
        result = set()
        for dependency in PACKAGES[package]:
            result.add(dependency)
            result.update(lower_layers(dependency))
        return result

    for name, dependencies in graph.items():
        allowed = lower_layers(owners[name]) | {owners[name]}
        for dependency in dependencies:
            if owners[dependency] not in allowed:
                raise SystemExit(f"Upward production include: {name} -> {dependency}")
    modules = {}
    for package in PACKAGES:
        text = (ROOT / package / "src/lib.cppm").read_text()
        modules[package] = re.findall(r"^(?:export )?import (\w+);", text, re.M)
        if set(modules[package]) != PACKAGES[package]:
            raise SystemExit(f"Module imports differ from the supported layers: {package}")
    module_order = topological(modules)
    evidence = {
        "scope": "All literal production header includes (all conditional branches) and named imports",
        "headers": graph,
        "header_topological_order": order,
        "modules": modules,
        "module_topological_order": module_order,
        "header_count": len(graph),
        "header_edge_count": sum(map(len, graph.values())),
    }
    if args.output:
        args.output.write_text(json.dumps(evidence, indent=2) + "\n")
    print(f"Verified {len(graph)} production headers, {evidence['header_edge_count']} include "
          f"edges and {len(modules)} modules: acyclic and confined to supported layers")


if __name__ == "__main__":
    main()
