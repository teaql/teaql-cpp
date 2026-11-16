"""Admit the exact constant-function parser and retained SDK provenance transition."""

import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path
import sys

from reproduce_const_function_sdks_5_0_6 import validate as validate_sdks
from verify_const_function_sql_5_0_6 import validate as validate_sql

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import same

RECORDS = {
    "java": ("ap02_const_function_java_candidate_v5_5_0_6.json.gz",
             "cd0758d390200edac8c901d26c18923b6ad84ba9a250a388e9156424ccea6fe5"),
    "compiled": ("ap02_const_function_repaired_generated_5_0_6.json.gz",
                 "e6d6873b686877e83290892d7a99606bc2ca8388bece23edf29e2aae28a444e5"),
    "proposal": ("ap02_const_function_source_proposal_5_0_6.json.gz",
                 "f9114c0d2785eaf91270fa53db2228563d526b4b9004ccc08da18226cddcf0e4"),
    "preserved": ("ap02_const_function_preserved_sdks_repaired_5_0_6.json.gz",
                  "2fb8876f4d32efb94e8e1ff69801c59b3b4e8ff81b44d4b882d48c1bd1f6024e"),
    "preceding": ("ap02_primitive_const_path_installed_5_0_6.json.gz",
                  "d0660657cc2c0744d797eb5ff10d9104e5afbaa6e95fc23db4fe03cca3c257d8"),
    "sdk": ("ap02_const_function_sdks_5_0_6.json.gz",
            "2480fa3b0782fdf392fe16f3e43a434ee1ce895b6eb0894c488c3e3c2736e984"),
    "sdk_controls": ("ap02_const_function_sdk_admission_5_0_6.json.gz",
                     "e7c2b097ec8692381ad3688a3cef62439c321183dddb86a627d617d9bef1c657"),
    "sql": ("ap02_const_function_sql_5_0_6.json.gz",
            "5202a86d855aa21668053086f31ab39294fc97e12b038bf50d50cb3560c253dc"),
    "sql_controls": ("ap02_const_function_sql_admission_5_0_6.json.gz",
                     "16acd9c9c770cfc0a16031fb316d3abfd3d99b77ac591b15f6bdd69df7818c78"),
    "sql_reuse": ("ap02_const_function_preserved_sql_5_0_6.json.gz",
                  "db36cc8ae7e8716f9d4777ce0b1aeee5ac6cc20f5c43941604dde83d9f3bbc2b"),
    "prior_sql": ("ap02_primitive_const_path_candidate_sql_5_0_6.json.gz",
                  "7de30cf5c4cee81af79cbfce416a44b330b1b73c20874c762a2219b172356b70"),
    "transition": ("ap02_const_function_helper_transition_5_0_6.json.gz",
                   "917d873718be377566c10fd1fb4a315c1fc08cae07f84cea2d244cd5fa39b56e"),
}


def require(value, message):
    if not value:
        raise RuntimeError("Constant-function source admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable evidence: " + name)
    return json.loads(gzip.decompress(data))


