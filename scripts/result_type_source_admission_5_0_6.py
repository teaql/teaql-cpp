"""Admit the exact Result error-type source and SDK transition."""

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
from standard_smart_list_source_admission_5_0_6 import current_input_matches as standard_input_matches

RECORDS = {
    "candidate": [
        "ap02_result_error_types_candidate_5_0_6.json.gz",
        "77a525588bd9809e1507e68f5fbddca11f6699bac8a0ef19346cd999a73f3407"
    ],
    "installed": [
        "ap02_result_error_types_installed_repaired_5_0_6.json.gz",
        "a6446037cdac282b5809b5b63eb236cfeb9e9792a50fa9be60394b551b25ff25"
    ],
    "preceding": [
        "ap02_container_entity_types_installed_5_0_6.json.gz",
        "c12ed4e7237527f623e34925bf717a87fe2d18e2a08a7cf9cd2bd89eae27e868"
    ],
    "sdk": [
        "ap02_result_error_types_sdks_5_0_6.json.gz",
        "b952fbdcd7a7ef33f71110558872b1ced270f896f574fafcaaacbc746a405284"
    ],
    "base_sdk": [
        "ap02_container_entity_types_sdks_5_0_6.json.gz",
        "6479659a4f793fea9aaa66d92846bf7ec6abc64a863c5431062045b9235bb7a9"
    ],
    "copied_sql": [
        "ap02_result_error_types_candidate_sql_5_0_6.json.gz",
        "e55672fb3b237b591da04e6862051ca8f72d68d573b7b26efa5e8a08ddef9230"
    ],
    "sql_reuse": [
        "ap02_result_error_types_production_sql_reuse_5_0_6.json.gz",
        "5d70cefcc448d914160aaf9e573982d7d9d4efd6216786a1ff2b119226f0e04e"
    ],
    "transition": [
        "ap02_result_error_types_helper_transition_5_0_6.json.gz",
        "53a1956c75f43a6526bc3d56cb632a4421338cc3ad36d6863fde6b420fe73f2a"
    ]
}


def require(condition, message):
    if not condition:
        raise RuntimeError("Result-type admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable evidence: " + name)
    return json.loads(gzip.decompress(data))


