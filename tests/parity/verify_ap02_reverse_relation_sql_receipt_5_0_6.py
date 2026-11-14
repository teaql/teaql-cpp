#!/usr/bin/env python3
"""Check actual generated SQLite relation, E and JSON execution against Rust 5.0.6."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET

from compare_values import json_lines, same


ROOT = Path(__file__).resolve().parents[2]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EXECUTION = ROOT / "docs/parity/ap02_reverse_relation_sql_execution_5_0_6.json.gz"
CORPUS = (PRODUCER / "generator/src/test/resources/"
          "cpp_reverse_relation_type_composition_metadata_5_0_6.json.gz")
NAMES = ("self_one", "child_one", "child_many")
QUERY_NAMES = ("unselected", "loaded", "empty", "minimal", "no_parent")
PACKAGES = {"teaql-core", "teaql-macros", "teaql-runtime", "teaql-data-service",
            "teaql-sql", "teaql-provider-sqlite"}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate(record):
    runtime = json.loads((ROOT / "upstream.lock.json").read_text())["commit"]
    generator = json.loads((PRODUCER / "cpp_generator.lock.json").read_text())[
        "generator_baseline_commit"]
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["runtime_commit"] == runtime
            and record["generator_commit"] == generator
            and record["differences"] == [], "Complete bounded locked-source execution")
    require(record["input_hashes_before"] == record["input_hashes_after"]
            and record["inputs_unchanged"] is True
            and len(record["input_hashes_before"]) == 795, "Unchanged execution inputs")
    native_headers = sorted(path for package in ROOT.glob("teaql_*")
                            for suffix in ("*.hpp", "*.h")
                            for path in (package / "include").rglob(suffix))
    require(record["native_headers"] == [str(path.relative_to(ROOT)) for path in native_headers]
            and {str(path) for path in native_headers} <= record["input_hashes_before"].keys(),
            "Complete local runtime header inventory")
    java_sources = {str(path) for repository in (ROOT / ".reference/teaql_code_gen", PRODUCER)
                    for package in ("model", "generator")
                    for path in (repository / package / "src/main/java").rglob("*.java")}
    require(java_sources <= record["input_hashes_before"].keys(),
            "Complete original/current generator Java source inventory")
    require(digest(Path(record["native_compiler"]["path"]))
            == record["native_compiler"]["sha256"], "Actual native compiler identity")
    for name, expected in record["input_hashes_before"].items():
        require(digest(Path(name)) == expected, "Current execution input changed: " + name)
    require(record["owned_directory_removed"] is True
            and not Path(record["owned_directory"]).exists(), "Owned build removed")
    corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
    source = next(row for row in corpus["cases"] if row["name"] == "type_composition_0086")
    require(record["source_case"] == source
            and record["source_corpus_sha256"] == digest(CORPUS), "Retained Self source input")
    base = ('17, reverse_relation(name="links",target="Child",foreign_key="owner",'
            'json_type="Self"), audit_value_max_len=21')
    inputs = [base, base.replace('json_type="Self"', 'json_type="crate::Child"'),
              base.replace('json_type="Self"', 'many,json_type="crate::Child"')]
    steps = record["steps"]
    require([step["label"] for step in steps] == [
        "upstream", "source_build", "source_metadata", "original_classpath",
        "original_java_build", "cpp_classpath", "cpp_java_build", "java_metadata",
    ] + [name + suffix for name in NAMES for suffix in (
        "_original_generate", "_cpp_generate", "_cpp_format", "_native_build",
        "_original_resolve", "_original_build", "_original_run", "_native_run")]
        and all(step["exit_code"] == 0 for step in steps), "All 32 actual process steps")
    by_name = {step["label"]: step for step in steps}
    source_metadata = json_lines(by_name["source_metadata"]["stdout"])
    java_metadata = json_lines(by_name["java_metadata"]["stdout"])
    require(len(source_metadata) == len(java_metadata) == 3
            and [case["name"] for case in record["cases"]] == list(NAMES),
            "Complete source and model cardinality")
    for index, case in enumerate(record["cases"]):
        many = case["name"] == "child_many"
        require(case["input"] == inputs[index] and case["many"] is many,
                "Exact source-derived input and cardinality")
        metadata = {"parsed": True, "entity": "Probe", "table": "probe_data",
                    "service": None, "indexed": False, "limit": "21", "masks": [],
                    "declared": False, "relations": [{"name": "links", "target": "Child",
                        "local": "id", "foreign": "owner", "many": many, "typed": True}]}
        require(same(source_metadata[index], metadata)
                and same(java_metadata[index], metadata)
                and same(case["original_metadata"], metadata)
                and same(case["java_metadata"], metadata), "Actual parsed metadata effects")
        model = ET.fromstring(case["model"])
        require(model.attrib == {"name": "ap02_reverse_relation_consumer", "data_service": "sqlite"}
                and model.find("item").attrib["_audit_value_max_len"] == inputs[index]
                and model.find("child").attrib["owner"] == "integer()",
                "Source input reaches the generated model")
        require(len(case["original_files"]) == 26 and len(case["cpp_files"]) == 53,
                "Complete fresh generated artifact sets")
        require(by_name[case["name"] + "_native_build"]["command"][0]
                == record["native_compiler"]["path"], "Recorded compiler executes native build")
        resolved = json.loads(by_name[case["name"] + "_original_resolve"]["stdout"])
        for name in PACKAGES:
            package = next(item for item in resolved["packages"] if item["name"] == name)
            require(package["version"] == "5.0.6" and package["source"] is None
                    and package["manifest_path"] == str(ROOT / ".reference/teaql-rs" / name / "Cargo.toml"),
                    "Original runtime dependency source: " + name)
        require("--offline" in by_name[case["name"] + "_original_build"]["command"]
                and "--locked" in by_name[case["name"] + "_original_build"]["command"],
                "Locked offline original consumer build")
        original = json_lines(by_name[case["name"] + "_original_run"]["stdout"])
        native = json_lines(by_name[case["name"] + "_native_run"]["stdout"])
        require(len(original) == len(native) == len(case["results"]) == 5,
                "Complete five-query output cardinality")
        for query_index, row in enumerate(case["results"]):
            label = QUERY_NAMES[query_index]
            selected = label != "unselected"
            parent = 11 if label == "empty" else 99 if label == "no_parent" else 10
            query = {"name": label, "parent": parent, "selected": selected,
                     "minimal": label == "minimal", "many": many}
            value = original[query_index]
            require(row["input"] == query and row["match"] is True
                    and same(row["original"], value) and same(row["cpp"], native[query_index])
                    and same(value, native[query_index]), "Unmodified raw original/native result")
            require(value["error"] is None, "Generated SQLite query succeeded")
            require(len(value["sql"]) == (1 if label in ("unselected", "no_parent") else 2)
                    and [entry["count"] for entry in value["sql"]]
                    == ([0] if label == "no_parent" else [1] if label == "unselected"
                        else [1, 0 if label == "empty" else 2]),
                    "Parent/child queries without implicit E/JSON SQL")
            require(all(entry["comment"] == "Inspect injected reverse relation"
                        and entry["purpose"] == "Qualify typed relation loading"
                        for entry in value["sql"]), "SQL intent survives relation execution")
            if label == "no_parent":
                require(value["rows"] == [], "Missing parent is an empty list")
                continue
            require(len(value["rows"]) == 1, "One requested parent")
            entity = value["rows"][0]
            require(entity["id"] == parent and entity["name"] == ("empty" if parent == 11 else "parent")
                    and entity["loaded"] is selected and entity["graph_present"] is selected,
                    "Entity identity, E name and relation load state")
            expected_children = []
            if label in ("loaded", "minimal"):
                expected_children = [{"id": child, "secret": {"not_loaded": True}
                                      if label == "minimal" else {"value": secret}}
                                     for child, secret in ((20, "alpha"), (21, "beta"))[:2 if many else 1]]
            require(same(entity["children"], expected_children),
                    "Actual typed graph and E value/NotLoaded result")
            rendered = entity["json"]
            if not selected or case["name"] == "self_one":
                require("links" not in rendered, "Unselected or mismatched typed JSON is omitted")
            elif label == "empty":
                require(same(rendered["links"], [] if many else None), "Empty list versus NULL")
            else:
                children = rendered["links"] if many else [rendered["links"]]
                require([child["id"] for child in children] == [child["id"] for child in expected_children],
                        "JSON relation cardinality and ordered identities")
                if label == "minimal":
                    require(all(child == {"id": child["id"], "owner": 10} for child in children),
                            "Omitted scalar fields remain omitted in JSON")
                else:
                    require([child["secret"] for child in children]
                            == [child["secret"]["value"] for child in expected_children],
                            "Loaded scalar values reach JSON")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "Preserve prior control evidence")
    record = json.loads(gzip.decompress(EXECUTION.read_bytes()))
    validate(record)
    controls = []

    def reject(name, mutate):
        altered = copy.deepcopy(record)
        mutate(altered)
        try:
            validate(altered)
        except RuntimeError:
            controls.append(name)
            return
        raise RuntimeError("Corrupted execution accepted: " + name)

    reject("incomplete", lambda row: row.update(complete=False))
    reject("missing_model", lambda row: row["cases"].pop())
    reject("truncated_query_results", lambda row: row["cases"][0]["results"].pop())
    reject("changed_source_metadata", lambda row: row["cases"][0]["original_metadata"].update(limit="17"))
    reject("changed_native_result", lambda row: row["cases"][1]["results"][1]["cpp"]["rows"][0].update(loaded=False))
    reject("empty_native_output", lambda row: next(step for step in row["steps"]
        if step["label"] == "child_many_native_run").update(stdout=""))
    reject("changed_json_type_input", lambda row: row["cases"][0].update(input="17"))
    def change_hash(row):
        path = str(ROOT / "tests/parity/ap02_reverse_relation_sql_5_0_6.cpp")
        row["input_hashes_before"][path] = row["input_hashes_after"][path] = "0" * 64
    reject("changed_observer_hash", change_hash)
    def synchronize(row, mutate):
        case = row["cases"][1]
        for language in ("original", "cpp"):
            mutate(case["results"][1][language])
        for suffix in ("_original_run", "_native_run"):
            step = next(step for step in row["steps"] if step["label"] == "child_one" + suffix)
            values = json_lines(step["stdout"])
            mutate(values[1])
            step["stdout"] = "".join(json.dumps(value) + "\n" for value in values)
    reject("synchronized_wrong_load_state", lambda row: synchronize(
        row, lambda value: value["rows"][0].update(loaded=False)))
    reject("synchronized_implicit_sql", lambda row: synchronize(
        row, lambda value: value["sql"].append(copy.deepcopy(value["sql"][0]))))
    if args.record is not None:
        output = {"complete": True, "whole_goal_complete": False,
                  "execution_sha256": digest(EXECUTION), "verifier_sha256": digest(Path(__file__)),
                  "models": 3, "queries": 15, "processes": 32, "controls": controls}
        args.record.write_bytes(gzip.compress(json.dumps(output, sort_keys=True).encode(), mtime=0))
    print("Generated SQLite reverse relations: three models, 15 queries, E/JSON/SQL and ten controls pass")


if __name__ == "__main__":
    main()
