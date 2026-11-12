#!/usr/bin/env python3
"""Check current-source provenance and protected generated Credential outcomes."""

import argparse
import copy
import gzip
import hashlib
import json
from functools import cache
from pathlib import Path
import subprocess

from compare_values import same


ROOT = Path(__file__).resolve().parents[2]
MAIN = ROOT / "docs/parity/generated_audit_credential_mutation_sinks_current_v2_5_0_6.json.gz"
QUERY = ROOT / "docs/parity/generated_audit_credential_mutation_query_matrix_v2_5_0_6.json.gz"
AP02_SOURCE = ROOT / "docs/parity/ap02_index_consumer_installed_source_5_0_6.json.gz"
AP02_SQL = ROOT / "docs/parity/ap02_index_consumer_sql_5_0_6.json.gz"
AP02_SOURCE_SHA256 = "c9277f1238c4ae6676b132b5901b45fdb41a2f2feec5bd2bb7cd8fe67910857e"
AP02_SQL_SHA256 = "d17e4a2ddf3fc83698ca7e96c51e14086b50d02a1607a3bf966105f586e6fd09"
AUDIT_SOURCE = str(ROOT / "scripts/verify_generated_audit_sql_sinks_5_0_6.py")
PRIVACY_SOURCE = str(ROOT / "tests/parity/verify_dynamic_json_privacy_sinks_5_0_6.py")
PARSER_SOURCE = str(ROOT.parent / "teaql_code_gen_cpp/generator/src/main/java/"
                    "com/skynet/codegenerator/cpp/CppRustLiteralMetadata.java")
OLD_CORPUS = str(ROOT.parent / "teaql_code_gen_cpp/generator/src/test/resources/"
                 "cpp_nested_item_checkpoint_metadata_5_0_6.json.gz")
NEW_CORPUS = str(ROOT.parent / "teaql_code_gen_cpp/generator/src/test/resources/"
                 "cpp_index_parsebuffer_consumer_metadata_5_0_6.json.gz")
PRIVACY_CONTROL = ROOT / "tests/parity/verify_dynamic_json_privacy_sink_controls_5_0_6.py"
PRIVACY_CONTROL_SHA256 = "a955894861f37bcec3f9ec7b3835e3b024923d36e9b37335ca4dfc02dbbfae60"
SOURCE_TRANSITIONS = {
    AUDIT_SOURCE: ("938778c2ea65a053a7bcc41328a1e8ca61c76b7e2eccf079e8ce230969a2dead",
                   "bdfc09b352dcf7e8b5e066d64753a43959b8670fc2b085218907636e0a3bbf93"),
    PRIVACY_SOURCE: ("b27a9f34616f12b91d68d89f15c42620450f3d4e631905b9376e93c2d470f119",
                     "1e2412dd22b0943c894d8a22fbe4db0eca558566df989d9a395a6abced41a88d"),
    PARSER_SOURCE: ("a3f16897f7fcfce33c863dc959e392850268621762ac2f957011cca4c6bee0ce",
                    "09c664a667ba1009e73add94adfbf1a1f4f69d7ed4e8f7cc52b380175d03179c"),
}
UPDATE = "UPDATE school_data SET password = '[REDACTED]' /* masked */, version = 4 WHERE id = 7 AND version = 3"
LIKE = "password LIKE '[REDACTED]' /* masked */"
SECRETS = ("PRIVATE-PASSWORD-CANARY", "UPDATED-PASSWORD-CANARY")
SAVE_MODES = (
    "success", "write_failure", "sink_failure", "policy_denied", "stale_version",
    "policy_approved", "graph_success", "graph_policy_denied", "graph_policy_approved",
    "graph_write_failure", "graph_stale_child", "graph_stale_root",
)
COMMITTED = {"success", "sink_failure", "policy_approved", "graph_success",
             "graph_policy_approved"}
EVENT_COUNTS = {"success": 1, "sink_failure": 1, "policy_approved": 1,
                "graph_success": 2, "graph_policy_approved": 2}


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sql_lines(log, needle):
    return [line for line in log.splitlines() if needle in line]


def protected(log):
    return all(secret not in log for secret in SECRETS)


