"""Admit the reviewed container-projection source and SDK epoch explicitly."""

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
    "candidate": ("ap02_container_entity_types_candidate_v2_5_0_6.json.gz",
                  "7ff7b8f387d017679c3b661d630cfa4a4823a29a6f873475ebe20b7078ae7e41"),
    "replay": ("ap02_container_entity_types_candidate_v2_reuse_5_0_6.json.gz",
               "b0e9d685414528c9b8113539e72fa67d201643c8f8b1c9de690e683e053f7aaf"),
    "installed": ("ap02_container_entity_types_installed_5_0_6.json.gz",
                  "c12ed4e7237527f623e34925bf717a87fe2d18e2a08a7cf9cd2bd89eae27e868"),
    "sdk": ("ap02_container_entity_types_sdks_5_0_6.json.gz",
            "6479659a4f793fea9aaa66d92846bf7ec6abc64a863c5431062045b9235bb7a9"),
    "base_sdk": ("ap02_qualified_entity_types_sdks_5_0_6.json.gz",
                 "021228757678400605ddb0ea4cc63c29ffcb864a04501d5b863e88d888e2084b"),
    "copied_sql": ("ap02_container_entity_types_candidate_sql_repaired_5_0_6.json.gz",
                   "4e3b2c8f0e201b7a73ccb1779a8ac6d68a8d3e568eb7031c730f3ae06ba222f0"),
    "sql_reuse": ("ap02_container_entity_types_production_sql_reuse_5_0_6.json.gz",
                  "70f6297e7138c39385c5882de16de7f49bc237c517452b38d3f05174e0074ddb"),
    "transition": ("ap02_container_entity_types_helper_transition_5_0_6.json.gz",
                   "5ce06e9309000a2d494213fe0cfa1fa22c08d01bf8d57481f23fa1d94aabc311"),
}


def require(condition, message):
    if not condition:
        raise RuntimeError("Container-type admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable evidence identity: " + name)
    return json.loads(gzip.decompress(data))