def validate(candidate, installed, preceding, sdk, base_sdk, copied_sql, sql_reuse, transition):
    require(candidate["complete"] is True and candidate["parity_passed"] is True
            and candidate["installed"] is False and candidate["junit_tests"] == 178
            and candidate["differences"] == [] and len(candidate["cases"]) == 28
            and len(candidate["candidate_overlays"]) == 2
            and candidate["inputs_unchanged"] is True and candidate["references_unchanged"] is True
            and candidate["owned_directory_removed"] is True, "complete copied source/compiler witness")
    require(installed["complete"] is True and installed["installed"] is True
            and installed["whole_goal_complete"] is False and installed["junit_tests"] == 178
            and installed["candidate_sha256"] == RECORDS["candidate"][1]
            and installed["preceding_sha256"] == RECORDS["preceding"][1]
            and installed["input_hashes_before"] == installed["input_hashes_after"]
            and installed["global_inputs_before"] == installed["global_inputs_after"]
            and installed["global_references_before"] == installed["global_references_after"]
            and installed["inputs_unchanged"] is True and installed["global_inputs_unchanged"] is True
            and installed["global_references_unchanged"] is True
            and installed["owned_directory_removed"] is True, "complete production epoch")
    require(preceding["complete"] is True and len(preceding["cases"]) == 56,
            "preserved complete compiler witness")
    expected = {"preserved_" + row["name"]: (row, True) for row in preceding["cases"]}
    expected.update({"result_" + row["name"]: (row, False) for row in candidate["cases"]})
    steps = {row["label"]: row for row in installed["steps"]}
    rows = installed["cases"]
    require(len(expected) == len(rows) == len({row["name"] for row in rows}) == 84
            and {row["name"] for row in rows} == set(expected)
            and len(steps) == len(installed["steps"]), "unique complete production selections")
    for row in rows:
        old, preserved = expected[row["name"]]
        files = old["files"] if preserved else old["cpp_files"]
        reused = preserved and not row["preserved_artifact_changed"]
        if row["preserved_artifact_changed"]:
            changed = sorted(name for name in row["files"] if row["files"][name] != files.get(name))
            require(preserved and old["type"] == "<u64 as ToOwned>::Owned"
                    and old["original_exit_code"] > 0 and set(row["files"]) == set(files)
                    and len(changed) == 1 and changed[0].endswith("/models.hpp")
                    and row["changed_files"] == changed
                    and any("projection.reverse_one<<u64 as ToOwned>::Owned>" in line
                            for line in row["cpp_projection"]), "bounded refused scalar diagnostic")
        else:
            require(row["files"] == files and row["changed_files"] == [], "complete exact SDK identity")
        require(row["model"] == old["model"] and row["type"] == old["type"]
                and row["match"] is True
                and type(row["installed_exit_code"]) is int
                and row["original_exit_code"] == old["original_exit_code"]
                and (row["installed_exit_code"] == 0) == (old["original_exit_code"] == 0)
                and row["compiler_result_reused"] is reused and row["actual_compile"] is not reused,
                "complete input/artifact/compiler result identity")
        if reused:
            require(row["installed_exit_code"] == old["installed_exit_code"]
                    and row["name"] + "_compile" not in steps, "honest unchanged-SDK compiler reuse")
        else:
            require(row["installed_exit_code"] == steps[row["name"] + "_compile"]["exit_code"],
                    "actual production compiler result")
    require(sum(row["actual_compile"] for row in rows) == 30
            and sum(row["compiler_result_reused"] for row in rows) == 54
            and sum(row["installed_exit_code"] == 0 for row in rows) == 57,
            "thirty actual and fifty-four reused compiler outcomes")
    actual_labels = {row["name"] + "_compile" for row in rows if row["actual_compile"]}
    require(all(row["exit_code"] == 0 for row in installed["steps"] if row["label"] not in actual_labels),
            "successful package, Java and generation processes")
    overlays = candidate["candidate_overlays"]
    for name, row in overlays.items():
        require(sha(row["before_source"].encode()) == row["before_sha256"]
                and sha(row["after_source"].encode()) == row["after_sha256"]
                and standard_input_matches(Path(name), row["after_sha256"]), "exact installed Java source")
    unit = PRODUCER / "generator/src/test/java/com/skynet/codegenerator/cpp/CppResultTypeProjectionTest.java"
    require(sha(unit.read_bytes()) == candidate["candidate_unit_sha256"], "permanent regressions")
    for name, expected_hash in installed["input_hashes_before"].items():
        metadata = transition["installation_script_metadata"]
        if name == str(ROOT / metadata["path"]):
            require(expected_hash == metadata["before_sha256"]
                    and sha(Path(name).read_bytes()) == metadata["after_sha256"],
                    "exact recipe wording correction; execution body preserved")
        else:
            require(standard_input_matches(Path(name), expected_hash), "current production compilation input")
    for name, expected_hash in installed["dependency_jars"].items():
        require(sha(Path(name).read_bytes()) == expected_hash, "current Java dependency")
    compiler = installed["native_compiler"]
    require(sha(Path(compiler["path"]).read_bytes()) == compiler["sha256"], "native compiler identity")

    require(base_sdk["complete"] is True and sdk["complete"] is True
            and sdk["installed_sha256"] == RECORDS["installed"][1]
            and sdk["baseline_sha256"] == RECORDS["base_sdk"][1]
            and sdk["before"] == base_sdk["after"] and sdk["producer_sources_unchanged"] is True
            and sdk["owned_directory_removed"] is True
            and len(sdk["before"]) == len(sdk["after"]) == len(sdk["cases"]) == 80
            and len(sdk["steps"]) == 6 and all(row["exit_code"] == 0 for row in sdk["steps"]),
            "all eighty SDKs regenerated and checked")
    for before, after, outcome in zip(sdk["before"], sdk["after"], sdk["cases"], strict=True):
        filename = Path(after["manifest"]).name
        require(before["manifest"] == after["manifest"] == outcome["manifest"]
                and set(before["files"]) == set(after["files"])
                and outcome["changed_files"] == [filename] and outcome["code_and_assist_identical"] is True
                and all(value == after["files"][name] for name, value in before["files"].items()
                        if name != filename), "unchanged complete implementation and Assist")
        expected_value = copy.deepcopy(before["value"])
        for name, row in overlays.items():
            key = Path(name).relative_to(PRODUCER).as_posix()
            require(expected_value["generator_implementation_sha256"][key] == row["before_sha256"],
                    "preceding producer fingerprint")
            expected_value["generator_implementation_sha256"][key] = row["after_sha256"]
        require(after["value"] == expected_value and sha(after["text"].encode()) == after["files"][filename],
                "exact two producer fingerprint changes")
        directory = (ROOT / after["manifest"]).parent
        require(all(standard_input_matches(directory / name, value) for name, value in after["files"].items()),
                "actual current complete SDK bytes")

    require(copied_sql["complete"] is True and copied_sql["installed"] is False
            and copied_sql["candidate_sha256"] == RECORDS["candidate"][1]
            and copied_sql["candidate_overlays"] == overlays and copied_sql["differences"] == []
            and copied_sql["compiled_cpp_inputs_unchanged"] is True
            and copied_sql["inputs_unchanged"] is True and copied_sql["global_inputs_unchanged"] is True
            and copied_sql["global_references_unchanged"] is True
            and copied_sql["owned_directory_removed"] is True
            and sql_reuse["complete"] is True and sql_reuse["installed"] is True
            and sql_reuse["source_sql_sha256"] == RECORDS["copied_sql"][1]
            and sql_reuse["installed_sha256"] == RECORDS["installed"][1]
            and sql_reuse["new_sql_executions"] == 0 and sql_reuse["reused_sql_results"] == 30
            and sql_reuse["inputs_unchanged"] is True and sql_reuse["references_unchanged"] is True
            and sql_reuse["owned_directory_removed"] is True
            and len(copied_sql["cases"]) == len(sql_reuse["cases"]) == 6
            and all(row["exit_code"] == 0 for receipt in (copied_sql, sql_reuse) for row in receipt["steps"]),
            "actual SQLite execution and explicit exact-artifact production reuse")
    for actual, old in zip(sql_reuse["cases"], copied_sql["cases"], strict=True):
        require(all(actual[key] == old[key] for key in ("name", "input", "model", "cpp_files"))
                and actual["reused_results"] == old["results"] and len(old["results"]) == 5,
                "complete model, SDK and thirty-result selection")
        require(all(row["match"] is True and same(row["original"], row["cpp"]) for row in old["results"]),
                "strict query, E and JSON equivalence")
    normalized = {}
    for name, value in copied_sql["compiled_cpp_java"].items():
        path = Path(name)
        for original_name in overlays:
            if path.name == Path(original_name).name:
                path = Path(original_name)
                break
        require(standard_input_matches(path, value), "actual compiled copied/production Java source")
        normalized[str(path)] = value
    require(sql_reuse["compiled_cpp_java"] == normalized
            and sql_reuse["dependency_jars"] == installed["dependency_jars"]
            and sql_reuse["input_hashes_before"] == sql_reuse["input_hashes_after"],
            "complete SQL compilation and input identity")
    for name, value in copied_sql["input_hashes_before"].items():
        path = Path(name)
        if name == copied_sql["adapted_driver_path"]:
            require(sha(copied_sql["adapted_driver_source"].encode()) == value, "retained SQL controller")
        elif name in overlays:
            require(value == overlays[name]["before_sha256"], "declared unused production source")
        else:
            require(standard_input_matches(path, value), "unchanged actual SQL input")

    metadata = transition["installation_script_metadata"]
    require(metadata["before_source"].count(metadata["before"]) == 1
            and metadata["before_source"].replace(metadata["before"], metadata["after"]) == metadata["after_source"]
            and sha(metadata["before_source"].encode()) == metadata["before_sha256"]
            and sha(metadata["after_source"].encode()) == metadata["after_sha256"]
            and (ROOT / metadata["path"]).read_text() == metadata["after_source"],
            "one metadata-only recipe wording correction")
    require(transition["complete"] is True and len(transition["edits"]) == 7,
            "seven exact preceding admission boundaries")
    source = transition["before_source"]
    for edit in transition["edits"]:
        require(edit["count"] == source.count(edit["before"]) == 1, "exact current-input predicate")
        source = source.replace(edit["before"], edit["after"])
    require(source == transition["after_source"]
            and sha(transition["before_source"].encode()) == transition["before_sha256"]
            and sha(source.encode()) == transition["after_sha256"]
            and (ROOT / transition["path"]).read_text() == source,
            "historical validation bodies and controls preserved")


