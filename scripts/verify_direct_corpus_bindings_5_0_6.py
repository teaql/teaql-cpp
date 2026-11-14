#!/usr/bin/env python3
"""Join every current direct comparator to its required source-contract fixture."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys

from inventory_parity_baselines import direct_corpus


ROOT = Path(__file__).resolve().parents[1]
RUNTIME = "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3"
GENERATOR = "45e1572c4956ca963410d8ea436ead3c8857087a"
ADAPTER = "tests/parity/compare_values.py"
ADAPTER_SHA256 = "f5d1cedd61db95d3d0aff17f25452754061c8a0e7043bfb03d0c20a98b76b651"
DEFAULT_CONTRACTS = {
    "parity_corpus_source_contracts": "corpus_source_contracts_5_0_6.json",
    "parity_remaining_corpus_source_contracts": "remaining_corpus_source_contracts_5_0_6.json",
    "parity_declared_core_driver_contracts": "declared_core_driver_contracts_5_0_6.json",
}
DEFAULT_EXECUTIONS = {
    "parity_remaining_corpus_source_contracts": "remaining_corpus_source_execution_5_0_6.json.gz",
    "parity_declared_core_driver_contracts": "declared_core_driver_execution_5_0_6.json.gz",
}


def require(condition, message):
    if not condition:
        raise RuntimeError("Direct corpus binding: " + message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == ".gz" else data)


def properties(test):
    return {row["name"]: row["value"] for row in test["properties"]}


def argument(command, name):
    if name not in command:
        return None
    require(command.count(name) == 1 and command.index(name) + 1 < len(command),
            "ambiguous or missing argument " + name)
    return command[command.index(name) + 1]


def collect(tests):
    state = {"tests": tests, "entries": [], "corpora": {}, "sources": {}, "executions": {}}
    for test in tests:
        scripts = [Path(part) for part in test["command"]
                   if part.endswith(".py") and Path(part).parent == ROOT / "tests/parity"]
        if not scripts:
            continue
        require(len(scripts) == 1, "ambiguous parity script " + test["name"])
        script = scripts[0]
        selected = direct_corpus(script)
        if selected is None:
            continue
        corpus = script.with_name(selected)
        path = corpus.relative_to(ROOT).as_posix()
        if path not in state["corpora"]:
            value = read(corpus)
            require(type(value.get("cases")) is list and bool(value["cases"]),
                    "nonempty complete corpus required: " + path)
            state["corpora"][path] = {
                "sha256": digest(corpus), "baseline": value.get("baseline"),
                "count": len(value["cases"]),
            }
        state["entries"].append({"test": test["name"], "corpus": path,
                                  "script": script.relative_to(ROOT).as_posix(),
                                  "script_sha256": digest(script)})
    needed = {fixture for entry in state["entries"]
              for test in tests if test["name"] == entry["test"]
              for fixture in properties(test).get("FIXTURES_REQUIRED", [])}
    for test in tests:
        if not needed.intersection(properties(test).get("FIXTURES_SETUP", [])):
            continue
        selected = argument(test["command"], "--contracts")
        if selected is None:
            name = DEFAULT_CONTRACTS.get(test["name"])
            if name is None:
                continue
            selected = ROOT / "tests/parity" / name
        path = Path(selected)
        execution = argument(test["command"], "--execution")
        if execution is None and test["name"] in DEFAULT_EXECUTIONS:
            execution = ROOT / "docs/parity" / DEFAULT_EXECUTIONS[test["name"]]
        state["sources"][test["name"]] = {
            "path": path.relative_to(ROOT).as_posix(), "sha256": digest(path),
            "registry": read(path),
            "execution": None if execution is None else Path(execution).relative_to(ROOT).as_posix(),
        }
    for source in state["sources"].values():
        executions = ([source["execution"]] if source["execution"] is not None else
                      [contract["receipt"] for contract in source["registry"]["contracts"]
                       if "receipt" in contract])
        for execution in executions:
            state["executions"][execution] = digest(ROOT / execution)
    return state


def validate(state):
    tests = {test["name"]: test for test in state["tests"]}
    require(len(tests) == len(state["tests"]), "duplicate CTest name")
    entries = state["entries"]
    require(len(entries) == len({entry["test"] for entry in entries}) == 305,
            "complete 305-entry direct cohort required")
    rows = []
    for entry in entries:
        test = tests[entry["test"]]
        command = test["command"]
        require(len(command) >= 4 and command[1] == str(ROOT / entry["script"])
                and command[2] == "--cpp"
                and command[4:] in ([], ["--cpp-emulator="], ["--cpp-emulator", ""]),
                "unqualified direct comparator selection: " + entry["test"])
        corpus = state["corpora"][entry["corpus"]]
        require(corpus["baseline"] == RUNTIME and type(corpus["count"]) is int
                and corpus["count"] > 0, "wrong corpus baseline or cardinality")
        required = properties(test).get("FIXTURES_REQUIRED", [])
        matches = []
        for source_name, source in state["sources"].items():
            setup = properties(tests[source_name]).get("FIXTURES_SETUP", [])
            fixtures = set(required).intersection(setup)
            if not fixtures:
                continue
            registry = source["registry"]
            require(registry["runtime_commit"] == RUNTIME
                    and registry["generator_commit"] == GENERATOR,
                    "wrong source-contract baseline")
            for contract in registry["contracts"]:
                if contract.get("corpus") != entry["corpus"]:
                    continue
                require(type(contract["count"]) is int and contract["count"] == corpus["count"],
                        "source/corpus cardinality mismatch")
                require(contract["native_test"] == test["name"]
                        and Path(command[3]).name == contract["native_target"],
                        "source/native test or executable mismatch")
                if "corpus_sha256" in contract:
                    require(contract["corpus_sha256"] == corpus["sha256"],
                            "source/corpus byte identity mismatch")
                require(len(fixtures) == 1, "ambiguous matching source fixture")
                selected = argument(tests[source_name]["command"], "--execution")
                if selected is not None:
                    selected = Path(selected).relative_to(ROOT).as_posix()
                elif source_name in DEFAULT_EXECUTIONS:
                    selected = "docs/parity/" + DEFAULT_EXECUTIONS[source_name]
                require(source["execution"] == selected, "source execution CLI mismatch")
                execution = selected or contract.get("receipt")
                require(execution is not None and execution in state["executions"],
                        "missing exact source execution selection")
                matches.append({
                    "fixture": next(iter(fixtures)), "source_test": source_name,
                    "contracts": source["path"], "contracts_sha256": source["sha256"],
                    "contract_name": contract["name"], "execution": execution,
                    "execution_sha256": state["executions"][execution],
                    "native_target": contract["native_target"],
                })
        require(len(matches) == 1, "unique required source contract missing: " + entry["test"])
        rows.append(dict(entry, count=corpus["count"], corpus_sha256=corpus["sha256"],
                         command=command, source=matches[0]))
    require(sum(row["count"] for row in rows) == 323568
            and len({row["source"]["source_test"] for row in rows}) == 27,
            "complete observation/source-owner cohort required")
    return rows


def controls(state, rows):
    results = []
    first = rows[0]
    for label in ("missing_entry", "duplicate_test", "missing_required_fixture",
                  "missing_source_setup", "wrong_corpus_baseline", "wrong_source_baseline",
                  "wrong_count", "wrong_binary", "wrong_execution_selection", "wrong_corpus_hash"):
        changed = copy.deepcopy(state)
        tests = {test["name"]: test for test in changed["tests"]}
        selected = tests[first["test"]]
        owner = tests[first["source"]["source_test"]]
        source = changed["sources"][owner["name"]]
        contract = next(row for row in source["registry"]["contracts"]
                        if row.get("corpus") == first["corpus"])
        if label == "missing_entry":
            changed["entries"].pop()
        elif label == "duplicate_test":
            changed["tests"].append(copy.deepcopy(selected))
        elif label == "missing_required_fixture":
            selected["properties"] = [row for row in selected["properties"]
                                      if row["name"] != "FIXTURES_REQUIRED"]
        elif label == "missing_source_setup":
            owner["properties"] = [row for row in owner["properties"]
                                   if row["name"] != "FIXTURES_SETUP"]
        elif label == "wrong_corpus_baseline":
            changed["corpora"][first["corpus"]]["baseline"] = "0" * 40
        elif label == "wrong_source_baseline":
            source["registry"]["runtime_commit"] = "0" * 40
        elif label == "wrong_count":
            contract["count"] -= 1
        elif label == "wrong_binary":
            selected["command"][3] = "/controlled/wrong_native_probe"
        elif label == "wrong_execution_selection":
            source["execution"] = None
            contract.pop("receipt", None)
        elif label == "wrong_corpus_hash":
            contract["corpus_sha256"] = "0" * 64
        try:
            validate(changed)
        except RuntimeError as error:
            results.append({"case": label, "rejected": True, "error": str(error)})
        else:
            raise RuntimeError("Direct corpus binding corruption accepted: " + label)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_native_current")
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "preserve binding records")
    require(digest(ROOT / ADAPTER) == ADAPTER_SHA256,
            "qualify changes to the shared type/framing/cardinality comparator")
    registry = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(args.build_dir.resolve()), "--show-only=json-v1"], text=True))
    state = collect(registry["tests"])
    rows = validate(state)
    results = controls(state, rows)
    if args.record:
        record = {"complete": True, "whole_goal_complete": False, "complete_sq02": False,
                  "scope": "Exact current direct-comparator/source-fixture join and structural controls; source executions need the separate fixture gate",
                  "runtime_commit": RUNTIME, "generator_commit": GENERATOR,
                  "optimized": sys.flags.optimize, "adapter_sha256": ADAPTER_SHA256,
                  "registered_tests": len(registry["tests"]), "rows": rows, "controls": results}
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print("305 direct comparators / 323568 observations / 27 required source fixtures; "
          "ten structural controls reject; source execution and complete SQ02 remain separate")


if __name__ == "__main__":
    main()