@cache
def transition_evidence():
    require(digest(AP02_SOURCE) == AP02_SOURCE_SHA256, "installed parser evidence identity")
    source = read(AP02_SOURCE)
    require(source.get("complete") is True and source.get("inputs_unchanged") is True
            and source.get("input_hashes_before") == source.get("input_hashes_after")
            and source.get("differences") == [] and source.get("junit_passed") is True
            and source.get("junit_tests") == 157 and len(source.get("cases", [])) == 80185
            and all(case["match"] is True for case in source["cases"])
            and source.get("owned_directory_removed") is True,
            "installed parser source and Java evidence")
    require(source["input_hashes_before"].get(PARSER_SOURCE)
            == SOURCE_TRANSITIONS[PARSER_SOURCE][1], "installed parser source hash")

    require(digest(AP02_SQL) == AP02_SQL_SHA256, "AP02 SQLite evidence identity")
    sql = read(AP02_SQL)
    require(sql.get("complete") is True and sql.get("inputs_unchanged") is True
            and sql.get("input_hashes_before") == sql.get("input_hashes_after")
            and sql.get("save_match") is True and sql.get("save_cases") == 12
            and sql.get("query_match") is True and sql.get("sink_cases") == 12
            and len(sql.get("steps", [])) == 41
            and all(step["exit_code"] == 0 for step in sql["steps"])
            and sql.get("owned_directory_removed") is True,
            "AP02 generated SQLite execution evidence")
    require(sql["input_hashes_before"].get(AUDIT_SOURCE)
            == SOURCE_TRANSITIONS[AUDIT_SOURCE][1]
            and sql["input_hashes_before"].get(PARSER_SOURCE)
            == SOURCE_TRANSITIONS[PARSER_SOURCE][1], "AP02 changed source hashes")
    require(digest(PRIVACY_CONTROL) == PRIVACY_CONTROL_SHA256,
            "private SQL sink optimized control identity")
    return source, sql


def verify_current_sources(main):
    source, sql = transition_evidence()
    require(source["runtime_commit"] == sql["runtime_commit"] == main["runtime_commit"]
            and source["generator_commit"] == sql["generator_commit"]
            == main["generator_commit"], "source transition reference revisions")
    recorded = main["input_hashes_before"]
    current_sql = sql["input_hashes_before"]
    require(set(SOURCE_TRANSITIONS) <= set(recorded), "historical transition sources")
    for name, wanted in recorded.items():
        path = Path(name)
        require(path.is_file(), "current source missing: " + name)
        if name in SOURCE_TRANSITIONS:
            old, current = SOURCE_TRANSITIONS[name]
            require(wanted == old and digest(path) == current,
                    "declared source transition changed: " + name)
        else:
            require(digest(path) == wanted, "unchanged source identity: " + name)
    require(set(recorded) - set(current_sql) == {OLD_CORPUS}
            and set(current_sql) - set(recorded) == {NEW_CORPUS}
            and {name for name in set(recorded) & set(current_sql)
                 if recorded[name] != current_sql[name]} == {AUDIT_SOURCE, PARSER_SOURCE}
            and current_sql[NEW_CORPUS] == digest(Path(NEW_CORPUS)),
            "exact AP02 source and corpus transition")