def validate(candidate, replay, installed, sdk, base_sdk, copied_sql, sql_reuse, transition):
    require(candidate["complete"] is True and candidate["parity_passed"] is True
            and candidate["installed"] is False and candidate["junit_tests"] == 173
            and replay["complete"] is True and replay["new_cpp_compiler_processes"] == 0
            and replay["corrected_candidate_sha256"] == RECORDS["candidate"][1]
            and len(replay["cases"]) == 46 and len(candidate["cases"]) == 10,
            "complete copied compiler and exact artifact witnesses")
    require(installed["complete"] is True and installed["installed"] is True
            and installed["whole_goal_complete"] is False and installed["junit_tests"] == 173
            and installed["candidate_sha256"] == RECORDS["candidate"][1]
            and installed["artifact_replay_sha256"] == RECORDS["replay"][1]
            and installed["producer_commit"] == "3187dad4bf3779ea361b250cb77f987296b6919b"
            and installed["input_hashes_before"] == installed["input_hashes_after"]
            and installed["global_inputs_before"] == installed["global_inputs_after"]
            and installed["global_references_before"] == installed["global_references_after"]
            and installed["inputs_unchanged"] is True and installed["global_inputs_unchanged"] is True
            and installed["global_references_unchanged"] is True
            and installed["owned_directory_removed"] is True, "actual complete production epoch")
    expected = {"matrix_" + row["name"]: (row["model"], row["generated_files"], row["reused_original_exit_code"])
                for row in replay["cases"]}
    expected.update({"lifetime_" + row["name"]: (row["model"], row["cpp_files"], row["original_exit_code"])
                     for row in candidate["cases"]})
    rows = installed["cases"]
    steps = {row["label"]: row for row in installed["steps"]}
    require(len(expected) == len(rows) == len({row["name"] for row in rows}) == 56
            and {row["name"] for row in rows} == set(expected)
            and len(steps) == len(installed["steps"]), "complete unique compiler selections")
    for row in rows:
        model, generated, original_code = expected[row["name"]]
        actual = steps[row["name"] + "_compile"]
        require(row["model"] == model and row["files"] == generated
                and type(row["installed_exit_code"]) is int
                and row["installed_exit_code"] == actual["exit_code"]
                and row["original_exit_code"] == original_code and row["match"] is True
                and (row["installed_exit_code"] == 0) == (original_code == 0),
                "actual original/production result and whole SDK identity")
    require(sum(row["installed_exit_code"] == 0 for row in rows) == 40
            and all(row["exit_code"] == 0 for row in installed["steps"]
                    if not row["label"].endswith("_compile")
                    or row["label"] == "production_fresh_java_compile"),
            "forty accepted/sixteen refused types and successful preparation")
    overlays = candidate["candidate_overlays"]
    require(len(overlays) == 1, "one reviewed parser implementation change")
    for name, row in overlays.items():
        require(sha(row["before_source"].encode()) == row["before_sha256"]
                and sha(row["after_source"].encode()) == row["after_sha256"]
                and sha(Path(name).read_bytes()) == row["after_sha256"], "exact installed parser")
    unit = PRODUCER / "generator/src/test/java/com/skynet/codegenerator/cpp/CppContainerTypeProjectionTest.java"
    require(sha(unit.read_bytes()) == candidate["candidate_unit_sha256"], "permanent regressions")
    for name, value in installed["input_hashes_before"].items():
        require(sha(Path(name).read_bytes()) == value, "current production compilation input")
    for name, value in installed["dependency_jars"].items():
        require(sha(Path(name).read_bytes()) == value, "current Java dependency")
    compiler = installed["native_compiler"]
    require(sha(Path(compiler["path"]).read_bytes()) == compiler["sha256"], "actual compiler identity")

    require(base_sdk["complete"] is True and sdk["complete"] is True
            and sdk["installed_sha256"] == RECORDS["installed"][1]
            and sdk["before"] == base_sdk["after"] and sdk["producer_sources_unchanged"] is True
            and sdk["owned_directory_removed"] is True
            and len(sdk["before"]) == len(sdk["after"]) == len(sdk["cases"]) == 80
            and len(sdk["steps"]) == 6 and all(row["exit_code"] == 0 for row in sdk["steps"]),
            "complete eighty-SDK production regeneration and checks")
    for before, after, outcome in zip(sdk["before"], sdk["after"], sdk["cases"], strict=True):
        filename = Path(after["manifest"]).name
        require(before["manifest"] == after["manifest"] == outcome["manifest"]
                and set(before["files"]) == set(after["files"])
                and outcome["changed_files"] == [filename]
                and outcome["code_and_assist_identical"] is True
                and all(value == after["files"][name] for name, value in before["files"].items()
                        if name != filename), "only metadata bytes change in each complete SDK")
        value = copy.deepcopy(before["value"])
        for name, row in overlays.items():
            key = Path(name).relative_to(PRODUCER).as_posix()
            require(value["generator_implementation_sha256"][key] == row["before_sha256"],
                    "exact preceding SDK producer epoch")
            value["generator_implementation_sha256"][key] = row["after_sha256"]
        require(after["value"] == value and sha(after["text"].encode()) == after["files"][filename],
                "one parser fingerprint and complete serialized metadata")
        directory = (ROOT / after["manifest"]).parent
        require(all(sha((directory / name).read_bytes()) == value for name, value in after["files"].items()),
                "actual current complete SDK bytes")

    require(copied_sql["complete"] is True and copied_sql["installed"] is False
            and copied_sql["candidate_sha256"] == RECORDS["candidate"][1]
            and copied_sql["differences"] == [] and copied_sql["compiled_cpp_inputs_unchanged"] is True
            and copied_sql["owned_directory_removed"] is True
            and sql_reuse["complete"] is True and sql_reuse["installed"] is True
            and sql_reuse["source_sql_sha256"] == RECORDS["copied_sql"][1]
            and sql_reuse["installed_sha256"] == RECORDS["installed"][1]
            and sql_reuse["new_sql_executions"] == 0 and sql_reuse["reused_sql_results"] == 30
            and sql_reuse["inputs_unchanged"] is True and sql_reuse["references_unchanged"] is True
            and sql_reuse["owned_directory_removed"] is True
            and len(copied_sql["cases"]) == len(sql_reuse["cases"]) == 6
            and all(row["exit_code"] == 0 for source in (copied_sql, sql_reuse) for row in source["steps"]),
            "actual six-model SQLite execution and honest production artifact reuse")
    for actual, old in zip(sql_reuse["cases"], copied_sql["cases"], strict=True):
        require(all(actual[key] == old[key] for key in ("name", "input", "model", "cpp_files"))
                and actual["reused_results"] == old["results"] and len(old["results"]) == 5,
                "complete source/model/SDK/runtime-result binding")
        require(all(row["match"] is True and same(row["original"], row["cpp"]) for row in old["results"]),
                "strict original/C++ query, E and JSON results")
    require(sql_reuse["input_hashes_before"] == sql_reuse["input_hashes_after"]
            and sql_reuse["dependency_jars"] == installed["dependency_jars"], "SQL input/dependency epoch")
    normalized = {}
    for name, value in copied_sql["compiled_cpp_java"].items():
        path = Path(name)
        if path.name == "CppRustLiteralMetadata.java":
            path = PRODUCER / "generator/src/main/java/com/skynet/codegenerator/cpp/CppRustLiteralMetadata.java"
        require(sha(path.read_bytes()) == value, "actual copied/production Java identity")
        normalized[str(path)] = value
    require(sql_reuse["compiled_cpp_java"] == normalized, "complete current SQL generator source set")
    for name, value in copied_sql["input_hashes_before"].items():
        path = Path(name)
        if name == copied_sql["adapted_driver_path"]:
            require(sha(copied_sql["adapted_driver_source"].encode()) == value,
                    "retained actual SQL execution controller")
        elif path.name == "CppRustLiteralMetadata.java" and path.is_relative_to(PRODUCER):
            require(value == next(iter(overlays.values()))["before_sha256"],
                    "declared unused prior source; actual compiled overlay checked separately")
        else:
            require(path.is_file() and sha(path.read_bytes()) == value,
                    "unchanged actual SQL source/library/dependency")

    require(transition["complete"] is True and len(transition["edits"]) == 6,
            "bounded preceding admission transition")
    source = transition["before_source"]
    for edit in transition["edits"]:
        require(edit["count"] == source.count(edit["before"]) == 1, "exact current-input predicate")
        source = source.replace(edit["before"], edit["after"])
    require(source == transition["after_source"]
            and sha(transition["before_source"].encode()) == transition["before_sha256"]
            and sha(source.encode()) == transition["after_sha256"]
            and (ROOT / transition["path"]).read_text() == source,
            "historical validation bodies and controls retained")