def validate(records):
    from compiler_projection_source_admission_5_0_6 import current_input_matches as compiler_input_matches

    java, compiled, proposal = (records[key] for key in ("java", "compiled", "proposal"))
    require(java["complete"] is True and java["qualified"] is True
            and java["differences"] == [] and len(java["cases"]) == 151
            and len(java["steps"]) == 3
            and all(type(row["exit_code"]) is int and row["exit_code"] == 0 for row in java["steps"])
            and "OK (222 tests)" in java["steps"][1]["stdout"]
            and all(type(row["accepted"]) is bool and type(row["cpp_resolved"]) is bool
                    and row["accepted"] is row["cpp_resolved"] and row["match"] is True
                    for row in java["cases"]), "actual Java regressions and projection outcomes")
    require(proposal["complete"] is True and proposal["apply_check_exit_code"] == 0
            and proposal["java_receipt_sha256"] == RECORDS["java"][1]
            and len(proposal["paths"]) == 2
            and sha(proposal["patch_source"].encode()) == proposal["patch_sha256"],
            "reviewable exact two-file source patch")
    for name, row in proposal["paths"].items():
        require(sha(row["after_source"].encode()) == row["after_sha256"]
                and sha((PRODUCER / name).read_bytes()) == row["after_sha256"],
                "exact tested current source")
        if row["before_sha256"] is not None:
            require(sha(row["before_source"].encode()) == row["before_sha256"], "exact source preimage")
    require(all(compiler_input_matches(Path(name), value)
                for name, value in java["sources"].items()), "all actual compiled Java inputs")
    require(compiled["complete"] is True and compiled["parity_passed"] is True
            and compiled["differences"] == []
            and len(compiled["cases"]) == len(compiled["planned_cases"]) == 87
            and compiled["input_hashes_before"] == compiled["input_hashes_after"]
            and compiled["references_before"] == compiled["references_after"]
            and compiled["inputs_unchanged"] is True and compiled["references_unchanged"] is True
            and compiled["owned_directory_removed"] is True,
            "complete actual original/C++ compiler cohort")
    plans = {row["name"]: row for row in compiled["planned_cases"]}
    require(len(plans) == len({row["name"] for row in compiled["cases"]}) == 87,
            "unique complete compiler selections")
    for row in compiled["cases"]:
        require(all(same(row[key], value) for key, value in plans[row["name"]].items())
                and type(row["original_exit_code"]) is int and type(row["cpp_exit_code"]) is int
                and (row["original_exit_code"] == 0) == (row["cpp_exit_code"] == 0)
                and row["match"] is True
                and (row["original_exit_code"] != 0
                     or row["original_projection_count"] == len(row["cpp_projection"]) == 1),
                "actual compiler outcome and retained relation identity")
    require(sum(row["original_exit_code"] == 0 for row in compiled["cases"]) == 63,
            "63 accepted and 24 refused complete models")
    require(all(java["sources"].get(name) == value
                for name, value in compiled["java_sources"]["cpp"].items())
            and all(Path(name).is_file() and sha(Path(name).read_bytes()) == value
                    for name, value in compiled["java_dependencies"].items())
            and sha(Path(compiled["native_compiler"]["path"]).read_bytes())
                == compiled["native_compiler"]["sha256"],
            "same tested producer sources, dependency closure and actual native compiler")
    labels = [row["label"] for row in compiled["steps"]]
    require(len(labels) == len(set(labels))
            and sum(label.endswith("_original_compile") for label in labels) == 87
            and sum(label.endswith("_cpp_compile") for label in labels) == 87
            and sum(label.endswith("_original_run") for label in labels) == 63,
            "honest compiler/executable process membership")
    preserved, preceding = (records[key] for key in ("preserved", "preceding"))
    require(preserved["complete"] is True and preserved["inputs_unchanged"] is True
            and preserved["source_hashes_before"] == preserved["source_hashes_after"]
            and preserved["owned_directory_removed"] is True
            and preserved["new_cpp_compiler_processes"] == 0
            and preserved["reused_cpp_compiler_outcomes"] == 319
            and len(preserved["cases"]) == len(preceding["cases"]) == 319,
            "fresh preservation of every previous complete SDK/compiler contract")
    require(all(java["sources"].get(name) == value
                for name, value in preserved["source_hashes_before"].items()),
            "previous SDK preservation used this same tested producer")
    for current, prior in zip(preserved["cases"], preceding["cases"], strict=True):
        require(same(current, {**prior, "compiler_outcome_explicitly_reused": True, "actual_compile": False}),
                "exact previous SDK, model and compiler result")
    validate_sdks(records["sdk"])
    validate_sql(records["sql"])
    for key, count, path in (
        ("sdk_controls", 18, "reproduce_const_function_sdks_5_0_6.py"),
        ("sql_controls", 17, "verify_const_function_sql_5_0_6.py"),
    ):
        control = records[key]
        require(control["complete"] is True and control["owned_directory_removed"] is True
                and compiler_input_matches(ROOT / "scripts" / path, control["current_helper_sha256"])
                and len(control["modes"]) == 2
                and {row["result"]["optimized"] for row in control["modes"]} == {0, 1}
                and all(row["exit_code"] == 0 and row["result"]["valid_record_accepted"] is True
                        and len(row["result"]["controls"]) == count
                        and all(item["rejected"] is True for item in row["result"]["controls"])
                        for row in control["modes"]), "ordinary/optimized execution receipt controls")
    reused, prior_sql = (records[key] for key in ("sql_reuse", "prior_sql"))
    require(reused["complete"] is True and reused["inputs_unchanged"] is True
            and reused["owned_directory_removed"] is True and reused["new_sql_executions"] == 0
            and reused["reused_sql_results"] == 40
            and reused["source_sql_sha256"] == RECORDS["prior_sql"][1]
            and len(reused["cases"]) == len(prior_sql["cases"]) == 8,
            "all eight complete prior SQLite consumers explicitly reused")
    for current, prior in zip(reused["cases"], prior_sql["cases"], strict=True):
        require(all(same(current[key], prior[key]) for key in ("name", "input", "model", "cpp_files"))
                and same(current["reused_results"], prior["results"])
                and current["sql_results_explicitly_reused"] is True,
                "complete unchanged SQL SDKs and forty prior actual outcomes")
    transition = records["transition"]
    require(transition["complete"] is True and len(transition["edits"]) == 7,
            "seven exact historical predicate delegation changes")
    source = transition["before_source"]
    for edit in transition["edits"]:
        require(edit["count"] == source.count(edit["before"]) == 1, "exact old source predicate")
        source = source.replace(edit["before"], edit["after"], 1)
    require(source == transition["after_source"]
            and sha(transition["before_source"].encode()) == transition["before_sha256"]
            and sha(source.encode()) == transition["after_sha256"]
            and compiler_input_matches(ROOT / transition["path"], sha(source.encode())),
            "all historical validation and corruption controls preserved")