def verify(main, query, *, current_sources):
    runtime = json.loads((ROOT / "upstream.lock.json").read_text())
    generator = json.loads((ROOT.parent / "teaql_code_gen_cpp/cpp_generator.lock.json").read_text())
    require(main.get("complete") is True, "complete original/C++ execution")
    require(main.get("runtime_commit") == query.get("runtime_commit") == runtime["commit"],
            "locked Rust runtime")
    require(main.get("generator_commit") == generator["generator_baseline_commit"],
            "locked original generator")
    require(main.get("mask_declaration") == "credential_field"
            and main.get("save_field") == "password", "generated Credential model")
    require(main.get("inputs_unchanged") is True
            and main.get("input_hashes_before") == main.get("input_hashes_after")
            and len(main["input_hashes_before"]) == 533, "complete unchanged input epoch")
    if current_sources:
        reference = subprocess.check_output(
            ["git", "-C", str(ROOT / ".reference/teaql_code_gen"), "rev-parse", "HEAD"],
            text=True).strip()
        require(reference == main["generator_commit"], "current original checkout revision")
        verify_current_sources(main)
    require(main.get("classpath_unchanged") is True,
            "original and producer Java dependency identities")
    require(len(main.get("classpath_hashes", {})) == 59,
            "complete resolved Java dependency set")
    if current_sources:
        require(all(Path(path).is_file() and digest(Path(path)) == wanted
                    for path, wanted in main["classpath_hashes"].items()),
                "current Java dependency hashes")
    require(main.get("owned_directory_removed") is True
            and not Path(main["owned_directory"]).exists()
            and main.get("owned_bytes_removed", 0) > 0,
            "owned build and log cleanup")
    require(len(main.get("original_generated_hashes", {})) == 44
            and len(main.get("cpp_generated_hashes", {})) == 98,
            "complete generated workspace file sets")
    require(main.get("metadata_match") is True
            and same(main["original_metadata"], main["cpp_metadata"]),
            "compiled generated descriptor equality")
    school = main["original_metadata"]["descriptors"][0]
    require(school["entity"] == "School" and school["password_sql_policy"] == "Credential",
            "generated password policy")

    steps = main.get("steps")
    required = {"upstream", "generator_ledger", "original_classpath", "original_compile",
                "original_generate", "cpp_classpath", "cpp_compile", "cpp_generate",
                "rust_build", "cpp_build", "cpp_metadata_build", "cpp_save_build",
                "rust_metadata", "cpp_metadata", "rust_query", "cpp_query", "sink_matrix"}
    required.update(f"{language}_save_{mode}"
                    for language in ("rust", "cpp") for mode in SAVE_MODES)
    required.update(f"mutation_sink_{language}_{mode}"
                    for language in ("rust", "cpp") for mode in ("unset", "enabled"))
    require(type(steps) is list and len(steps) == 45
            and {step["label"] for step in steps} == required
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0
                    for step in steps), "45 terminal execution steps")

    original = main.get("original_save")
    candidate = main.get("cpp_save")
    require(main.get("save_match") is True
            and set(original) == set(candidate) == set(SAVE_MODES)
            and same(original, candidate), "twelve complete Save comparisons")
    for mode in SAVE_MODES:
        row = original[mode]
        committed = mode in COMMITTED
        require(row["db_name"] == ("UPDATED-PASSWORD-CANARY" if committed
                                   else "PRIVATE-PASSWORD-CANARY"),
                "authoritative database state: " + mode)
        require(row["event_count"] == EVENT_COUNTS.get(mode, 0),
                "committed audit delivery count: " + mode)
        require(row["save_ok"] is (mode in {"success", "policy_approved",
                                             "graph_success", "graph_policy_approved"}),
                "Save outcome: " + mode)
        if committed:
            field = row["safe_field"]
            require(field["name"] == "password" and field["value"] == "[REDACTED]"
                    and field["masked"] is True and field["truncated"] is False
                    and protected(json.dumps(field)), "safe credential event: " + mode)
        else:
            require(row["safe_field"] is None, "no precommit credential event: " + mode)
    require(main.get("query_match") is True
            and same(main["original_query"], main["cpp_query"])
            and main["original_query"]["rows"] == [[7, 3, None]],
            "generated credential SQLite query")

    mutation = main.get("mutation_sink_observations")
    require(main.get("mutation_sink_cases") == 4 and len(mutation) == 4
            and {(row["language"], row["case"]) for row in mutation}
            == {(lang, mode) for lang in ("rust", "cpp")
                for mode in ("unset", "enabled")}, "four fresh mutation sink cases")
    for row in mutation:
        require(protected(row["ordinary"]) and protected(row["debug"]),
                "credential absent from mutation files")
        require(sql_lines(row["ordinary"], "UPDATE school_data") == [UPDATE],
                "masked ordinary UPDATE SQL")
        require((sql_lines(row["debug"], "UPDATE school_data") == [UPDATE])
                if row["case"] == "enabled" else row["debug"] == "",
                "exact acknowledgement protects debug UPDATE")
    require(main.get("sink_cases") == 12 and main["sink_receipt"]["path"]
            == str(QUERY.relative_to(ROOT))
            and main["sink_receipt"]["sha256"] == digest(QUERY),
            "bound query file-sink receipt")
    observations = query.get("observations")
    require(query.get("owned_directory_removed") is True
            and len(observations) == 12
            and {(row["language"], row["case"]) for row in observations}
            == {(lang, mode) for lang in ("rust", "cpp")
                for mode in ("unset", "invalid", "enabled", "no_debug_sink",
                             "endpoint_off", "logging_disabled")},
            "twelve fresh query sink cases")
    for row in observations:
        require(row["exit_code"] == 0 and protected(row["ordinary"])
                and protected(row["debug"]), "query sink completion and privacy")
        require(row["authorized_debug"] is (row["case"] == "enabled"),
                "exact plaintext-debug authorization")
        if row["case"] in ("endpoint_off", "logging_disabled"):
            require(row["ordinary"] == row["debug"] == "", "disabled logging")
        else:
            require(len(sql_lines(row["ordinary"], LIKE)) == 1,
                    "masked ordinary LIKE SQL")
            require((len(sql_lines(row["debug"], LIKE)) == 1)
                    if row["case"] == "enabled" else row["debug"] == "",
                    "exact acknowledgement protects debug LIKE")


