#!/usr/bin/env python3
"""Qualify source-selected String assignment and exact historical source forwarding."""
import argparse
import ast
import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
sys.path.insert(0, str(ROOT / "tests/parity"))
RECORD = ROOT / "docs/parity/compiler_text_alias_sources_5_0_6.json.gz"
RECORD_SHA = "af2dd7e1cef73559884795e2f47d407206241401e59998c459e65b7700f08ba9"
EXECUTION = ROOT / "docs/parity/compiler_text_alias_repaired_complete_5_0_6.json.gz"
EXECUTION_SHA = "023ecc5e1368caf3eb4873dc1fd621058ee913134700e1ef39c2aca6e67e0082"
SDKS = ROOT / "docs/parity/compiler_text_alias_sdks_5_0_6.json.gz"
SDKS_SHA = "ff913c9cd11477458ad909f23c1be7a493f809d94d0ddd5dbdf2737dd904f56e"
DIAGNOSIS = ROOT / "docs/parity/compiler_text_alias_diagnosis_5_0_6.json.gz"
DIAGNOSIS_SHA = "6a571d96c61ba1953efa7bd9b5212cff517d6774107c3617f3c3962d94103dd3"
EXTENDED = ROOT / "docs/parity/compiler_text_alias_controls_execution_5_0_6.json.gz"
EXTENDED_SHA = "2d0760b2c1f1ef804f28ef22a2c2140698185bd721de4cb847c7ebf6852dcc82"
CORE = "producer:generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java"
SELF = "runtime:scripts/text_alias_source_admission_5_0_6.py"
READERS = {
    "runtime:scripts/verify_compiled_column_boundary_5_0_6.py": {"validate"},
    "runtime:scripts/reproduce_compiled_column_sdks_5_0_6.py": {"validate"},
    "runtime:scripts/compiled_column_source_admission_5_0_6.py":
        {"validate", "current_input_matches", "qualified_transition"},
    "runtime:scripts/compiler_offline_source_admission_5_0_6.py":
        {"source_epochs", "added_input_hashes"},
}

def require(condition, message):
    if not condition:
        raise ValueError("Text assignment source admission: " + message)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def read(path):
    return json.loads(gzip.decompress(Path(path).read_bytes()))

def source_path(name):
    scope, relative = name.split(":", 1)
    require(scope in ("runtime", "producer") and relative and not Path(relative).is_absolute()
            and ".." not in Path(relative).parts, "canonical source identity")
    return (ROOT if scope == "runtime" else PRODUCER) / relative

def function_changes(before, after, expected):
    trees = [ast.parse(value) for value in (before, after)]
    functions = [{n.name: ast.dump(n) for n in tree.body if isinstance(n, ast.FunctionDef)} for tree in trees]
    require(functions[0].keys() == functions[1].keys()
            and {key for key in functions[0] if functions[0][key] != functions[1][key]} == expected,
            "exact identity-only reader changes")
    for tree in trees:
        for node in tree.body:
            if isinstance(node, ast.FunctionDef) and node.name in expected:
                node.body = [ast.Pass()]
    require(ast.dump(trees[0]) == ast.dump(trees[1]), "unchanged execution, comparison and control bodies")

def validate_execution(record):
    from default_source_guard_source_admission_5_0_6 import current_input_matches as guard_matches

    from compare_values import json_lines, same
    diagnosis = read(DIAGNOSIS)
    require(record.get("complete") is True and record.get("whole_goal_complete") is False
            and record["diagnosis_sha256"] == DIAGNOSIS_SHA, "finished native repair and exact original reuse")
    require(record["inputs_before"] == record["inputs_after"]
            and record["references_before"] == record["references_after"]
            and record["native_before"] == record["native_after"]
            and record["clock_before"] == record["clock_after"], "stable complete execution frame")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
            and type(record["owned_bytes_removed"]) is int and record["owned_bytes_removed"] > 0,
            "owned repair outputs removed")
    steps = record["steps"]
    require([step["label"] for step in steps] == ["fresh_javac", "shared_junit", "cpp_junit",
            "compiled_sdk_generate", "checker_native_build", "checker_native_run",
            "reads_native_build", "reads_native_run", "graph_native_build", "graph_native_run"]
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0 for step in steps)
            and "OK (98 tests)" in steps[1]["stdout"] and "OK (297 tests)" in steps[2]["stdout"],
            "actual current Java regression")
    require(len(record["compiled_java_sources"]) == 330 and set(record["compiled_java_sources"]) == {
        arg for arg in steps[0]["command"] if arg.endswith(".java")}
        and all(sha(Path(path).read_bytes()) == digest or guard_matches(path, digest)
                for path, digest in record["compiled_java_sources"].items()),
        "complete current Java compiler inputs")
    case = diagnosis["cases"][0]
    total = 0
    for name, count in (("checker", 492), ("reads", 2), ("graph", 13)):
        values = record["consumers"][name]
        expected = case["consumers"][name]["original"]
        run = next(step for step in steps if step["label"] == name + "_native_run")
        require(values["original_reused"] is True and values["equal"] is True
                and same(values["inputs"], case["consumers"][name]["inputs"])
                and same(values["original"], expected) and same(values["cpp"], expected)
                and same(json_lines(run["stdout"]), expected) and len(expected) == count,
                "complete actual native/original observation identity: " + name)
        total += count
    require(total == 507, "all changed and unchanged consumers")

