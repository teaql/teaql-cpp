"""Admit the reviewed standard-value and SmartList source/SDK transition."""

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
        "ap02_standard_smart_list_candidate_5_0_6.json.gz",
        "7913c56de1a85dd347073c6d27d317b7362504a26c3c8df93b38ccae4d4f4d57"
    ],
    "standard": [
        "ap02_standard_value_type_candidate_5_0_6.json.gz",
        "b5cf26c7575594d5bbc826309dae2961a48ef51a411a7023eb9c45175dd05a00"
    ],
    "installed": [
        "ap02_standard_smart_list_installed_5_0_6.json.gz",
        "007895d6a575a9efdcf33900ee40306ecf8e7b7a01dd80776566ddb42c2165fd"
    ],
    "preceding": [
        "ap02_result_error_types_installed_repaired_5_0_6.json.gz",
        "a6446037cdac282b5809b5b63eb236cfeb9e9792a50fa9be60394b551b25ff25"
    ],
    "sdk": [
        "ap02_standard_smart_list_sdks_5_0_6.json.gz",
        "ab6edecf44d52b29b08dee3b98c9694cceb6e1706ae39ad0687a29513db06e68"
    ],
    "base_sdk": [
        "ap02_result_error_types_sdks_5_0_6.json.gz",
        "b952fbdcd7a7ef33f71110558872b1ced270f896f574fafcaaacbc746a405284"
    ],
    "copied_sql": [
        "ap02_standard_smart_list_candidate_sql_5_0_6.json.gz",
        "d704f61fec6a70b4881101e6412e2893fa300e584f96d4428a53df86d55b3dff"
    ],
    "sql_reuse": [
        "ap02_standard_smart_list_production_sql_reuse_5_0_6.json.gz",
        "955c70d47b3aa4fa92cf8ba614cdccf0bb37f4472ff09ce8178e0fabfbc80067"
    ],
    "transition": [
        "ap02_standard_smart_list_helper_transition_5_0_6.json.gz",
        "a04ad2e98bf276e801a379e6bc22de0f4ed335e12f2e83aed53356482402cdc3"
    ]
}


def require(condition, message):
    if not condition:
        raise RuntimeError("Standard/SmartList admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable evidence: " + name)
    return json.loads(gzip.decompress(data))