def controls(main, query):
    def poison_source(receipt, path):
        receipt["input_hashes_before"][path] = "0" * 64
        receipt["input_hashes_after"][path] = "0" * 64

    def poison_same_source(receipt, _query):
        path = next(path for path in receipt["input_hashes_before"]
                    if path not in SOURCE_TRANSITIONS)
        poison_source(receipt, path)

    changes = (
        ("numeric_complete", lambda m, q: m.update(complete=1)),
        ("changed_input", lambda m, q: m["input_hashes_after"].update(
            {next(iter(m["input_hashes_after"])): "0" * 64})),
        ("same_stale_source", poison_same_source),
        ("stale_audit_transition", lambda m, q: poison_source(m, AUDIT_SOURCE)),
        ("stale_privacy_transition", lambda m, q: poison_source(m, PRIVACY_SOURCE)),
        ("stale_parser_transition", lambda m, q: poison_source(m, PARSER_SOURCE)),
        ("stale_classpath", lambda m, q: m["classpath_hashes"].update(
            {next(iter(m["classpath_hashes"])): "0" * 64})),
        ("missing_step", lambda m, q: m["steps"].pop()),
        ("altered_original_save", lambda m, q: m["original_save"]["success"].update(
            db_name="PRIVATE-PASSWORD-CANARY")),
        ("altered_candidate_save", lambda m, q: m["cpp_save"]["success"].update(
            event_count=0)),
        ("precommit_event", lambda m, q: m["original_save"]["write_failure"].update(
            event_count=1)),
        ("unsafe_safe_field", lambda m, q: m["original_save"]["success"]["safe_field"].update(
            value="UPDATED-PASSWORD-CANARY")),
        ("mutation_ordinary_secret", lambda m, q: m["mutation_sink_observations"][0].update(
            ordinary=m["mutation_sink_observations"][0]["ordinary"] + SECRETS[1])),
        ("mutation_debug_without_ack", lambda m, q: m["mutation_sink_observations"][0].update(
            debug=UPDATE)),
        ("wrong_sink_hash", lambda m, q: m["sink_receipt"].update(sha256="0" * 64)),
        ("query_debug_secret", lambda m, q: next(row for row in q["observations"]
            if row["case"] == "enabled").update(debug=SECRETS[0])),
        ("missing_query_case", lambda m, q: q["observations"].pop()),
    )
    results = []
    for name, change in changes:
        changed_main, changed_query = copy.deepcopy(main), copy.deepcopy(query)
        change(changed_main, changed_query)
        try:
            verify(changed_main, changed_query,
                   current_sources=name in ("same_stale_source", "stale_audit_transition",
                                            "stale_privacy_transition", "stale_parser_transition",
                                            "stale_classpath"))
        except (ValueError, KeyError, TypeError, AssertionError) as error:
            results.append({"name": name, "rejected": True, "reason": str(error)})
        else:
            raise AssertionError("Receipt corruption passed: " + name)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    if args.record and (args.record.exists() or args.record.suffix != ".gz"):
        parser.error("Use a fresh .gz receipt")
    receipt, query = read(MAIN), read(QUERY)
    verify(receipt, query, current_sources=True)
    results = controls(receipt, query)
    record = {
        "complete": True,
        "scope": "Current-source receipt admission and controlled corruption of retained generated Credential execution; not fresh SQL execution or full G14/G17/P7 signoff",
        "runtime_commit": receipt["runtime_commit"],
        "generator_commit": receipt["generator_commit"],
        "inputs": {str(path.relative_to(ROOT)): digest(path)
                   for path in (MAIN, QUERY, Path(__file__))},
        "source_inputs_checked": len(receipt["input_hashes_before"]),
        "source_transitions_checked": len(SOURCE_TRANSITIONS),
        "unchanged_source_inputs_checked": len(receipt["input_hashes_before"])
        - len(SOURCE_TRANSITIONS),
        "source_transition_evidence": {
            str(AP02_SOURCE.relative_to(ROOT)): AP02_SOURCE_SHA256,
            str(AP02_SQL.relative_to(ROOT)): AP02_SQL_SHA256,
            str(PRIVACY_CONTROL.relative_to(ROOT)): PRIVACY_CONTROL_SHA256,
        },
        "classpath_inputs_checked": len(receipt["classpath_hashes"]),
        "original_cpp_save_pairs": len(SAVE_MODES),
        "query_sink_cases": len(query["observations"]),
        "mutation_sink_cases": len(receipt["mutation_sink_observations"]),
        "controls": results,
    }
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + "\n").encode(),
                                              mtime=0))
    print("Generated Credential receipt: 530 unchanged inputs, 3 source transitions, "
          "12 Save pairs, "
          "12 query sinks, 4 mutation sinks, " + str(len(results)) + " controls rejected")


if __name__ == "__main__":
    main()
