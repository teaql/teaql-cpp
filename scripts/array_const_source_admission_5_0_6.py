"""Admit the reviewed array constant projection source and SDK transition."""

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

RECORDS = {
    "candidate": [
        "ap02_array_const_expression_candidate_v3_5_0_6.json.gz",
        "813407b1ca0495b1d059245ed33f203aa848eb1955b412f0b05fd0d6943697c5"
    ],
    "installed": [
        "ap02_array_const_expression_installed_5_0_6.json.gz",
        "4b4ba9fcddd2e349b746834975cd0da5f4a3122a98ea4c11cacaf39423abf1fe"
    ],
    "preceding": [
        "ap02_standard_smart_list_installed_5_0_6.json.gz",
        "007895d6a575a9efdcf33900ee40306ecf8e7b7a01dd80776566ddb42c2165fd"
    ],
    "sdk": [
        "ap02_array_const_expression_sdks_repaired_5_0_6.json.gz",
        "291114a3336b941e41e365d450dec5870aa69a573d4a35a89634c7ef6a0805c6"
    ],
    "base_sdk": [
        "ap02_standard_smart_list_sdks_5_0_6.json.gz",
        "ab6edecf44d52b29b08dee3b98c9694cceb6e1706ae39ad0687a29513db06e68"
    ],
    "copied_sql": [
        "ap02_array_const_expression_candidate_sql_5_0_6.json.gz",
        "d0eabc5a38e227a437718bc588d8f68ca50d65fd9ba15dfe37df1ac95425421e"
    ],
    "sql_reuse": [
        "ap02_array_const_expression_production_sql_reuse_5_0_6.json.gz",
        "6525e593642fa4d5c03677eacb0f510de3f59ef27667ad2e75fe278a570e7ac5"
    ],
    "transition": [
        "ap02_array_const_expression_helper_transition_5_0_6.json.gz",
        "54768ea1f45a1ada8735700087d0c00d8c0e3044ac65ebbefa8f0030576074b2"
    ],
    "partial": [
        "ap02_array_const_expression_candidate_v2_5_0_6.json.gz",
        "c5ea1f2bdd66f66374d5025c73245d6e8568facea8ed85710809eed06a0237b2"
    ]
}


def require(condition, message):
    if not condition:
        raise RuntimeError("Array const admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable evidence: " + name)
    return json.loads(gzip.decompress(data))