@cache
def evidence():
    values = tuple(bound(key) for key in RECORDS)
    validate(*values)
    return values


@cache
def artifact_epochs():
    sdk = bound("sdk")
    result = {ROOT / before["manifest"]: (before["files"][Path(before["manifest"]).name],
                                         after["files"][Path(after["manifest"]).name])
              for before, after in zip(sdk["before"], sdk["after"], strict=True)}
    candidate = bound("candidate")
    result.update({Path(name): (row["before_sha256"], row["after_sha256"])
                   for name, row in candidate["candidate_overlays"].items()})
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
        ("missing_compiler_case", 2, lambda r: r["cases"].pop()),
        ("wrong_compiler_result", 2, lambda r: r["cases"][0].update(installed_exit_code=False)),
        ("changed_generated_file", 2, lambda r: r["cases"][0]["files"].update(invented="0" * 64)),
        ("incomplete_sdk", 3, lambda r: r["after"].pop()),
        ("extra_metadata_change", 3, lambda r: r["after"][0]["value"].update(invented=True)),
        ("missing_sql_model", 5, lambda r: r["cases"].pop()),
        ("wrong_sql_result", 6, lambda r: r["cases"][0]["reused_results"][0].update(cpp=None)),
        ("overstated_sql_execution", 6, lambda r: r.update(new_sql_executions=30)),
        ("changed_helper_body", 7, lambda r: r.update(after_source="pass\n")),
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
            raise RuntimeError("Container-type corruption accepted: " + name)
    for path, (_, after) in (next(iter(artifact_epochs().items())), list(artifact_epochs().items())[-1]):
        require(not current_input_matches(path, after) and not current_input_matches(path, "0" * 64),
                "historical epochs cannot be relabeled or invented")
        results.extend([{"case": path.name + ":relabel", "rejected": True},
                        {"case": path.name + ":unknown_epoch", "rejected": True}])
    return results
