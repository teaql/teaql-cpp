#!/usr/bin/env python3
"""Admit strict projection JSON and exact preceding source/artifact identities."""
import argparse
import ast
import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
RECORD = ROOT / "docs/parity/projection_trailing_sources_5_0_6.json.gz"
RECORD_SHA = "c07036d0d2914537e8ff3c91561aa1ee9b6743c824b085e031bd7168354e25d6"
JAVA = ROOT / "docs/parity/projection_trailing_java_5_0_6.json.gz"
JAVA_SHA = "e18a8fe48b70e903f94fd233a0db485945435dfb8822c501e59ae811e3124155"
SDKS = ROOT / "docs/parity/projection_trailing_sdks_5_0_6.json.gz"
SDKS_SHA = "88daf36f523edabbaf94a2a68a93bd7c27fa0bec4676a797b7bac0936042e2e8"
READER = "producer:generator/src/main/java/com/skynet/codegenerator/cpp/CppCompilerProjectionResolver.java"
TEST = "producer:generator/src/test/java/com/skynet/codegenerator/cpp/CppCompilerProjectionResolverTest.java"
SELF = "runtime:scripts/projection_trailing_source_admission_5_0_6.py"
READERS = {
    "runtime:scripts/default_source_guard_source_admission_5_0_6.py":
        {"validate_java", "validate_sdks", "validate", "current_input_matches",
         "qualified_transition", "preceding_source", "sdk_snapshot_matches"},
    "runtime:scripts/compiler_offline_source_admission_5_0_6.py":
        {"source_epochs", "added_input_hashes"},
}

def require(value, message):
    if not value:
        raise ValueError("Projection JSON source admission: " + message)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def read(path):
    return json.loads(gzip.decompress(Path(path).read_bytes()))

def source_path(name):
    scope, relative = name.split(":", 1)
    require(scope in ("runtime", "producer") and relative and not Path(relative).is_absolute()
            and ".." not in Path(relative).parts, "canonical source identity")
    return (ROOT if scope == "runtime" else PRODUCER) / relative

def validate_java(record):
    require(record.get("complete") is True and record.get("whole_goal_complete") is False
            and record["inputs_before"] == record["inputs_after"]
            and record["references_before"] == record["references_after"], "complete stable Java execution")
    steps = record["steps"]
    require([s["label"] for s in steps] == ["fresh_current_javac", "current_shared_junit",
            "current_cpp_junit", "counterfactual_javac", "counterfactual_projection_regressions"]
            and all(type(s["exit_code"]) is int for s in steps)
            and [s["exit_code"] for s in steps] == [0, 0, 0, 0, 1]
            and "OK (98 tests)" in steps[1]["stdout"] and "OK (305 tests)" in steps[2]["stdout"]
            and "Tests run: 12,  Failures: 1" in steps[-1]["stdout"]
            and "trailingJsonValuesAndGarbageRejectBeforeReturningFacts" in steps[-1]["stdout"],
            "403 current tests and exact preceding-reader failure")
    seed = read(ROOT / "docs/parity/default_source_guard_java_complete_5_0_6.json.gz")
    for index, key in ((1, "shared_test_classes"), (2, "cpp_test_classes")):
        command = steps[index]["command"]
        require(record[key] == seed[key]
                and command[command.index("org.junit.runner.JUnitCore") + 1:] == seed[key],
                "complete JUnit membership")
    require(len(record["compiled_java_sources"]) == 331
            and set(record["compiled_java_sources"]) == {x for x in steps[0]["command"] if x.endswith(".java")}
            and all(sha(Path(p).read_bytes()) == digest for p, digest in record["compiled_java_sources"].items()),
            "complete actual current Java compiler inputs")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
            and type(record["owned_bytes_removed"]) is int and record["owned_bytes_removed"] > 0,
            "measured owned cleanup")
    images = record["source_images"]
    require(set(images) == {x.split(":", 1)[1] for x in (READER, TEST)}, "only reader and regression sources")
    for name, row in images.items():
        require(type(row["before_source"]) is str and row["before_sha256"] == sha(row["before_source"].encode())
                and row["after_sha256"] == sha(row["after_source"].encode())
                == sha((PRODUCER / name).read_bytes()), "complete actual source images")
    row = images[READER.split(":", 1)[1]]
    old = "    ObjectMapper mapper = new ObjectMapper().enable(JsonParser.Feature.STRICT_DUPLICATE_DETECTION);\n"
    new = ("    ObjectMapper mapper = new ObjectMapper().enable(JsonParser.Feature.STRICT_DUPLICATE_DETECTION)\n"
           "        .enable(com.fasterxml.jackson.databind.DeserializationFeature.FAIL_ON_TRAILING_TOKENS);\n")
    require(row["before_source"].count(old) == 1 and row["before_source"].replace(old, new, 1) == row["after_source"]
            and record["counterfactual_reader_source"] == row["before_source"],
            "only complete-stream JSON validation changes")