def validate(candidate, standard, installed, preceding, sdk, base_sdk, copied_sql, sql_reuse, transition):
    require(candidate["complete"] is True and candidate["parity_passed"] is True
            and candidate["installed"] is False and candidate["junit_tests"] == 186
            and len(candidate["cases"]) == 21 and len(candidate["standard_artifact_reuse"]) == 34
            and candidate["standard_candidate_sha256"] == RECORDS["standard"][1]
            and candidate["inputs_unchanged"] is True and candidate["references_unchanged"] is True
            and candidate["owned_directory_removed"] is True
            and len(candidate["candidate_overlays"]) == 1, "complete copied core-type qualification")
    require(standard["complete"] is True and standard["parity_passed"] is True
            and standard["junit_tests"] == 183 and len(standard["cases"]) == 34
            and preceding["complete"] is True and preceding["installed"] is True
            and len(preceding["cases"]) == 84, "complete retained compiler qualifications")
    reused = {row["name"]: row for row in candidate["standard_artifact_reuse"]}
    require(len(reused) == 34, "unique complete standard SDK reuse")
    for row in standard["cases"]:
        actual = reused[row["name"]]
        require(all(actual[key] == row[key] for key in ("name", "model", "type", "cpp_files",
                                                       "original_exit_code", "cpp_exit_code"))
                and actual["compiler_outcomes_explicitly_reused"] is True,
                "complete copied standard model/artifact/result binding")

    require(installed["complete"] is True and installed["installed"] is True
            and installed["whole_goal_complete"] is False and installed["junit_tests"] == 186
            and installed["candidate_sha256"] == RECORDS["candidate"][1]
            and installed["standard_sha256"] == RECORDS["standard"][1]
            and installed["preceding_sha256"] == RECORDS["preceding"][1]
            and installed["new_cpp_compiler_processes"] == 0
            and installed["reused_cpp_compiler_outcomes"] == 139
            and installed["input_hashes_before"] == installed["input_hashes_after"]
            and installed["global_inputs_before"] == installed["global_inputs_after"]
            and installed["global_references_before"] == installed["global_references_after"]
            and installed["inputs_unchanged"] is True and installed["global_inputs_unchanged"] is True
            and installed["global_references_unchanged"] is True
            and installed["owned_directory_removed"] is True, "complete honest production epoch")
    expected = {}
    for prefix, owner, rows, file_key, code_key in (
        ("preserved_", "preceding", preceding["cases"], "files", "installed_exit_code"),
        ("standard_", "standard", standard["cases"], "cpp_files", "cpp_exit_code"),
        ("smart_list_", "candidate", candidate["cases"], "cpp_files", "cpp_exit_code"),
    ):
        for row in rows:
            expected[prefix + row["name"]] = (owner, row, file_key, code_key)
    rows = installed["cases"]
    require(len(rows) == len(expected) == len({row["name"] for row in rows}) == 139
            and {row["name"] for row in rows} == set(expected), "all unique production SDK/compiler selections")
    for row in rows:
        owner, old, file_key, code_key = expected[row["name"]]
        require(row["source_record"] == owner and row["source_case"] == old["name"]
                and row["model"] == old["model"] and row["type"] == old["type"]
                and row["files"] == old[file_key]
                and type(row["reused_cpp_exit_code"]) is int
                and row["reused_cpp_exit_code"] == old[code_key]
                and row["original_exit_code"] == old["original_exit_code"]
                and (row["reused_cpp_exit_code"] == 0) == (row["original_exit_code"] == 0)
                and row["compiler_outcome_explicitly_reused"] is True
                and row["actual_compile"] is False, "exact complete production SDK and retained compiler outcome")
    require(sum(row["reused_cpp_exit_code"] == 0 for row in rows) == 94,
            "ninety-four accepted and forty-five refused contracts")
    steps = {row["label"]: row for row in installed["steps"]}
    required = {"producer_package", "production_java_compile", "production_junit"}
    required.update(row["name"] + "_generate" for row in rows)
    require(len(steps) == len(installed["steps"]) == 142 and set(steps) == required
            and all(row["exit_code"] == 0 for row in installed["steps"]),
            "complete successful production processes without invented C++ compilation")
    overlays = candidate["candidate_overlays"]
    for name, row in overlays.items():
        require(sha(row["before_source"].encode()) == row["before_sha256"]
                and sha(row["after_source"].encode()) == row["after_sha256"]
                and sha(Path(name).read_bytes()) == row["after_sha256"], "exact reviewed installed resolver")
    for name, expected_hash in candidate["candidate_unit_sha256"].items():
        unit = PRODUCER / ("generator/src/test/java/com/skynet/codegenerator/cpp/" + name + ".java")
        require(sha(unit.read_bytes()) == expected_hash, "all eight permanent regression methods")
    for name, expected_hash in installed["input_hashes_before"].items():
        require(sha(Path(name).read_bytes()) == expected_hash, "actual current production input")
    for name, expected_hash in installed["dependency_jars"].items():
        require(sha(Path(name).read_bytes()) == expected_hash, "actual Java dependency")
    compiler = installed["native_compiler"]
    require(compiler == candidate["native_compiler"] == standard["native_compiler"] == preceding["native_compiler"]
            and sha(Path(compiler["path"]).read_bytes()) == compiler["sha256"], "unchanged actual compiler")

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
                        if name != filename), "unchanged complete generated code and Assist")
        value = copy.deepcopy(before["value"])
        for name, row in overlays.items():
            key = Path(name).relative_to(PRODUCER).as_posix()
            require(value["generator_implementation_sha256"][key] == row["before_sha256"],
                    "preceding exact producer fingerprint")
            value["generator_implementation_sha256"][key] = row["after_sha256"]
        require(after["value"] == value and sha(after["text"].encode()) == after["files"][filename],
                "one exact parser fingerprint change")
        directory = (ROOT / after["manifest"]).parent
        require(all(sha((directory / name).read_bytes()) == expected_hash
                    for name, expected_hash in after["files"].items()), "actual current complete SDK bytes")

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
            "actual eight-model SQLite execution and explicit complete production artifact reuse")
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
            "seven exact historical validation import/predicate changes")
    source = transition["before_source"]
    for edit in transition["edits"]:
        require(edit["count"] == source.count(edit["before"]) == 1, "exact historical current-input predicate")
        source = source.replace(edit["before"], edit["after"])
    require(source == transition["after_source"]
            and sha(transition["before_source"].encode()) == transition["before_sha256"]
            and sha(source.encode()) == transition["after_sha256"]
            and (ROOT / transition["path"]).read_text() == source,
            "historical validation bodies and all controls retained")


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
        ("incomplete_installation", 2, lambda r: r.update(complete=False)),
        ("missing_compiler_selection", 2, lambda r: r["cases"].pop()),
        ("wrong_cpp_result", 2, lambda r: r["cases"][0].update(reused_cpp_exit_code=False)),
        ("overstated_cpp_execution", 2, lambda r: r.update(new_cpp_compiler_processes=1)),
        ("changed_generated_file", 2, lambda r: r["cases"][0]["files"].update(invented="0" * 64)),
        ("wrong_source_owner", 2, lambda r: r["cases"][0].update(source_record="candidate")),
        ("incomplete_sdk", 4, lambda r: r["after"].pop()),
        ("extra_metadata_change", 4, lambda r: r["after"][0]["value"].update(invented=True)),
        ("missing_sql_model", 6, lambda r: r["cases"].pop()),
        ("wrong_sql_result", 7, lambda r: r["cases"][0]["reused_results"][0].update(cpp=None)),
        ("overstated_sql_execution", 7, lambda r: r.update(new_sql_executions=40)),
        ("changed_helper_body", 8, lambda r: r.update(after_source="pass\n")),
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
            raise RuntimeError("Standard/SmartList corruption accepted: " + name)
    for path, (_, after) in (next(iter(artifact_epochs().items())), list(artifact_epochs().items())[-1]):
        require(not current_input_matches(path, after) and not current_input_matches(path, "0" * 64),
                "historical epochs cannot be relabeled or invented")
        results.extend([{"case": path.name + ":relabel", "rejected": True},
                        {"case": path.name + ":unknown_epoch", "rejected": True}])
    return results