@cache
def evidence():
    values = tuple(bound(key) for key in RECORDS)
    validate(*values)
    return values


@cache
def artifact_epochs():
    candidate, _, _, sdk, *_ = (bound(key) for key in RECORDS)
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
        return standard_input_matches(path, after)
    return standard_input_matches(path, expected)


def rejection_controls():
    original = evidence()
    controls = (
        ("incomplete_installation", 1, lambda r: r.update(complete=False)),
        ("missing_compiler_case", 1, lambda r: r["cases"].pop()),
        ("wrong_compiler_result", 1, lambda r: r["cases"][-1].update(installed_exit_code=False)),
        ("overstated_compiler_execution", 1, lambda r: r["cases"][0].update(actual_compile=True)),
        ("changed_generated_file", 1, lambda r: r["cases"][0]["files"].update(invented="0" * 64)),
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
            raise RuntimeError("Result-type corruption accepted: " + name)
    for path, (_, after) in (next(iter(artifact_epochs().items())), list(artifact_epochs().items())[-1]):
        require(not current_input_matches(path, after) and not current_input_matches(path, "0" * 64),
                "historical epochs cannot be relabeled or invented")
        results.extend([{"case": path.name + ":relabel", "rejected": True},
                        {"case": path.name + ":unknown_epoch", "rejected": True}])
    return results
