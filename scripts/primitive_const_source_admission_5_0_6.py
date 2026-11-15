"""Admit the reviewed primitive constant/tuple projection source and SDK transition."""

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

RECORDS = {
    "candidate": [
        "ap02_primitive_const_path_candidate_v2_5_0_6.json.gz",
        "58a5af8a15b873712640479744408adc6ea2d5af699e452b3838d8fd5f81be96"
    ],
    "installed": [
        "ap02_primitive_const_path_installed_5_0_6.json.gz",
        "d0660657cc2c0744d797eb5ff10d9104e5afbaa6e95fc23db4fe03cca3c257d8"
    ],
    "preceding": [
        "ap02_scalar_const_value_installed_5_0_6.json.gz",
        "c194c580dac6a0922084897b6e7df1599ea681d514f9b33e01403920aa535625"
    ],
    "sdk": [
        "ap02_primitive_const_path_sdks_5_0_6.json.gz",
        "99363b549281aab35bca4fc7b9bd2f0ddc974013c49df39eee2b33440f7255c6"
    ],
    "base_sdk": [
        "ap02_scalar_const_value_sdks_5_0_6.json.gz",
        "ff65f4f5934e8aebacad283bee6b555efc7a53a400f557ee655abb8f8483592d"
    ],
    "copied_sql": [
        "ap02_primitive_const_path_candidate_sql_5_0_6.json.gz",
        "7de30cf5c4cee81af79cbfce416a44b330b1b73c20874c762a2219b172356b70"
    ],
    "sql_reuse": [
        "ap02_primitive_const_path_production_sql_reuse_5_0_6.json.gz",
        "b077c4cb1d4491833d23d943e5a0de658ba6def0c4c1537265f6df3c0e3aa868"
    ],
    "transition": [
        "ap02_primitive_const_path_helper_transition_5_0_6.json.gz",
        "4b8c8292faa049661a85dc2d5d82b1463b489e65f096b753d80f3a7c081508f8"
    ],
    "models": [
        "ap02_primitive_const_path_model_inventory_5_0_6.json.gz",
        "2001fe75959d703b020d882932050adc4e22f762405a700d2c4c508d9d00e564"
    ],
    "expressions": [
        "ap02_primitive_const_expression_matrix_v3_5_0_6.json.gz",
        "7d1696a7c54c5073cc90a2e8027af9dd1401589c4b942eefd828b79677641103"
    ],
    "language": [
        "ap02_primitive_const_path_language_matrix_v2_5_0_6.json.gz",
        "c90e193636099755ad6fd3ee07b33727ef80d165676531df50d4f14c9608a145"
    ],
    "proposal": [
        "ap02_primitive_const_path_proposal_5_0_6.json.gz",
        "5297e6b21608e7ef8e261ee43e576d46b2dd6ca0b8596ef2d8c004b2b2372ad7"
    ],
    "copied_controls": [
        "ap02_primitive_const_path_copied_controls_5_0_6.json.gz",
        "ebff176e545a002ddb034cfcb8d075694a61796404254115740501de1e4d6ab2"
    ]
}

def require(condition, message):
    if not condition:
        raise RuntimeError("Primitive const admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable evidence: " + name)
    return json.loads(gzip.decompress(data))


