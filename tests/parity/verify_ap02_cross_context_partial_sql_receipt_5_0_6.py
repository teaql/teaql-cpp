#!/usr/bin/env python3
"""Verify generated SQLite and publication outcomes for the AP02 partial effect."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
from xml.sax.saxutils import escape

from compare_values import same
import verify_audit_cross_context_corpus_5_0_6 as source


ROOT = Path(__file__).resolve().parents[2]
MAIN = ROOT / "docs/parity/ap02_cross_context_partial_sql_5_0_6.json.gz"
SINKS = ROOT / "docs/parity/ap02_cross_context_partial_sql_sinks_5_0_6.json.gz"
WRAPPER = ROOT / "scripts/verify_audit_cross_context_sql_sinks_5_0_6.py"
BASE_MODEL = ROOT / "tests/generator/inheritance_graph.xml"
RETAINED_MODEL = ROOT / "tests/parity/inheritance_audit_mask_expression_5_0_6.xml"
CASE = "cross_malformed_match_arm_audit_value_max_len"
MODES = (
    "success", "write_failure", "sink_failure", "policy_denied", "stale_version",
    "policy_approved", "graph_success", "graph_policy_denied", "graph_policy_approved",
    "graph_write_failure", "graph_stale_child", "graph_stale_root",
)
SINK_MODES = (
    "unset", "invalid", "enabled", "no_debug_sink", "endpoint_off",
    "logging_disabled",
)
COMMITTED = {"success", "sink_failure", "policy_approved", "graph_success",
             "graph_policy_approved"}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def validate(main, sinks, *, current_sources):
    runtime = json.loads((ROOT / "upstream.lock.json").read_text())["commit"]
    generator = json.loads((ROOT.parent / "teaql_code_gen_cpp/cpp_generator.lock.json").read_text())[
        "generator_baseline_commit"]
    require(main.get("complete") is True and main.get("runtime_commit") == runtime
            and main.get("generator_commit") == generator, "Complete locked execution")
    require(main.get("mask_declaration") == "named" and main.get("save_field") == "name"
            and main.get("source_case", {}).get("name") == CASE,
            "Exact partial-effect model")
    require(main.get("cross_context_wrapper_sha256") == digest(WRAPPER)
            and main.get("cross_context_source_receipt_sha256") == digest(source.EXECUTION)
            and main.get("source_corpus_sha256") == digest(source.CORPUS),
            "Current wrapper and original-source receipt")
    expected_case = next(row for row in read(source.CORPUS)["cases"]
                         if row["name"] == CASE)
    require(same(main["source_case"], expected_case), "Actual locked-source case")
    original = 'advisor="specialist(optional=true)" _audit_mask_fields="name" />'
    baseline = BASE_MODEL.read_text()
    require(baseline.count(original) == 1, "One School declaration in baseline model")
    replacement = ('advisor="specialist(optional=true)" _audit_mask_fields="name"\n'
                   '            _audit_value_max_len="'
                   + escape(expected_case["input"], {'"': '&quot;'}) + '" />')
    derived_model = baseline.replace(original, replacement)
    require(hashlib.sha256(derived_model.encode()).hexdigest() == main["model_sha256"]
            and main["baseline_model_sha256"] == digest(BASE_MODEL)
            and main["retained_model_sha256"] == digest(RETAINED_MODEL)
            and main["school_model_metadata"]["audit_limit"] == expected_case["input"],
            "Exact generated model from the original partial-effect input")
    require(main.get("input_hashes_before") == main.get("input_hashes_after")
            and len(main["input_hashes_before"]) == 533
            and main.get("inputs_unchanged") is True
            and main.get("classpath_unchanged") is True,
            "Complete unchanged build-input epoch")
    if current_sources:
        require(all(Path(path).is_file() and digest(Path(path)) == value
                    for path, value in main["input_hashes_before"].items()),
                "Current build-input identity")
        require(all(Path(path).is_file() and digest(Path(path)) == value
                    for path, value in main["classpath_hashes"].items()),
                "Current Java dependency identity")
    require(main.get("owned_directory_removed") is True
            and not Path(main["owned_directory"]).exists()
            and main.get("owned_bytes_removed", 0) > 0,
            "Owned generation/build cleanup")
    require(len(main.get("original_generated_hashes", {})) == 44
            and len(main.get("cpp_generated_hashes", {})) == 95,
            "Complete generated file sets")
    require(main.get("metadata_match") is True
            and same(main["original_metadata"], main["cpp_metadata"]),
            "Actual generated descriptor equality")
    school = main["original_metadata"]["descriptors"][0]
    require(school["entity"] == "School" and school["limit"] == "13"
            and school["table"] == "school_data" and school["service"] == "sqlite"
            and school["masks"] == ["name"] and school["name_sql_policy"] == "Masked",
            "Partial length and retained School route/privacy")
    safe = main["original_metadata"]["safe_audit"][0]
    require(safe == {
        "name": "name", "value": "PR***...***RY", "masked": True, "truncated": True,
        "raw_length": 21, "output_length": 13, "mask_reason": "_audit_mask_fields",
        "truncate_reason": "_audit_value_max_len",
    }, "Generated descriptor applied to safe audit value")
    require(main.get("save_match") is True and main.get("save_cases") == 12
            and set(main["original_save"]) == set(main["cpp_save"]) == set(MODES)
            and same(main["original_save"], main["cpp_save"]),
            "Twelve generated original/native Save pairs")
    for mode in MODES:
        row = main["original_save"][mode]
        committed = mode in COMMITTED
        require(row["mode"] == mode and row["db_name"] == (
            "PRIVATE-SCHOOL-CANARY" if committed else "school")
            and row["event_count"] == (2 if mode in ("graph_success", "graph_policy_approved")
                                       else 1 if committed else 0)
            and same(row["safe_field"], safe if committed else None),
            "Authoritative row and safe committed event: " + mode)
    require(main.get("query_match") is True
            and same(main["original_query"], main["cpp_query"])
            and main["original_query"]["rows"] == [[7, 3, None]]
            and main["original_query"]["error"] is None
            and "PRIVATE-SCHOOL-CANARY" not in json.dumps(main["original_query"]["sql"])
            and "MASKED; NOT REPLAYABLE" in json.dumps(main["original_query"]["sql"]),
            "Generated SQLite query and masked diagnostics")
    steps = main.get("steps", [])
    required = {"upstream", "generator_ledger", "original_classpath", "original_compile",
                "original_generate", "cpp_classpath", "cpp_compile", "cpp_generate",
                "rust_build", "cpp_build", "cpp_metadata_build", "cpp_save_build",
                "rust_metadata", "cpp_metadata", "rust_query", "cpp_query", "sink_matrix"}
    required.update(f"{language}_save_{mode}"
                    for language in ("rust", "cpp") for mode in MODES)
    require(len(steps) == 41 and {step["label"] for step in steps} == required
            and all(step["exit_code"] == 0 for step in steps),
            "All source/generation/build/execution steps")
    by_label = {step["label"]: step for step in steps}
    for language, data in (("rust", main["original_save"]), ("cpp", main["cpp_save"])):
        require(same(json.loads(by_label[language + "_metadata"]["stdout"]),
                     main["original_metadata"] if language == "rust"
                     else main["cpp_metadata"]), "Raw generated metadata: " + language)
        require(same(json.loads(by_label[language + "_query"]["stdout"]),
                     main["original_query"] if language == "rust"
                     else main["cpp_query"]), "Raw generated query: " + language)
        for mode in MODES:
            require(same(json.loads(by_label[f"{language}_save_{mode}"]["stdout"]),
                         data[mode]), "Raw generated Save: " + language + "/" + mode)
    require(main.get("sink_cases") == 12 and main["sink_receipt"]["sha256"] == digest(SINKS)
            and sinks["runtime_commit"] == runtime
            and sinks["owned_directory_removed"] is True
            and len(sinks["observations"]) == 12,
            "Actual file-sink matrix and cleanup")
    require({(row["language"], row["case"]) for row in sinks["observations"]}
            == {(language, mode) for language in ("rust", "cpp") for mode in SINK_MODES},
            "All source/native sink configurations")
    for row in sinks["observations"]:
        require(row["exit_code"] == 0 and row["policy"] == "masked"
                and "PRIVATE-SCHOOL-CANARY" not in row["ordinary"]
                and ("MASKED; NOT REPLAYABLE" in row["ordinary"])
                is (row["case"] not in ("endpoint_off", "logging_disabled"))
                and ("PRIVATE-SCHOOL-CANARY" in row["debug"]) is
                (row["case"] == "enabled")
                and row["authorized_debug"] is (row["case"] == "enabled"),
                "Safe ordinary and authorized debug sink: " + row["language"]
                + "/" + row["case"])
    for mode in SINK_MODES:
        pair = [row for row in sinks["observations"] if row["case"] == mode]
        require(len(pair) == 2 and same(json.loads(pair[0]["stdout"]),
                                        json.loads(pair[1]["stdout"])),
                "Actual original/native sink query: " + mode)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    if args.record is not None:
        require(not args.record.exists(), "Fresh control receipt")
    source.validate(source.read(source.FIXTURE), source.read(source.EXECUTION),
                    source.read(source.CORPUS), source.read(source.CALLERS))
    main_record, sinks = read(MAIN), read(SINKS)
    validate(main_record, sinks, current_sources=True)
    controls = []

    def reject(name, changed_main=main_record, changed_sinks=sinks):
        try:
            validate(changed_main, changed_sinks, current_sources=False)
        except (RuntimeError, KeyError, TypeError, ValueError, IndexError):
            controls.append(name)
        else:
            raise RuntimeError("Corrupt AP02 SQL receipt accepted: " + name)

    reject("incomplete", changed_main=dict(main_record, complete=False))
    changed = copy.deepcopy(main_record)
    changed["original_metadata"]["descriptors"][0]["limit"] = "31"
    reject("wrong_limit", changed_main=changed)
    changed = copy.deepcopy(main_record)
    changed["original_save"]["success"]["safe_field"]["value"] = "PRIVATE-SCHOOL-CANARY"
    reject("raw_save_event", changed_main=changed)
    changed = copy.deepcopy(main_record)
    changed["steps"] = changed["steps"][:-1]
    reject("truncated_steps", changed_main=changed)
    changed = copy.deepcopy(sinks)
    changed["observations"][0]["ordinary"] += "PRIVATE-SCHOOL-CANARY"
    reject("plain_ordinary_log", changed_sinks=changed)
    require(len(controls) == 5, "Five effective receipt corruption controls")
    if args.record is not None:
        record = {
            "complete": True, "whole_goal_complete": False,
            "scope": "One source-bound generated partial-effect SQLite/Safe-SQL execution",
            "source_receipt_sha256": digest(source.EXECUTION),
            "source_corpus_sha256": digest(source.CORPUS),
            "main_sha256": digest(MAIN), "sinks_sha256": digest(SINKS),
            "verifier_sha256": digest(Path(__file__)), "wrapper_sha256": digest(WRAPPER),
            "save_pairs": 12, "sink_cases": 12, "controls": controls,
        }
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(),
                                              mtime=0))
    print("1/1 source-bound generated model, 12/12 Save pairs, 12/12 sink cases, 5/5 controls")


if __name__ == "__main__":
    main()