@cache
def evidence():
    records = {key: bound(key) for key in RECORDS}
    validate(records)
    return records


@cache
def artifact_epochs():
    proposal, sdk, transition = (bound(key) for key in ("proposal", "sdk", "transition"))
    result = {PRODUCER / name: (row["before_sha256"], row["after_sha256"])
              for name, row in proposal["paths"].items() if row["before_sha256"] is not None}
    result.update({ROOT / before["manifest"]: (before["files"][Path(before["manifest"]).name],
                                              after["files"][Path(after["manifest"]).name])
                   for before, after in zip(sdk["before"], sdk["after"], strict=True)})
    result[ROOT / transition["path"]] = (transition["before_sha256"], transition["after_sha256"])
    return result


def current_input_matches(path, expected):
    from compiler_projection_source_admission_5_0_6 import current_input_matches as compiler_input_matches

    epoch = artifact_epochs().get(path)
    if epoch is not None:
        before, after = epoch
        if expected != before:
            return False
        evidence()
        return compiler_input_matches(path, after)
    return compiler_input_matches(path, expected)


def rejection_controls():
    original = evidence()
    controls = (
        ("java", "incomplete_java", lambda r: r.update(complete=False)),
        ("java", "missing_java_case", lambda r: r["cases"].pop()),
        ("java", "wrong_java_result", lambda r: r["cases"][0].update(cpp_resolved=False)),
        ("proposal", "failed_patch_check", lambda r: r.update(apply_check_exit_code=1)),
        ("compiled", "missing_model", lambda r: r["cases"].pop()),
        ("compiled", "boolean_compiler_code", lambda r: r["cases"][0].update(cpp_exit_code=False)),
        ("compiled", "lost_relation", lambda r: r["cases"][0].update(cpp_projection=[])),
        ("compiled", "missing_compiler", lambda r: r["steps"].pop(next(i for i, s in enumerate(r["steps"]) if s["label"].endswith("_cpp_compile")))),
        ("preserved", "missing_previous_sdk", lambda r: r["cases"].pop()),
        ("preserved", "invented_compilation", lambda r: r.update(new_cpp_compiler_processes=1)),
        ("sdk", "missing_retained_sdk", lambda r: r["after"].pop()),
        ("sdk", "extra_metadata", lambda r: r["after"][0]["value"].update(invented=True)),
        ("sdk_controls", "missing_sdk_mode", lambda r: r["modes"].pop()),
        ("sql", "wrong_sql_result", lambda r: r["cases"][0]["results"][0]["cpp"].update(error=False)),
        ("sql", "missing_sql_query", lambda r: r["cases"][0]["results"].pop()),
        ("sql_controls", "missing_sql_control", lambda r: r["modes"][0]["result"]["controls"].pop()),
        ("sql_reuse", "invented_sql_execution", lambda r: r.update(new_sql_executions=40)),
        ("sql_reuse", "changed_previous_sql_sdk", lambda r: r["cases"][0]["cpp_files"].update(invented="0" * 64)),
        ("transition", "lost_historical_predicate", lambda r: r["edits"].pop()),
        ("transition", "changed_historical_body", lambda r: r.update(after_source="pass\n")),
    )
    results = []
    for owner, name, mutate in controls:
        changed = dict(original)
        changed[owner] = copy.deepcopy(changed[owner])
        mutate(changed[owner])
        try:
            validate(changed)
        except RuntimeError as error:
            results.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Constant-function corruption accepted: " + name)
    for path, (before, after) in artifact_epochs().items():
        require(current_input_matches(path, before)
                and not current_input_matches(path, after)
                and not current_input_matches(path, "0" * 64),
                "known source transitions admit only their exact preceding identity")
    results.append(dict(case="82_exact_source_epochs", rejected=True))
    return results
