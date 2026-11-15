"""Admit the reviewed scalar constant projection source and SDK transition."""

import base64
import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import same
from primitive_const_source_admission_5_0_6 import current_input_matches as primitive_input_matches

RECORDS = {
    "candidate": [
        "ap02_scalar_const_value_candidate_v3_5_0_6.json.gz",
        "9686dcf2f0eb7a42f3e27e8b99bf7b8827e4ecea8f48d15465d7772d31a0ca10"
    ],
    "installed": [
        "ap02_scalar_const_value_installed_5_0_6.json.gz",
        "c194c580dac6a0922084897b6e7df1599ea681d514f9b33e01403920aa535625"
    ],
    "preceding": [
        "ap02_array_const_expression_installed_5_0_6.json.gz",
        "4b4ba9fcddd2e349b746834975cd0da5f4a3122a98ea4c11cacaf39423abf1fe"
    ],
    "sdk": [
        "ap02_scalar_const_value_sdks_5_0_6.json.gz",
        "ff65f4f5934e8aebacad283bee6b555efc7a53a400f557ee655abb8f8483592d"
    ],
    "base_sdk": [
        "ap02_array_const_expression_sdks_repaired_5_0_6.json.gz",
        "291114a3336b941e41e365d450dec5870aa69a573d4a35a89634c7ef6a0805c6"
    ],
    "copied_sql": [
        "ap02_scalar_const_value_candidate_sql_5_0_6.json.gz",
        "b01d4dfd8820675677607a37d3fbb36f34799ba866e14cadfd1deaaeae454350"
    ],
    "sql_reuse": [
        "ap02_scalar_const_value_production_sql_reuse_5_0_6.json.gz",
        "6063f444cebcd2e6d6a1579accb0a94b948661d5484f2d6caad90e24a502a8ad"
    ],
    "transition": [
        "ap02_scalar_const_value_helper_transition_repaired_5_0_6.json.gz",
        "b1bd15ea79424e16710a60d0216f0262b136861919858d7e65d2dcf36666aefc"
    ],
    "partial": [
        "ap02_scalar_const_value_candidate_5_0_6.json.gz",
        "8242c17120fb0ff573cdf1db05d812e9015c7ea4cdf142d89758c358a776875f"
    ],
    "matrix": [
        "ap02_scalar_cast_const_matrix_5_0_6.json.gz",
        "34991adc391729df5ee76ede2eff25167038efb05c515ecef36361d7f75748d3"
    ],
    "literal": [
        "ap02_scalar_const_literal_generation_diagnostic_5_0_6.json.gz",
        "a607fcf8efde5e0c672f817fd6cf3f060539594c1a8aab5ec278bd95321203dd"
    ],
    "expectation_transition": [
        "ap02_scalar_const_value_existing_unit_guard_transition_5_0_6.json.gz",
        "f6a40a445a7bde3a9005098ee8a2197ecef89743b2abf21e4c1e035d5e588c46"
    ]
}


def require(condition, message):
    if not condition:
        raise RuntimeError("Scalar const admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable evidence: " + name)
    return json.loads(gzip.decompress(data))