def validate(candidate, installed, preceding, sdk, base_sdk, copied_sql, sql_reuse,
             transition, models, expressions, language, proposal, copied_controls):
    require(candidate["complete"] is True and candidate["parity_passed"] is True
            and candidate["installed"] is False and candidate["whole_goal_complete"] is False
            and candidate["junit_tests"] == 213 and candidate["differences"] == []
            and len(candidate["cases"]) == len(candidate["planned_cases"]) == 61
            and candidate["actual_cpp_compiler_processes"] == 61
            and candidate["reused_original_compiler_outcomes"] == 61
            and candidate["new_original_compiler_processes"] == 0
            and candidate["inputs_unchanged"] is True and candidate["references_unchanged"] is True
            and candidate["input_hashes_before"] == candidate["input_hashes_after"]
            and candidate["references_before"] == candidate["references_after"]
            and candidate["owned_directory_removed"] is True
            and candidate["diagnostic_sha256"] == RECORDS["models"][1]
            and candidate["expression_matrix_sha256"] == RECORDS["expressions"][1],
            "complete copied primitive/tuple source/compiler qualification")
    require(models["complete"] is True and len(models["cases"]) == 61
            and expressions["complete"] is True and len(expressions["cases"]) == 928
            and language["complete"] is True and len(language["constant_inventory"]) == 70
            and len(language["canonical"]) == 560 and len(language["legacy"]) == 280
            and expressions["language_sha256"] == RECORDS["language"][1],
            "complete actual constant catalog, expression contexts and compiler ownership")
    actual = {row["name"]: row for row in models["cases"]}
    plans = {row["name"]: row for row in candidate["planned_cases"]}
    require(len(actual) == len(plans) == 61
            and {row["name"] for row in candidate["cases"]} == set(actual) == set(plans),
            "exact unique compiler selections")
    for row in candidate["cases"]:
        prior = actual[row["name"]]
        require(row["match"] is True and row["original_compiler_outcome_explicitly_reused"] is True
                and all(row[key] == prior[key] for key in (
                    "model", "type", "original_files", "original_exit_code", "consumer_manifest",
                    "consumer_source", "consumer_lock"))
                and all(row[key] == plans[row["name"]][key] for key in ("name", "model", "type"))
                and type(row["cpp_exit_code"]) is int and type(row["original_exit_code"]) is int
                and (row["cpp_exit_code"] == 0) == (row["original_exit_code"] == 0)
                and (row["cpp_exit_code"] != 0
                     or len(row["cpp_projection"]) == row["original_projection_count"]),
                "fresh C++ and byte-identical original SDK/compiler/projection witness")
    require(sum(row["cpp_exit_code"] == 0 for row in candidate["cases"]) == 52
            and sum(row["original_projection_count"] == 1 for row in candidate["cases"]) == 50,
            "compiler success and retained typed projection stay distinct")
    labels = [row["label"] for row in candidate["steps"]]
    require(len(labels) == len(set(labels))
            and sum(name.endswith("_cpp_compile") for name in labels) == 61
            and not any(name.endswith("_original_compile") for name in labels),
            "honest actual compiler process membership")
    resource = candidate["candidate_resource"]
    resource_bytes = base64.b64decode(resource["content_base64"], validate=True)
    require(sha(resource_bytes) == resource["sha256"]
            and sha((PRODUCER / resource["path"]).read_bytes()) == resource["sha256"]
            and resource["source_reference_sha256"] == RECORDS["expressions"][1],
            "immutable installed source-derived fixture")
    corpus = json.loads(gzip.decompress(resource_bytes))
    require(corpus["source_reference_sha256"] == RECORDS["expressions"][1]
            and len(corpus["cases"]) == 928 and len(corpus["model_cases"]) == 61,
            "all expression and actual-model fixture entries")
    overrides = {"tuple_index_33", "tuple_index_34", "tuple_index_35",
                 "tuple_index_36", "tuple_index_37"}
    for raw, row in zip(expressions["cases"], corpus["cases"], strict=True):
        require(raw["expression"] == row["expression"], "exact original expression")
        if row.get("name") in overrides:
            witness = actual[row["name"]]
            require(raw["accepted"] is False and row["constant_context_accepted"] is False
                    and row["accepted"] is True
                    and witness["original_exit_code"] == 0
                    and witness["original_projection_count"] == 1
                    and row["derive_normalization_witness"]["source_sha256"]
                        == witness["source_receipt_sha256"],
                    "actual derive normalization without relabeling raw const evidence")
        else:
            require(row["accepted"] is raw["accepted"], "no undeclared compiler-context change")
    require(proposal["complete"] is True and proposal["installed"] is False
            and proposal["candidate_sha256"] == RECORDS["candidate"][1]
            and proposal["apply_check_exit_code"] == 0 and len(proposal["paths"]) == 3
            and sha((ROOT / proposal["patch"]).read_bytes()) == proposal["patch_sha256"],
            "exact reviewed three-file source/resource patch")
    require(copied_controls["complete"] is True and len(copied_controls["modes"]) == 2
            and {row["optimized"] for row in copied_controls["modes"]} == {0, 1}
            and all(len(row["controls"]) == 15
                    and all(item["rejected"] is True for item in row["controls"])
                    for row in copied_controls["modes"]),
            "actual ordinary/optimized copied-evidence controls")
    require(preceding["complete"] is True and preceding["installed"] is True
            and preceding["junit_tests"] == 203 and len(preceding["cases"]) == 258,
            "all preceding production compiler contracts")
    require(installed["complete"] is True and installed["installed"] is True
            and installed["whole_goal_complete"] is False and installed["junit_tests"] == 213
            and installed["candidate_sha256"] == RECORDS["candidate"][1]
            and installed["preceding_sha256"] == RECORDS["preceding"][1]
            and installed["new_cpp_compiler_processes"] == 0
            and installed["reused_cpp_compiler_outcomes"] == 319
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
        ("primitive_", "candidate", candidate["cases"], "cpp_files", "cpp_exit_code"),
    ):
        for row in selections:
            expected[prefix + row["name"]] = (owner, row, file_key, code_key)
    rows = installed["cases"]
    require(len(rows) == len(expected) == len({row["name"] for row in rows}) == 319
            and {row["name"] for row in rows} == set(expected), "all production compiler contracts")
    for row in rows:
        owner, old, file_key, code_key = expected[row["name"]]
        require(row["source_record"] == owner and row["source_case"] == old["name"]
                and row["model"] == old["model"] and row["type"] == old["type"]
                and row["files"] == old[file_key]
                and type(row["original_exit_code"]) is int
                and type(row["reused_cpp_exit_code"]) is int
                and row["reused_cpp_exit_code"] == old[code_key]
                and row["original_exit_code"] == old["original_exit_code"]
                and (row["reused_cpp_exit_code"] == 0) == (row["original_exit_code"] == 0)
                and row["compiler_outcome_explicitly_reused"] is True and row["actual_compile"] is False,
                "exact complete production SDK and retained compiler result")
    for row in rows:
        if row["source_record"] == "candidate":
            require(row["expected_projection_count"] == actual[row["source_case"]]["original_projection_count"],
                    "production preserves typed projection versus ignored nested type")
    steps = {row["label"]: row for row in installed["steps"]}
    required = {"producer_package", "production_java_compile", "production_junit"}
    required.update(row["name"] + "_generate" for row in rows)
    require(len(steps) == len(installed["steps"]) == 322 and set(steps) == required
            and all(row["exit_code"] == 0 for row in installed["steps"]),
            "all successful production processes without invented compilation")
    overlays = candidate["candidate_overlays"]
    require(len(overlays) == 1, "one reviewed production parser")
    for name, row in overlays.items():
        require(sha(row["before_source"].encode()) == row["before_sha256"]
                and sha(row["after_source"].encode()) == row["after_sha256"]
                and sha(Path(name).read_bytes()) == row["after_sha256"], "exact reviewed installed parser")
    unit = PRODUCER / "generator/src/test/java/com/skynet/codegenerator/cpp/CppPrimitiveConstPathProjectionTest.java"
    require(sha(unit.read_bytes()) == candidate["candidate_unit_sha256"], "ten permanent regressions")
    require(sha(candidate["candidate_unit_source"].encode()) == candidate["candidate_unit_sha256"],
            "retained permanent regression source")
    for name, value in installed["input_hashes_before"].items():
        require(sha(Path(name).read_bytes()) == value, "actual current production input")
    for name, value in installed["dependency_jars"].items():
        require(sha(Path(name).read_bytes()) == value, "actual Java dependency")
    compiler = installed["native_compiler"]
    require(compiler == candidate["native_compiler"] == models["native_compiler"] == preceding["native_compiler"]
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
        ("incomplete_candidate", 0, lambda r: r.update(complete=False)),
        ("wrong_copied_cpp_result", 0, lambda r: r["cases"][0].update(cpp_exit_code=False)),
        ("wrong_copied_projection", 0, lambda r: r["cases"][0].update(cpp_projection=[])),
        ("overstated_original_execution", 0, lambda r: r.update(new_original_compiler_processes=61)),
        ("wrong_source_resource", 0, lambda r: r["candidate_resource"].update(sha256="0" * 64)),
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
        ("missing_original_owner", 8, lambda r: r["cases"].pop()),
        ("changed_original_sdk", 8, lambda r: r["cases"][0]["original_files"].update(invented="0" * 64)),
        ("changed_const_context", 9, lambda r: r["cases"][0].update(accepted=False)),
        ("missing_constant_kind", 10, lambda r: r["constant_inventory"].pop()),
        ("failed_patch_check", 11, lambda r: r.update(apply_check_exit_code=1)),
        ("changed_patch_owner", 11, lambda r: r.update(candidate_sha256="0" * 64)),
        ("missing_copied_control", 12, lambda r: r["modes"][0]["controls"].pop()),
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
            raise RuntimeError("Primitive const corruption accepted: " + name)
    epochs = list(artifact_epochs().items())
    for path, (_, after) in (epochs[0], epochs[-1]):
        require(not current_input_matches(path, after) and not current_input_matches(path, "0" * 64),
                "historical epochs cannot be relabeled or invented")
        results.extend([{"case": path.name + ":relabel", "rejected": True},
                        {"case": path.name + ":unknown_epoch", "rejected": True}])
    return results