def validate(candidate, installed, preceding, sdk, base_sdk, copied_sql, sql_reuse, transition, partial):
    require(candidate["complete"] is True and candidate["parity_passed"] is True
            and candidate["installed"] is False and candidate["whole_goal_complete"] is False
            and candidate["junit_tests"] == 194 and candidate["differences"] == []
            and len(candidate["cases"]) == len(candidate["planned_cases"]) == 54
            and candidate["actual_cpp_compiler_processes"] == 42
            and candidate["new_cpp_compiler_processes"] == 42
            and candidate["actual_descriptor_consumers"] == 5
            and candidate["reused_cpp_compiler_outcomes"] == 12
            and candidate["prior_partial_sha256"] == RECORDS["partial"][1]
            and candidate["inputs_unchanged"] is True and candidate["references_unchanged"] is True
            and candidate["input_hashes_before"] == candidate["input_hashes_after"]
            and candidate["references_before"] == candidate["references_after"]
            and candidate["owned_directory_removed"] is True,
            "complete revised compiler and actual metadata consumer evidence")
    require(partial["junit_tests"] == 194 and len(partial["cases"]) == 12
            and partial["inputs_unchanged"] is True and partial["references_unchanged"] is True
            and partial["input_hashes_before"] == partial["input_hashes_after"]
            and partial["references_before"] == partial["references_after"]
            and partial["owned_directory_removed"] is True
            and partial["candidate_overlays"] == candidate["candidate_overlays"],
            "exact completed subset from the intentionally stopped controller")
    plans = {row["name"]: row for row in candidate["planned_cases"]}
    prior = {row["name"]: row for row in partial["cases"]}
    rows = candidate["cases"]
    require(len(plans) == len({row["name"] for row in rows}) == 54
            and {row["name"] for row in rows} == set(plans), "all unique planned compiler selections")
    for row in rows:
        require(all(row[key] == plans[row["name"]][key] for key in ("name", "model", "type"))
                and row["match"] is True and type(row["original_exit_code"]) is int
                and type(row["cpp_exit_code"]) is int
                and (row["original_exit_code"] == 0) == (row["cpp_exit_code"] == 0),
                "complete actual model and original/native compiler binding")
        if row["name"] in prior:
            old = prior[row["name"]]
            require(row["compiler_outcomes_explicitly_reused"] is True
                    and row["source_receipt_sha256"] == RECORDS["partial"][1]
                    and all(row[key] == old[key] for key in
                            ("name", "model", "type", "original_files", "cpp_files",
                             "original_exit_code", "cpp_exit_code")),
                    "honest exact complete SDK/compiler subset reuse")
    steps = {row["label"]: row for row in candidate["steps"]}
    require(len(steps) == len(candidate["steps"])
            and sum(name.endswith("_cpp_compile") for name in steps) == 42
            and sum(name.endswith("_cpp_descriptor_run") for name in steps) == 5,
            "actual compiler and descriptor process membership")
    metadata_names = {"bitwise_and_comparison", "bitwise_or_comparison",
                      "bitwise_xor_comparison", "chained_comparison", "parenthesized_comparison"}
    for row in rows:
        if row["name"] in metadata_names:
            expected = "Item|0|17\n" if row["name"] == "chained_comparison" else "Item|1|21\n"
            require(row["original_exit_code"] == row["cpp_exit_code"] == 0
                    and row["original_descriptor_stdout"] == expected
                    and steps[row["name"] + "_original_run"]["stdout"] == expected
                    and steps[row["name"] + "_cpp_descriptor_run"]["stdout"] == expected
                    and steps[row["name"] + "_cpp_descriptor_link"]["exit_code"] == 0,
                    "actual descriptor relation and partial attribute effects")
    require(preceding["complete"] is True and preceding["installed"] is True
            and preceding["junit_tests"] == 186 and len(preceding["cases"]) == 139,
            "all preserved standard-value and earlier compiler contracts")
    require(installed["complete"] is True and installed["installed"] is True
            and installed["whole_goal_complete"] is False and installed["junit_tests"] == 194
            and installed["candidate_sha256"] == RECORDS["candidate"][1]
            and installed["preceding_sha256"] == RECORDS["preceding"][1]
            and installed["new_cpp_compiler_processes"] == 0
            and installed["reused_cpp_compiler_outcomes"] == 193
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
        ("array_", "candidate", candidate["cases"], "cpp_files", "cpp_exit_code"),
    ):
        for row in selections:
            expected[prefix + row["name"]] = (owner, row, file_key, code_key)
    rows = installed["cases"]
    require(len(rows) == len(expected) == len({row["name"] for row in rows}) == 193
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
    require(len(steps) == len(installed["steps"]) == 196 and set(steps) == required
            and all(row["exit_code"] == 0 for row in installed["steps"]),
            "all successful production processes without invented compilation")
    overlays = candidate["candidate_overlays"]
    require(len(overlays) == 1, "one reviewed production parser")
    for name, row in overlays.items():
        require(sha(row["before_source"].encode()) == row["before_sha256"]
                and sha(row["after_source"].encode()) == row["after_sha256"]
                and sha(Path(name).read_bytes()) == row["after_sha256"], "exact reviewed installed parser")
    unit = PRODUCER / "generator/src/test/java/com/skynet/codegenerator/cpp/CppArrayConstExpressionProjectionTest.java"
    require(sha(unit.read_bytes()) == candidate["candidate_unit_sha256"], "eight permanent regressions")
    for name, value in installed["input_hashes_before"].items():
        require(sha(Path(name).read_bytes()) == value, "actual current production input")
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
        require(all(sha((directory / name).read_bytes()) == value for name, value in after["files"].items()),
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
        require(sha(path.read_bytes()) == value, "actual copied/production compilation source")
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
            require(path.is_file() and sha(path.read_bytes()) == value, "unchanged actual SQL source input")
    require(transition["complete"] is True and len(transition["edits"]) == 7,
            "seven exact historical validation import/predicate edits")
    source = transition["before_source"]
    for edit in transition["edits"]:
        require(edit["count"] == source.count(edit["before"]) == 1, "exact historical current-input predicate")
        source = source.replace(edit["before"], edit["after"])
    require(source == transition["after_source"]
            and sha(transition["before_source"].encode()) == transition["before_sha256"]
            and sha(source.encode()) == transition["after_sha256"]
            and (ROOT / transition["path"]).read_text() == source,
            "all historical validation bodies and controls retained")


@cache
def evidence():
    values = tuple(bound(key) for key in RECORDS)
    validate(*values)
    return values


@cache
def artifact_epochs():
    candidate, sdk = bound("candidate"), bound("sdk")
    result = {Path(name): (row["before_sha256"], row["after_sha256"])
              for name, row in candidate["candidate_overlays"].items()}
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
        return sha(path.read_bytes()) == after
    return path.is_file() and sha(path.read_bytes()) == expected


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
        ("wrong_descriptor_effect", 0, lambda r: r["cases"][0].update(original_descriptor_stdout="Item|0|17\n")),
        ("missing_partial_compiler_result", 8, lambda r: r["cases"].pop()),
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
            raise RuntimeError("Array const corruption accepted: " + name)
    for path, (_, after) in (next(iter(artifact_epochs().items())), list(artifact_epochs().items())[-1]):
        require(not current_input_matches(path, after) and not current_input_matches(path, "0" * 64),
                "historical epochs cannot be relabeled or invented")
        results.extend([{"case": path.name + ":relabel", "rejected": True},
                        {"case": path.name + ":unknown_epoch", "rejected": True}])
    return results