def validate_sdks(record):
    from default_source_guard_source_admission_5_0_6 import sdk_snapshot_matches as guard_snapshot, preceding_fingerprint

    from reproduce_compiler_projection_sdks_5_0_6 import snapshot
    require(record.get("complete") is True and record.get("whole_goal_complete") is False
            and len(record["before"]) == len(record["after"]) == 80
            and (record["after"] == snapshot() or guard_snapshot(record["after"], snapshot())), "eighty current regenerated SDKs")
    for before, after in zip(record["before"], record["after"], strict=True):
        leaf = Path(before["manifest"]).name
        expected = copy.deepcopy(before["value"])
        expected["generator_implementation_sha256"][CORE.split(":", 1)[1]] = preceding_fingerprint(CORE)
        require(before["manifest"] == after["manifest"] and before["files"].keys() == after["files"].keys()
                and all(after["files"][key] == digest for key, digest in before["files"].items() if key != leaf)
                and after["value"] == expected, "only source-selected setter fingerprint changes")
    expected_actions = [("reproduce_cpp_contract_fixtures.py", False), ("generate_examples.py", False),
        ("reproduce_cpp_independent_sdks_5_0_6.py", False), ("reproduce_cpp_contract_fixtures.py", True),
        ("generate_examples.py", True), ("reproduce_cpp_independent_sdks_5_0_6.py", False)]
    require([(step["recipe"], "--check" in step["arguments"]) for step in record["actions"]] == expected_actions
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0
                    for step in record["steps"] + record["actions"]), "finished generation and verification commands")
    require(record["observed_before"] == record["before"] and record["resume_expected_provenance"] is False
            and record["references_before"] == record["references_after"]
            and record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(),
            "fixed reference and owned SDK cleanup")
    old, new = record["input_hashes_before"], record["input_hashes_after"]
    require(old.keys() == new.keys()
            and {key for key in old if old[key] != new[key]} ==
                {"runtime:" + row["manifest"] for row in record["before"]},
            "exact eighty-manifest source transition")
    require(sum(len(row["files"]) - 1 for row in record["after"]) == 3560,
            "all retained code and guidance")

def validate_extended(record):
    """Qualify only the two executed controls and retain the third reference refusal."""
    from compare_values import json_lines, same
    require(record.get("complete") is False and record.get("whole_goal_complete") is False
            and record.get("semantic_acceptance") is False, "extended batch remains incomplete")
    require(all(record[key + "_before"] == record[key + "_after"]
                for key in ("inputs", "native", "references", "clock"))
            and record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(),
            "stable extended execution and cleanup")
    names = ["qualified_text_alias_required", "literal_text_alias_optional", "associated_text_alias_optional"]
    require(record["selected_cases"] == names and [case["name"] for case in record["cases"]] == names,
            "exact two executed controls and refused third input")
    steps = record["steps"]
    require(len(steps) == 28 and all(type(step["exit_code"]) is int and step["exit_code"] == 0
                                   for step in steps[:-1])
            and steps[-1]["exit_code"] == 2 and type(steps[-1]["exit_code"]) is int
            and steps[-1]["label"] == names[-1] + "_host"
            and "REFERENCE_COMPILER_REFUSAL" in steps[-1]["stderr"] and "E0308" in steps[-1]["stderr"]
            and record["cases"][-1]["consumers"] == {}, "preserved actual original refusal")
    counts = [(249, 2, 10), (492, 2, 13)]
    for case, expected_counts in zip(record["cases"][:2], counts, strict=True):
        require(case["cpp_sources"] == case["cpp_sources_after"]
                and case["original_sources"] == case["original_sources_after"], "stable complete SDKs")
        require(set(case["consumers"]) == {"checker", "reads", "graph"}, "complete protected consumers")
        for name, count in zip(("checker", "reads", "graph"), expected_counts, strict=True):
            consumer = case["consumers"][name]
            original_run = next(step for step in steps if step["label"] == case["name"] + "_" + name + "_original_run")
            cpp_run = next(step for step in steps if step["label"] == case["name"] + "_" + name + "_native_run")
            require(len(consumer["original"]) == count and len(consumer["cpp"]) == count
                    and same(consumer["cpp"], consumer["original"])
                    and same(json_lines(original_run["stdout"]), consumer["original"])
                    and same(json_lines(cpp_run["stdout"]), consumer["cpp"]), "actual extended output: " + name)