def validate_sdks(record):
    from reproduce_compiler_projection_sdks_5_0_6 import snapshot
    require(record.get("complete") is True and record.get("whole_goal_complete") is False
            and len(record["before"]) == len(record["after"]) == 80
            and record["before"] == record["observed_before"] and record["resume_expected_provenance"] is False
            and record["after"] == snapshot(), "all eighty current reproduced SDKs")
    for before, after in zip(record["before"], record["after"], strict=True):
        expected = copy.deepcopy(before["value"])
        expected["generator_implementation_sha256"][READER.split(":", 1)[1]] = sha(source_path(READER).read_bytes())
        leaf = Path(before["manifest"]).name
        require(before["manifest"] == after["manifest"] and before["files"].keys() == after["files"].keys()
                and all(after["files"][key] == digest for key, digest in before["files"].items() if key != leaf)
                and after["value"] == expected, "only projection-reader provenance changes")
    actions = [("reproduce_cpp_contract_fixtures.py", False), ("generate_examples.py", False),
        ("reproduce_cpp_independent_sdks_5_0_6.py", False), ("reproduce_cpp_contract_fixtures.py", True),
        ("generate_examples.py", True), ("reproduce_cpp_independent_sdks_5_0_6.py", False)]
    require([(x["recipe"], "--check" in x["arguments"]) for x in record["actions"]] == actions
            and all(type(x["exit_code"]) is int and x["exit_code"] == 0
                    for x in record["steps"] + record["actions"] + [record["preparation_step"]]),
            "all actual packaging/generation/check commands")
    require(record["preparation_inputs_before"] == record["preparation_inputs_after"]
            and record["references_before"] == record["references_after"]
            and record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(),
            "stable inputs/references and owned cleanup")
    before, after = record["input_hashes_before"], record["input_hashes_after"]
    require(before.keys() == after.keys()
            and {key for key in before if before[key] != after[key]} ==
                {"runtime:" + row["manifest"] for row in record["before"]}
            and sum(len(row["files"]) - 1 for row in record["after"]) == 3560,
            "exact eighty manifest updates and 3560 unchanged code/Assist files")

@cache
def executed_records():
    require(sha(JAVA.read_bytes()) == JAVA_SHA and sha(SDKS.read_bytes()) == SDKS_SHA, "immutable actual executions")
    java, sdks = read(JAVA), read(SDKS)
    validate_java(java)
    validate_sdks(sdks)
    require(java["inputs_after"] == sdks["preparation_inputs_before"]
            == {key: value for key, value in sdks["input_hashes_before"].items()
                if key != "runtime:docs/cpp_generator_parity_ledger.json"}, "one source epoch")
    return java, sdks

def validate(record):
    from default_source_guard_source_admission_5_0_6 import function_changes
    java, sdks = executed_records()
    expected = {READER, TEST, *READERS, *("runtime:" + row["manifest"] for row in sdks["after"])}
    require(record.get("schema") == "teaql.projection_json.source_transition.v1"
            and record.get("complete") is True and record.get("whole_goal_complete") is False
            and record["java_sha256"] == JAVA_SHA and record["sdks_sha256"] == SDKS_SHA
            and set(record["paths"]) == expected and len(expected) == 84, "complete exact source membership")
    for name, row in record["paths"].items():
        require(type(row["before_source"]) is str and row["before_sha256"] == sha(row["before_source"].encode())
                and row["after_sha256"] == sha(row["after_source"].encode())
                == sha(source_path(name).read_bytes()), "full current before/after images")
        if name.startswith("producer:"):
            require(row == java["source_images"][name.split(":", 1)[1]], "actual compiled source image")
        if name in READERS:
            require(row["before_sha256"] == java["inputs_before"][name], "reader preimage in actual Java frame")
            function_changes(row["before_source"], row["after_source"], READERS[name])
    for before, after in zip(sdks["before"], sdks["after"], strict=True):
        row = record["paths"]["runtime:" + before["manifest"]]
        require(row["before_source"] == before["text"] and row["after_source"] == after["text"],
                "actual regenerated metadata images")
    binding = 'RECORD_SHA = "' + RECORD_SHA + '"\n'
    from native_receipt_budget_source_admission_5_0_6 import preceding_source
    current = preceding_source(SELF, Path(__file__).read_text())
    require(current.count(binding) == 1 and current.replace(binding, 'RECORD_SHA = "__SOURCE_SHA__"\n', 1)
            == record["helper_after_source_normalized"], "complete qualified helper image")