def validate(candidate, installed, preceding, sdk, base_sdk, copied_sql, sql_reuse,
             transition, partial, matrix, literal, expectation_transition):
    require(candidate["complete"] is True and candidate["parity_passed"] is True
            and candidate["installed"] is False and candidate["whole_goal_complete"] is False
            and candidate["junit_tests"] == 203 and candidate["differences"] == []
            and len(candidate["cases"]) == len(candidate["planned_cases"]) == 65
            and candidate["actual_cpp_contract_compiler_processes"] == 29
            and candidate["fresh_cpp_contract_compiler_processes"] == 29
            and candidate["reused_cpp_compiler_outcomes"] == 36
            and candidate["prior_partial_sha256"] == RECORDS["partial"][1]
            and candidate["cast_matrix_sha256"] == RECORDS["matrix"][1]
            and candidate["literal_generation_sha256"] == RECORDS["literal"][1]
            and candidate["inputs_unchanged"] is True and candidate["references_unchanged"] is True
            and candidate["input_hashes_before"] == candidate["input_hashes_after"]
            and candidate["references_before"] == candidate["references_after"]
            and candidate["owned_directory_removed"] is True,
            "complete scalar source/compiler qualification")
    require(partial["junit_tests"] == 202 and len(partial["cases"]) == 36
            and partial["inputs_unchanged"] is True and partial["references_unchanged"] is True
            and partial["input_hashes_before"] == partial["input_hashes_after"]
            and partial["references_before"] == partial["references_after"]
            and partial["owned_directory_removed"] is True,
            "exact completed subset from the preserved partial controller")
    plans = {row["name"]: row for row in candidate["planned_cases"]}
    prior = {row["name"]: row for row in partial["cases"]}
    rows = candidate["cases"]
    require(len(plans) == len({row["name"] for row in rows}) == 65
            and {row["name"] for row in rows} == set(plans), "all unique planned compiler selections")
    require({row["name"] for row in rows if row.get("compiler_outcomes_explicitly_reused") is True}
            == set(prior), "exact reused compiler membership")
    for row in rows:
        require(all(row[key] == plans[row["name"]][key] for key in ("name", "model", "type"))
                and row["match"] is True and type(row["original_exit_code"]) is int
                and type(row["cpp_exit_code"]) is int
                and (row["original_exit_code"] == 0) == (row["cpp_exit_code"] == 0),
                "complete actual model and original/native compiler binding")
        require(row["cpp_exit_code"] != 0 or len(row["cpp_projection"]) == 1,
                "accepted models retain the reverse projection")
        if row["name"] in prior:
            old = prior[row["name"]]
            require(row["compiler_outcomes_explicitly_reused"] is True
                    and row["source_receipt_sha256"] == RECORDS["partial"][1]
                    and all(row[key] == old[key] for key in
                            ("name", "model", "type", "original_files", "cpp_files",
                             "original_exit_code", "cpp_exit_code", "consumer_source",
                             "consumer_manifest", "consumer_lock")),
                    "honest exact complete SDK/compiler subset reuse")
    steps = {row["label"]: row for row in candidate["steps"]}
    require(len(steps) == len(candidate["steps"])
            and sum(name.endswith("_cpp_compile") for name in steps) == 29,
            "actual compiler process membership")
    require(matrix["complete"] is True and len(matrix["cases"]) == 294
            and matrix["owned_directory_removed"] is True
            and all(type(row["exit_code"]) is int for row in matrix["cases"]),
            "complete actual const-cast observations")
    resource = candidate["candidate_resource"]
    resource_bytes = base64.b64decode(resource["content_base64"], validate=True)
    require(resource["source_reference_sha256"] == RECORDS["matrix"][1]
            and sha(resource_bytes) == resource["sha256"]
            and sha((PRODUCER / resource["path"]).read_bytes()) == resource["sha256"],
            "immutable source-derived cast resource")
    corpus = json.loads(gzip.decompress(resource_bytes))
    require(corpus["source_reference_sha256"] == RECORDS["matrix"][1]
            and corpus["cases"] == [
                dict(expression=row["expression"], accepted=row["exit_code"] == 0)
                for row in matrix["cases"]],
            "every resource expectation follows the actual Rust observation")
    require(literal["complete"] is True and len(literal["cases"]) == 9
            and all(type(row["original_exit_code"]) is int and row["original_exit_code"] == 0
                    for row in literal["cases"]), "actual generated-context literal witnesses")
    require(preceding["complete"] is True and preceding["installed"] is True
            and preceding["junit_tests"] == 194 and len(preceding["cases"]) == 193,
            "all preserved array-constant and earlier compiler contracts")
    require(installed["complete"] is True and installed["installed"] is True
            and installed["whole_goal_complete"] is False and installed["junit_tests"] == 203
            and installed["candidate_sha256"] == RECORDS["candidate"][1]
            and installed["preceding_sha256"] == RECORDS["preceding"][1]
            and installed["new_cpp_compiler_processes"] == 0
            and installed["reused_cpp_compiler_outcomes"] == 258
            and installed["input_hashes_before"] == installed["input_hashes_after"]
            and installed["global_inputs_before"] == installed["global_inputs_after"]
            and installed["global_references_before"] == installed["global_references_after"]
            and installed["inputs_unchanged"] is True and installed["global_inputs_unchanged"] is True
            and installed["global_references_unchanged"] is True
            and installed["owned_directory_removed"] is True,
            "complete production Java and honest complete SDK/compiler reuse")
    expected = {}
    for prefix, owner, selections, file_key, code_key in (
        ("preserved_", "preceding", preceding["cases"], "files", "reused_cpp_exit_code"),
        ("scalar_", "candidate", candidate["cases"], "cpp_files", "cpp_exit_code"),
    ):
        for row in selections:
            expected[prefix + row["name"]] = (owner, row, file_key, code_key)
    rows = installed["cases"]
    require(len(rows) == len(expected) == len({row["name"] for row in rows}) == 258
            and {row["name"] for row in rows} == set(expected), "all production compiler contracts")
    for row in rows:
        owner, old, file_key, code_key = expected[row["name"]]
        require(row["source_record"] == owner and row["source_case"] == old["name"]
                and row["model"] == old["model"] and row["type"] == old["type"]
                and row["files"] == old[file_key]
                and type(row["reused_cpp_exit_code"]) is int
                and row["reused_cpp_exit_code"] == old[code_key]
                and row["original_exit_code"] == old["original_exit_code"]
                and (row["reused_cpp_exit_code"] == 0) == (row["original_exit_code"] == 0)
                and row["compiler_outcome_explicitly_reused"] is True and row["actual_compile"] is False,
                "exact complete production SDK and retained compiler result")
    steps = {row["label"]: row for row in installed["steps"]}
    required = {"producer_package", "production_java_compile", "production_junit"}
    required.update(row["name"] + "_generate" for row in rows)
    require(len(steps) == len(installed["steps"]) == 261 and set(steps) == required
            and all(row["exit_code"] == 0 for row in installed["steps"]),
            "all successful production processes without invented compilation")
    overlays = candidate["candidate_overlays"]
    require(len(overlays) == 1, "one reviewed production parser")
    for name, row in overlays.items():
        require(sha(row["before_source"].encode()) == row["before_sha256"]
                and sha(row["after_source"].encode()) == row["after_sha256"]
                and primitive_input_matches(Path(name), row["after_sha256"]), "exact reviewed installed parser")
    unit = PRODUCER / "generator/src/test/java/com/skynet/codegenerator/cpp/CppScalarConstValueProjectionTest.java"
    require(sha(unit.read_bytes()) == candidate["candidate_unit_sha256"], "nine permanent regressions")
    require(sha(candidate["candidate_unit_source"].encode()) == candidate["candidate_unit_sha256"],
            "retained permanent regression source")
    test_overlays = candidate["candidate_test_overlays"]
    require(len(test_overlays) == 1, "one existing expectation follows generated-context evidence")
    for name, row in test_overlays.items():
        require(sha(row["before_source"].encode()) == row["before_sha256"]
                and sha(row["after_source"].encode()) == row["after_sha256"]
                and sha(Path(name).read_bytes()) == row["after_sha256"],
                "exact existing-test expectation transition")
    for name, value in installed["input_hashes_before"].items():
        require(primitive_input_matches(Path(name), value), "actual current production input")
    for name, value in installed["dependency_jars"].items():
        require(sha(Path(name).read_bytes()) == value, "actual Java dependency")
    compiler = installed["native_compiler"]
    require(compiler == candidate["native_compiler"] == partial["native_compiler"] == preceding["native_compiler"]
            and sha(Path(compiler["path"]).read_bytes()) == compiler["sha256"],
            "unchanged actual native compiler")
    require(sdk["complete"] is True and base_sdk["complete"] is True
            and sdk["installed_sha256"] == RECORDS["installed"][1]
            and sdk["baseline_sha256"] == RECORDS["base_sdk"][1]
            and sdk["before"] == base_sdk["after"] and sdk["producer_sources_unchanged"] is True
            and sdk["owned_directory_removed"] is True
            and len(sdk["before"]) == len(sdk["after"]) == len(sdk["cases"]) == 80
            and len(sdk["steps"]) == 6 and all(row["exit_code"] == 0 for row in sdk["steps"]),
            "complete eighty-SDK production refresh and independent checks")
    for before, after, outcome in zip(sdk["before"], sdk["after"], sdk["cases"], strict=True):
        filename = Path(after["manifest"]).name
        require(before["manifest"] == after["manifest"] == outcome["manifest"]
                and set(before["files"]) == set(after["files"])
                and outcome["changed_files"] == [filename] and outcome["code_and_assist_identical"] is True
                and all(value == after["files"][name] for name, value in before["files"].items()
                        if name != filename), "unchanged complete generated implementation and Assist")
        value = copy.deepcopy(before["value"])
        for name, row in overlays.items():
            key = Path(name).relative_to(PRODUCER).as_posix()
            require(value["generator_implementation_sha256"][key] == row["before_sha256"],
                    "exact preceding parser fingerprint")
            value["generator_implementation_sha256"][key] = row["after_sha256"]
        require(after["value"] == value and sha(after["text"].encode()) == after["files"][filename],
                "one exact parser fingerprint change")
        directory = (ROOT / after["manifest"]).parent
        require(all(primitive_input_matches(directory / name, value) for name, value in after["files"].items()),
                "actual current complete SDK bytes")
    require(copied_sql["complete"] is True and copied_sql["installed"] is False
            and copied_sql["candidate_sha256"] == RECORDS["candidate"][1]
            and copied_sql["candidate_overlays"] == overlays and copied_sql["differences"] == []
            and copied_sql["compiled_cpp_inputs_unchanged"] is True and copied_sql["inputs_unchanged"] is True
            and copied_sql["global_inputs_unchanged"] is True and copied_sql["global_references_unchanged"] is True
            and copied_sql["owned_directory_removed"] is True
            and sql_reuse["complete"] is True and sql_reuse["installed"] is True
            and sql_reuse["source_sql_sha256"] == RECORDS["copied_sql"][1]
            and sql_reuse["installed_sha256"] == RECORDS["installed"][1]
            and sql_reuse["new_sql_executions"] == 0 and sql_reuse["reused_sql_results"] == 40
            and sql_reuse["inputs_unchanged"] is True and sql_reuse["references_unchanged"] is True
            and sql_reuse["owned_directory_removed"] is True
            and len(copied_sql["cases"]) == len(sql_reuse["cases"]) == 8
            and all(row["exit_code"] == 0 for receipt in (copied_sql, sql_reuse) for row in receipt["steps"]),
            "actual eight-model SQLite execution and explicit complete production SDK reuse")
    for actual, old in zip(sql_reuse["cases"], copied_sql["cases"], strict=True):
        require(all(actual[key] == old[key] for key in ("name", "input", "model", "cpp_files"))
                and actual["reused_results"] == old["results"] and len(old["results"]) == 5,
                "complete source/model/SDK and forty-result binding")
        require(all(row["match"] is True and same(row["original"], row["cpp"]) for row in old["results"]),
                "strict query, E and JSON equivalence")
    normalized = {}
    for name, value in copied_sql["compiled_cpp_java"].items():
        path = Path(name)
        for original_name in overlays:
            if path.name == Path(original_name).name:
                path = Path(original_name)
                break
        require(primitive_input_matches(path, value), "actual copied/production compilation source")
        normalized[str(path)] = value
    require(sql_reuse["compiled_cpp_java"] == normalized
            and sql_reuse["dependency_jars"] == installed["dependency_jars"]
            and sql_reuse["input_hashes_before"] == sql_reuse["input_hashes_after"],
            "complete SQL compilation/dependency/input identity")
    for name, value in copied_sql["input_hashes_before"].items():
        path = Path(name)
        if name == copied_sql["adapted_driver_path"]:
            require(sha(copied_sql["adapted_driver_source"].encode()) == value, "retained actual SQL controller")
        elif name in overlays:
            require(value == overlays[name]["before_sha256"], "declared unused prior production source")
        else:
            require(primitive_input_matches(path, value), "unchanged actual SQL source input")
    require(transition["complete"] is True and len(transition["edits"]) == 8,
            "eight exact historical validation import/predicate edits")
    source = transition["before_source"]
    for edit in transition["edits"]:
        require(edit["count"] == source.count(edit["before"]) == 1, "exact historical current-input predicate")
        source = source.replace(edit["before"], edit["after"])
    require(source == transition["after_source"]
            and sha(transition["before_source"].encode()) == transition["before_sha256"]
            and sha(source.encode()) == transition["after_sha256"]
            and (ROOT / transition["path"]).read_text() == source,
            "all historical validation bodies and controls retained")


    require(expectation_transition["complete"] is True
            and len(expectation_transition["edits"]) == 1,
            "one historical existing-unit predicate follows its declared source epoch")
    expected_source = expectation_transition["before_source"]
    for edit in expectation_transition["edits"]:
        require(edit["count"] == expected_source.count(edit["before"]) == 1,
                "exact historical unit predicate")
        expected_source = expected_source.replace(edit["before"], edit["after"], 1)
    require(expected_source == expectation_transition["after_source"]
            and sha(expectation_transition["before_source"].encode()) == expectation_transition["before_sha256"]
            and sha(expected_source.encode()) == expectation_transition["after_sha256"]
            and (ROOT / expectation_transition["path"]).read_text() == expected_source,
            "historical unit guard retains all other validation and controls")


