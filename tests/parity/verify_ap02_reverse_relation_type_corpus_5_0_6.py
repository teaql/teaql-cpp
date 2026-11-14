#!/usr/bin/env python3
"""Bind unique reverse-relation type cases to the locked-source execution."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
SOURCE = ROOT / "docs/parity/ap02_reverse_relation_type_composition_exploratory_5_0_6.json.gz"
CORPUS = (PRODUCER / "generator/src/test/resources/"
          "cpp_reverse_relation_type_composition_metadata_5_0_6.json.gz")
SOURCE_GATE = ROOT / "docs/parity/ap02_reverse_relation_type_installed_source_5_0_6.json.gz"
CALLERS = ROOT / "docs/parity/ap02_reverse_relation_type_compiled_callers_5_0_6.json.gz"
SDK = ROOT / "docs/parity/ap02_reverse_relation_type_independent_sdk_reproduction_5_0_6.json.gz"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def unchanged_inputs(record):
    require(record["input_hashes_before"] == record["input_hashes_after"]
            and record["inputs_unchanged"] is True, "Execution inputs changed")
    for path, expected in record["input_hashes_before"].items():
        require(digest(Path(path).read_bytes()) == expected,
                "Current execution input changed: " + path)


def removed_own_build(record):
    require(record["owned_directory_removed"] is True
            and not Path(record["owned_directory"]).exists(),
            "Task-owned execution build remains")


def validate(source, corpus, source_bytes):
    runtime = json.loads((ROOT / "upstream.lock.json").read_text())["commit"]
    generator = json.loads((PRODUCER / "cpp_generator.lock.json").read_text())[
        "generator_baseline_commit"]
    require(source["schema"] == corpus["schema"] == 1, "Source/corpus schema")
    require(source["runtime_commit"] == corpus["runtime_commit"] == runtime
            and source["generator_commit"] == corpus["generator_commit"] == generator,
            "Locked reference identities")
    require(source["case_count"] == corpus["source_case_count"] == 4000
            and source["difference_count"] == 0, "Complete matching source run")
    require(corpus["source_receipt"] == str(SOURCE.relative_to(ROOT))
            and corpus["source_receipt_sha256"] == digest(source_bytes),
            "Exact source receipt")
    require(source["owned_directory_removed"] is True
            and not Path(source["owned_directory"]).exists(), "Owned source build removed")
    for path, expected in source["source_hashes"].items():
        require(digest(Path(path).read_bytes()) == expected,
                "Source input changed: " + path)
    observed = source["cases"]
    require(len(observed) == 4000
            and len({row["name"] for row in observed}) == 4000,
            "Complete source result cardinality")
    encoded = "".join(json.dumps(row["input"], ensure_ascii=True) + "\n"
                      for row in observed).encode()
    require(source["input_sha256"] == digest(encoded), "Exact source input stream")
    first = {}
    for row in observed:
        require(row["original"] == row["current"],
                "Original/current metadata differs: " + row["name"])
        if row["input"] not in first:
            first[row["input"]] = {"name": row["name"], "input": row["input"],
                                   "expected": row["original"]}
        else:
            require(first[row["input"]]["expected"] == row["original"],
                    "Duplicate input has different source result")
    require(len(first) == corpus["unique_inputs"] == len(corpus["cases"]) == 2495,
            "Unique type-composition inputs")
    require(corpus["cases"] == list(first.values()),
            "Every retained case must be the first source observation")
    require(sum(bool(row["original"]["relations"]) for row in observed) == 1649
            and source["accepted_relations"] == 1649
            and source["earlier_boundary"] == 2351,
            "Accepted-relation and earlier-boundary partition")


def validate_execution(gate, callers, sdk, corpus):
    require(gate["complete"] is True and gate["full_goal_complete"] is False
            and gate["parser_overlay"] is None and gate["candidate_overlays"] == {}
            and gate["original_corpus_differences"] == gate["differences"] == []
            and gate["junit_passed"] is True and gate["junit_tests"] == 159
            and len(gate["cases"]) == 82680, "Complete installed source replay")
    require(gate["corpora"][0] == {
        "path": str(CORPUS), "sha256": digest(CORPUS.read_bytes()),
        "cases": 2495, "source_receipt": str(SOURCE.relative_to(ROOT)),
    }, "Installed corpus registration")
    for row, retained in zip(gate["cases"][:2495], corpus["cases"], strict=True):
        require(row["name"] == retained["name"] and row["input"] == retained["input"]
                and row["original"] == row["java"] == retained["expected"]
                and row["match"] is True, "Fresh locked-source corpus comparison")
    require(len(gate["steps"]) == 7
            and all(step["exit_code"] == 0 for step in gate["steps"]),
            "Original, Java and JUnit process results")
    unchanged_inputs(gate)
    removed_own_build(gate)

    require(callers["complete"] is True and callers["candidate_overlays"] == {}
            and callers["binding_redactions"] is False
            and callers["differences"] == []
            and [case["name"] for case in callers["cases"]]
            == ["type_composition_0086", "type_composition_0000"]
            and len(callers["steps"]) == 18
            and all(step["exit_code"] == 0 for step in callers["steps"]),
            "Complete original/C++ generated callers")
    for case in callers["cases"]:
        source_case = next(row for row in corpus["cases"] if row["name"] == case["name"])
        require(case["input"] == source_case["input"]
                and case["original"] == case["cpp"]
                and case["match"] is True and len(case["original"]) == 3,
                "Generated descriptor result and source input")
    require(sum(len(entity["policies"]) for case in callers["cases"]
                for entity in case["original"]) == 28,
            "Six descriptor and 28 field-policy observations")
    unchanged_inputs(callers)
    removed_own_build(callers)

    require(sdk["complete"] is True and sdk["refresh_provenance"] is False
            and len(sdk["cases"]) == 5
            and all(case["generated_bytes_identical"] is True
                    and case["changed_files"] == [] for case in sdk["cases"])
            and all(step["exit_code"] == 0 for step in sdk["steps"]),
            "Five unchanged independent generated SDKs")
    unchanged_inputs(sdk)
    removed_own_build(sdk)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(),
            "Preserve existing control receipt")
    source_bytes = SOURCE.read_bytes()
    source = read(SOURCE)
    corpus = read(CORPUS)
    validate(source, corpus, source_bytes)
    gate, callers, sdk = read(SOURCE_GATE), read(CALLERS), read(SDK)
    validate_execution(gate, callers, sdk, corpus)
    mutations = [
        ("changed_source_hash", lambda s, c: c.update(source_receipt_sha256="0" * 64)),
        ("missing_case", lambda s, c: c["cases"].pop()),
        ("changed_expected", lambda s, c: c["cases"][0]["expected"].update(limit="999")),
        ("changed_source_outcome", lambda s, c: s["cases"][0]["original"].update(limit="999")),
    ]
    controls = []
    for name, mutate in mutations:
        changed_source, changed_corpus = copy.deepcopy(source), copy.deepcopy(corpus)
        mutate(changed_source, changed_corpus)
        try:
            validate(changed_source, changed_corpus, source_bytes)
        except RuntimeError:
            controls.append(name)
            continue
        raise RuntimeError("Invalid corpus mutation accepted: " + name)
    for name, record, change in [
            ("incomplete_source_gate", gate, lambda row: row.update(complete=False)),
            ("altered_compiled_result", callers,
             lambda row: row["cases"][0].update(match=False))]:
        changed = copy.deepcopy(record)
        change(changed)
        try:
            validate_execution(changed if name == "incomplete_source_gate" else gate,
                               changed if name == "altered_compiled_result" else callers,
                               sdk, corpus)
        except RuntimeError:
            controls.append(name)
            continue
        raise RuntimeError("Invalid execution mutation accepted: " + name)
    if args.record is not None:
        result = {
            "complete": True, "whole_goal_complete": False,
            "runtime_commit": source["runtime_commit"],
            "generator_commit": source["generator_commit"],
            "source_sha256": digest(source_bytes),
            "corpus_sha256": digest(CORPUS.read_bytes()),
            "source_gate_sha256": digest(SOURCE_GATE.read_bytes()),
            "compiled_callers_sha256": digest(CALLERS.read_bytes()),
            "independent_sdk_sha256": digest(SDK.read_bytes()),
            "source_cases": 4000, "unique_inputs": 2495,
            "replayed_metadata_cases": 82680, "junit_tests": 159,
            "generated_caller_pairs": 2, "controls": controls,
        }
        args.record.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(),
                                              mtime=0))
    print("Reverse-relation types: 4,000 source results, 2,495 unique inputs, "
          "159 Java tests, two generated callers and six corruption controls pass")


if __name__ == "__main__":
    main()