@cache
def executed_records():
    require(sha(EXECUTION.read_bytes()) == EXECUTION_SHA and sha(SDKS.read_bytes()) == SDKS_SHA
            and sha(DIAGNOSIS.read_bytes()) == DIAGNOSIS_SHA and sha(EXTENDED.read_bytes()) == EXTENDED_SHA,
            "immutable actual execution records")
    execution, sdks = read(EXECUTION), read(SDKS)
    validate_execution(execution)
    validate_sdks(sdks)
    validate_extended(read(EXTENDED))
    return execution, sdks

def validate(record):
    from default_source_guard_source_admission_5_0_6 import current_input_matches as guard_matches, preceding_source as guard_preimage

    execution, sdks = executed_records()
    expected = {CORE, *READERS, *("runtime:" + row["manifest"] for row in sdks["after"])}
    require(record.get("schema") == "teaql.compiled_text_assignment.source_transition.v1"
            and record.get("complete") is True and record.get("whole_goal_complete") is False
            and record["execution_sha256"] == EXECUTION_SHA and record["sdks_sha256"] == SDKS_SHA
            and set(record["paths"]) == expected and len(expected) == 85,
            "one emitter, four readers and eighty manifests")
    rows = record["paths"]
    for name, row in rows.items():
        require(row["before_sha256"] == sha(row["before_source"].encode())
                and row["after_sha256"] == sha(row["after_source"].encode())
                and (row["after_sha256"] == sha(source_path(name).read_bytes())
                     or guard_matches(source_path(name), row["after_sha256"])), "exact current source transition: " + name)
        if name in READERS:
            require(row["before_sha256"] == execution["inputs_before"][name],
                    "actual preceding source frame")
            function_changes(row["before_source"], row["after_source"], READERS[name])
    require(rows[CORE] == execution["source_images"][CORE.split(":", 1)[1]],
            "actual compiled emitter source image")
    old = '        boolean textField = !CppBusinessIdMetadata.isBusinessId(fd) && baseType(fd).equals("String");\n'
    new = ('        // The locked setter rule trims only the literal Rust type spelling String.\n'
           '        // Compiler-equivalent aliases retain their text through the ordinary clone/stage path.\n'
           '        boolean textField = !CppBusinessIdMetadata.isBusinessId(fd) && baseType(fd).equals("String")\n'
           '            && "String".equals(RustTypeNaming.rustType(fd.getFieldType()));\n')
    require(rows[CORE]["before_source"].count(old) == 1
            and rows[CORE]["before_source"].replace(old, new, 1) == rows[CORE]["after_source"],
            "only source-selected assignment behavior changes")
    for before, after in zip(sdks["before"], sdks["after"], strict=True):
        row = rows["runtime:" + before["manifest"]]
        require(row["before_source"] == before["text"] and row["after_source"] == after["text"],
                "exact regenerated manifest source images")
    binding = 'RECORD_SHA = "' + RECORD_SHA + '"\n'
    current = Path(__file__).read_text()
    if current.replace(binding, 'RECORD_SHA = "__SOURCE_SHA__"\n', 1) != record["helper_after_source_normalized"]:
        current = guard_preimage(SELF, current)
    require(current.count(binding) == 1
            and current.replace(binding, 'RECORD_SHA = "__SOURCE_SHA__"\n', 1)
            == record["helper_after_source_normalized"], "complete source helper image")

@cache
def sources():
    require(sha(RECORD.read_bytes()) == RECORD_SHA, "immutable exact source continuation")
    value = read(RECORD)
    validate(value)
    return value["paths"]

def source_rows():
    return {**sources(), SELF: dict(before_source=None, before_sha256=None,
        after_source=Path(__file__).read_text(), after_sha256=sha(Path(__file__).read_bytes()))}

def added_input_hashes():
    return {SELF: sha(Path(__file__).read_bytes())} if sources() else {}

def current_input_matches(path, expected):
    from default_source_guard_source_admission_5_0_6 import current_input_matches as guard_matches

    path = Path(path).resolve()
    if type(expected) is not str or re.fullmatch(r"[0-9a-f]{64}", expected) is None or not path.is_file():
        return False
    scope = "runtime:" if path.is_relative_to(ROOT) else "producer:" if path.is_relative_to(PRODUCER) else None
    if scope is None:
        return False
    base = ROOT if scope == "runtime:" else PRODUCER
    row = source_rows().get(scope + path.relative_to(base).as_posix())
    return (row is not None and expected in (row["before_sha256"], row["after_sha256"])) or guard_matches(path, expected)