@cache
def sources():
    require(sha(RECORD.read_bytes()) == RECORD_SHA, "immutable source continuation")
    value = read(RECORD)
    validate(value)
    return value["paths"]

def source_rows():
    from native_receipt_budget_source_admission_5_0_6 import source_rows as budget_rows
    return {**sources(), SELF: dict(before_source=None, before_sha256=None,
        after_source=Path(__file__).read_text(), after_sha256=sha(Path(__file__).read_bytes())),
        **budget_rows()}

def added_input_hashes():
    from native_receipt_budget_source_admission_5_0_6 import added_input_hashes as budget_added
    return {SELF: sha(Path(__file__).read_bytes()), **budget_added()} if sources() else {}

def current_input_matches(path, expected):
    path = Path(path).resolve()
    if type(expected) is not str or re.fullmatch(r"[0-9a-f]{64}", expected) is None or not path.is_file():
        return False
    scope = "runtime:" if path.is_relative_to(ROOT) else "producer:" if path.is_relative_to(PRODUCER) else None
    if scope is None:
        return False
    base = ROOT if scope == "runtime:" else PRODUCER
    row = source_rows().get(scope + path.relative_to(base).as_posix())
    return row is not None and expected in (row["before_sha256"], row["after_sha256"])

def qualified_transition(name, before, after):
    row = source_rows().get(name)
    return row is not None and row["before_sha256"] == before and row["after_sha256"] == after

def preceding_source(name, current):
    row = sources().get(name)
    require(row is not None and current == row["after_source"], "one exact preceding reader image")
    return row["before_source"]

def sdk_snapshot_matches(archived, observed):
    sources()
    record = read(SDKS)
    return archived == record["before"] and observed == record["after"]

def controls(record):
    results = []
    for name, mutate in [
        ("numeric_complete", lambda r: r.update(complete=1)),
        ("goal_promoted", lambda r: r.update(whole_goal_complete=True)),
        ("missing_reader", lambda r: r["paths"].pop(READER)),
        ("missing_manifest", lambda r: r["paths"].pop("runtime:examples/generated/manifest.json")),
        ("changed_reader", lambda r: r["paths"][next(iter(READERS))].update(after_source="pass")),
        ("changed_helper", lambda r: r.update(helper_after_source_normalized="pass")),
    ]:
        altered = copy.deepcopy(record)
        mutate(altered)
        try:
            validate(altered)
        except (ValueError, RuntimeError, KeyError, TypeError, IndexError) as error:
            results.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise ValueError("Invalid projection source continuation admitted: " + name)
    for path, validator, mutations in [
        (JAVA, validate_java, [
            ("unfinished_java", lambda r: r.update(complete=False)),
            ("boolean_java_exit", lambda r: r["steps"][0].update(exit_code=False)),
            ("hidden_old_failure", lambda r: r["steps"][-1].update(exit_code=0)),
            ("missing_java_source", lambda r: r["compiled_java_sources"].pop(next(iter(r["compiled_java_sources"])))),
            ("changed_old_reader", lambda r: r.update(counterfactual_reader_source="class Altered {}")),
        ]),
        (SDKS, validate_sdks, [
            ("unfinished_sdk", lambda r: r.update(complete=False)),
            ("missing_sdk", lambda r: r["after"].pop()),
            ("changed_sdk_file", lambda r: r["after"][0]["files"].update(invented="0" * 64)),
            ("lost_generation", lambda r: r["actions"].pop()),
            ("boolean_packaging_exit", lambda r: r["preparation_step"].update(exit_code=False)),
        ]),
    ]:
        for name, mutate in mutations:
            altered = read(path)
            mutate(altered)
            try:
                validator(altered)
            except (ValueError, RuntimeError, KeyError, TypeError, IndexError) as error:
                results.append(dict(case=name, rejected=True, error=str(error)))
            else:
                raise ValueError("Invalid projection execution admitted: " + name)
    return results

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(not args.record or not args.record.exists(), "preserve control evidence")
    sources()
    results = controls(read(RECORD))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True, whole_goal_complete=False,
            source_sha256=RECORD_SHA, results=results), sort_keys=True).encode(), mtime=0))
    print(f"Strict projection JSON qualified; {len(results)} corruptions rejected")

if __name__ == "__main__":
    main()
