#!/usr/bin/env python3
"""Inventory active native corpus baselines; this does not establish semantic parity."""

import argparse
import ast
import gzip
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / "tests/parity"


def shared_default_corpus():
    tree = ast.parse((PARITY / "compare_values.py").read_text())
    main = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "main")
    default = main.args.defaults[0]
    if not isinstance(default, ast.Constant) or not isinstance(default.value, str):
        raise RuntimeError("Shared corpus default must be an explicit source string")
    return default.value


def direct_corpus(script):
    """Resolve only explicit calls to the imported compare_values.main adapter."""
    if script.resolve() == (PARITY / "compare_values.py").resolve():
        return shared_default_corpus()
    tree = ast.parse(script.read_text())
    imported_names = {
        alias.asname or alias.name
        for node in ast.walk(tree)
        if isinstance(node, ast.ImportFrom) and node.module == "compare_values"
        for alias in node.names if alias.name == "main"
    }
    # A local main with the same name is a custom dispatcher, not this adapter.
    imported_names -= {
        node.name for node in ast.walk(tree)
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef))
    }
    matches = []
    for node in ast.walk(tree):
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
            if node.func.id in imported_names:
                if not node.args:
                    matches.append(shared_default_corpus())
                elif isinstance(node.args[0], ast.Constant) and isinstance(node.args[0].value, str):
                    matches.append(node.args[0].value)
    return matches[0] if len(matches) == 1 else None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--registry", type=Path, required=True,
                        help="JSON emitted by ctest --preset native --show-only=json-v1.")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--check", action="store_true",
                        help="Fail for historical/missing corpora or unresolved custom dispatchers.")
    args = parser.parse_args()
    registry = json.loads(args.registry.read_text())
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    tests = []
    cache = {}
    for test in registry["tests"]:
        command = test.get("command", [])
        scripts = [Path(part) for part in command if part.endswith(".py")]
        scripts = [script for script in scripts if script.parent == PARITY]
        if not scripts:
            continue
        if len(scripts) != 1:
            raise RuntimeError(f"Ambiguous comparator command: {test['name']}")
        script = scripts[0]
        corpus_name = direct_corpus(script)
        row = dict(test=test["name"], comparator=str(script.relative_to(ROOT)))
        if corpus_name is None:
            row.update(status="custom_dispatcher_requires_review")
        else:
            corpus = script.with_name(corpus_name)
            row["corpus"] = str(corpus.relative_to(ROOT))
            if not corpus.is_file():
                row.update(status="missing_corpus")
            else:
                if corpus not in cache:
                    opener = gzip.open if corpus.suffix == ".gz" else open
                    with opener(corpus, "rt", encoding="utf-8") as source:
                        data = json.load(source)
                    cases = data.get("cases")
                    cache[corpus] = dict(
                        baseline=data.get("baseline"),
                        observations=len(cases) if type(cases) is list else None,
                        valid_cases=type(cases) is list and bool(cases),
                        sha256=hashlib.sha256(corpus.read_bytes()).hexdigest(),
                    )
                row.update(cache[corpus])
                row["status"] = ("invalid_corpus_shape" if not row["valid_cases"] else
                                 "active_baseline" if row["baseline"] == lock["commit"] else
                                 "historical_baseline_requires_replay")
        tests.append(row)
    counts = {}
    for row in tests:
        counts[row["status"]] = counts.get(row["status"], 0) + 1
    report = dict(
        schema=1, runtime_commit=lock["commit"], profile="native",
        registered_tests=len(registry["tests"]),
        notice="Static comparator/corpus binding inventory only. An active baseline does not "
               "prove live source provenance, behavioral agreement or capability acceptance. "
               "Custom dispatchers are unresolved by this inventory and require review.",
        counts=counts, tests=tests,
    )
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(counts, sort_keys=True))
    if args.check and any(row["status"] != "active_baseline" for row in tests):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