@cache
def evidence():
    values = tuple(bound(key) for key in RECORDS)
    validate(*values)
    return values


@cache
def artifact_epochs():
    candidate, sdk = bound("candidate"), bound("sdk")
    result = {Path(name): (row["before_sha256"], row["after_sha256"])
              for name, row in {**candidate["candidate_overlays"], **candidate["candidate_test_overlays"]}.items()}
    transition = bound("expectation_transition")
    result[ROOT / transition["path"]] = (
        transition["before_sha256"], transition["after_sha256"])
    result.update({ROOT / before["manifest"]: (before["files"][Path(before["manifest"]).name],
                                              after["files"][Path(after["manifest"]).name])
                   for before, after in zip(sdk["before"], sdk["after"], strict=True)})
    return result


def current_input_matches(path, expected):
    epoch = artifact_epochs().get(path)
    if epoch is not None:
        before, after = epoch
        if expected != before:
            return False
        evidence()
        return primitive_input_matches(path, after)
    return primitive_input_matches(path, expected)


def rejection_controls():
    original = evidence()
    controls = (
        ("incomplete_installation", 1, lambda r: r.update(complete=False)),
        ("missing_compiler_selection", 1, lambda r: r["cases"].pop()),
        ("wrong_cpp_result", 1, lambda r: r["cases"][0].update(reused_cpp_exit_code=False)),
        ("overstated_cpp_execution", 1, lambda r: r.update(new_cpp_compiler_processes=1)),
        ("changed_generated_file", 1, lambda r: r["cases"][0]["files"].update(invented="0" * 64)),
        ("wrong_source_owner", 1, lambda r: r["cases"][0].update(source_record="candidate")),
        ("incomplete_sdk", 3, lambda r: r["after"].pop()),
        ("extra_metadata_change", 3, lambda r: r["after"][0]["value"].update(invented=True)),
        ("missing_sql_model", 5, lambda r: r["cases"].pop()),
        ("wrong_sql_result", 6, lambda r: r["cases"][0]["reused_results"][0].update(cpp=None)),
        ("overstated_sql_execution", 6, lambda r: r.update(new_sql_executions=40)),
        ("changed_helper_body", 7, lambda r: r.update(after_source="pass\n")),
        ("wrong_projection_effect", 0, lambda r: r["cases"][0].update(cpp_projection=[])),
        ("missing_partial_compiler_result", 8, lambda r: r["cases"].pop()),
        ("wrong_cast_observation", 9, lambda r: r["cases"][0].update(exit_code=1)),
        ("wrong_cast_source_link", 0, lambda r: r.update(cast_matrix_sha256="0" * 64)),
        ("wrong_cast_resource", 0, lambda r: r["candidate_resource"].update(sha256="0" * 64)),
        ("wrong_existing_test_source", 0, lambda r: next(iter(r["candidate_test_overlays"].values())).update(after_source="pass\\n")),
        ("missing_existing_test", 0, lambda r: r.update(candidate_test_overlays={})),
        ("fabricated_compiler_reuse", 0, lambda r: r.update(reused_cpp_compiler_outcomes=65)),
        ("wrong_generated_literal", 10, lambda r: r["cases"][0].update(original_exit_code=1)),
        ("wrong_existing_test_epoch", 0, lambda r: next(iter(r["candidate_test_overlays"].values())).update(before_sha256="0" * 64)),
        ("changed_historical_unit_guard", 11, lambda r: r.update(after_source="pass\n")),
    )
    results = []
    for name, index, mutate in controls:
        changed = list(original)
        changed[index] = copy.deepcopy(changed[index])
        mutate(changed[index])
        try:
            validate(*changed)
        except RuntimeError as error:
            results.append({"case": name, "rejected": True, "error": str(error)})
        else:
            raise RuntimeError("Scalar const corruption accepted: " + name)
    epochs = list(artifact_epochs().items())
    for path, (_, after) in (epochs[0], epochs[1], epochs[2], epochs[-1]):
        require(not current_input_matches(path, after) and not current_input_matches(path, "0" * 64),
                "historical epochs cannot be relabeled or invented")
        results.extend([{"case": path.name + ":relabel", "rejected": True},
                        {"case": path.name + ":unknown_epoch", "rejected": True}])
    return results