def qualified_transition(name, before, after):
    from default_source_guard_source_admission_5_0_6 import qualified_transition as guard_transition

    row = source_rows().get(name)
    return (row is not None and row["before_sha256"] == before
            and (row["after_sha256"] == after or guard_transition(name, row["after_sha256"], after))) \
        or guard_transition(name, before, after)

def preceding_source(name, current):
    row = sources().get(name)
    if row is not None and current != row["after_source"]:
        from default_source_guard_source_admission_5_0_6 import preceding_source as guard_preimage
        current = guard_preimage(name, current)
    require(name in READERS and row is not None and current == row["after_source"], "exact prior reader image")
    return row["before_source"]

def sdk_snapshot_matches(archived, observed):
    from default_source_guard_source_admission_5_0_6 import sdk_snapshot_matches as guard_snapshot

    sources()
    record = read(SDKS)
    return (archived == record["before"] and (observed == record["after"]
            or guard_snapshot(record["after"], observed))) or guard_snapshot(archived, observed)

def controls(record):
    changes = [
        ("numeric_complete", lambda x: x.update(complete=1)),
        ("goal_promoted", lambda x: x.update(whole_goal_complete=True)),
        ("missing_emitter", lambda x: x["paths"].pop(CORE)),
        ("missing_manifest", lambda x: x["paths"].pop("runtime:examples/generated/manifest.json")),
        ("changed_reader", lambda x: x["paths"][next(iter(READERS))].update(after_source="pass")),
        ("changed_before", lambda x: x["paths"][CORE].update(before_sha256="0" * 64)),
        ("changed_helper", lambda x: x.update(helper_after_source_normalized="pass")),
    ]
    results = []
    for name, change in changes:
        altered = copy.deepcopy(record)
        change(altered)
        try:
            validate(altered)
        except (ValueError, KeyError, TypeError, IndexError) as error:
            results.append(dict(name=name, rejected=True, reason=str(error)))
        else:
            raise ValueError("Invalid text-assignment source transition admitted: " + name)

    execution_changes = [
        ("unfinished_execution", lambda x: x.update(complete=False)),
        ("boolean_compiler_exit", lambda x: x["steps"][0].update(exit_code=False)),
        ("missing_compiler_step", lambda x: x["steps"].pop()),
        ("missing_java_source", lambda x: x["compiled_java_sources"].pop(next(iter(x["compiled_java_sources"])))),
        ("changed_execution_frame", lambda x: x.update(clock_after="0" * 64)),
        ("forged_native_output", lambda x: x["steps"][5].update(stdout="[]")),
        ("hidden_original_reuse", lambda x: x["consumers"]["checker"].update(original_reused=False)),
        ("invented_equal_result", lambda x: x["consumers"]["checker"].update(equal=1)),
    ]
    sdk_changes = [
        ("numeric_sdk_complete", lambda x: x.update(complete=1)),
        ("missing_sdk", lambda x: x["after"].pop()),
        ("changed_sdk_source", lambda x: x["after"][0]["files"].update(invented="0" * 64)),
        ("changed_sdk_fingerprint", lambda x: x["after"][0]["value"]["generator_implementation_sha256"].update(
            {CORE.split(":", 1)[1]: "0" * 64})),
        ("boolean_generation_exit", lambda x: x["steps"][0].update(exit_code=False)),
        ("lost_sdk_action", lambda x: x["actions"].pop()),
        ("undeclared_resume", lambda x: x.update(resume_expected_provenance=True)),
        ("lost_sdk_cleanup", lambda x: x.update(owned_directory_removed=False)),
    ]
    extended_changes = [
        ("extended_promoted", lambda x: x.update(complete=True)),
        ("reference_refusal_hidden", lambda x: x["steps"][-1].update(exit_code=0)),
        ("lost_executed_control", lambda x: x["cases"].pop(0)),
        ("forged_control_output", lambda x: x["steps"][7].update(stdout="[]")),
    ]
    for path, validator, mutations in [(EXECUTION, validate_execution, execution_changes),
                                       (SDKS, validate_sdks, sdk_changes),
                                       (EXTENDED, validate_extended, extended_changes)]:
        for name, change in mutations:
            altered = copy.deepcopy(read(path))
            change(altered)
            try:
                validator(altered)
            except (ValueError, KeyError, TypeError, IndexError) as error:
                results.append(dict(name=name, rejected=True, reason=str(error)))
            else:
                raise ValueError("Invalid text-assignment execution admitted: " + name)
    return results

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(not args.record or not args.record.exists(), "preserve source control evidence")
    sources()
    results = controls(read(RECORD))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True, whole_goal_complete=False,
            source_sha256=RECORD_SHA, results=results), sort_keys=True).encode(), mtime=0))
    print(f"Source-selected String assignment qualified; {len(results)} source/execution corruptions rejected")

if __name__ == "__main__":
    main()
