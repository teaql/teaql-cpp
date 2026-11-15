"""Admit the exact associated-type source/SDK epoch for older nominal receipts."""

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

CANDIDATE = EVIDENCE / "ap02_qualified_entity_types_candidate_5_0_6.json.gz"
CANDIDATE_SHA256 = "324991cc1ad2585050a42d75a7d6518dbf6eaf2b18a5979909684aae311b8e83"
INSTALLED = EVIDENCE / "ap02_qualified_entity_types_installed_5_0_6.json.gz"
INSTALLED_SHA256 = "c9a2a92ed5e01b7613dd7906175943289e85df90b21728070e8e651738c23b65"
SDK = EVIDENCE / "ap02_qualified_entity_types_sdks_5_0_6.json.gz"
SDK_SHA256 = "021228757678400605ddb0ea4cc63c29ffcb864a04501d5b863e88d888e2084b"
BASE_SDK = EVIDENCE / "ap02_reverse_relation_nominal_type_sdk_complete_5_0_6.json.gz"
BASE_SDK_SHA256 = "c7feefaf34dc57b321722bcbc9476304fe1ddd3226224e9937d5d64258dc8888"
SQL = EVIDENCE / "ap02_qualified_entity_types_production_sql_5_0_6.json.gz"
SQL_SHA256 = "ac4cb14fff6bc1549964c7be4970ce1ae25adeb74a7b53cce7545c67a811d254"
COPIED_SQL = EVIDENCE / "ap02_qualified_entity_types_candidate_sql_clang_5_0_6.json.gz"
COPIED_SQL_SHA256 = "f3c9fe75452ce319cfc8a813baba76eb500743b24636feceb8e31d67ae199663"
TRANSITION = EVIDENCE / "ap02_qualified_entity_types_helper_transition_5_0_6.json.gz"
TRANSITION_SHA256 = "9c108dafe0312401e0b1ae32097657958bb09d2835ebe72892b4bfdffde201a8"


def require(condition, message):
    if not condition:
        raise RuntimeError("Projected-type admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(path, expected):
    data = path.read_bytes()
    require(sha(data) == expected, "immutable evidence identity: " + path.name)
    return json.loads(gzip.decompress(data))


def validate(candidate, installed, sdk, base_sdk, sql, transition):
    require(candidate["complete"] is True and candidate["parity_passed"] is True
            and candidate["installed"] is False and installed["complete"] is True
            and installed["installed"] is True and installed["junit_tests"] == 167
            and installed["candidate_sha256"] == CANDIDATE_SHA256
            and installed["input_hashes_before"] == installed["input_hashes_after"]
            and installed["inputs_unchanged"] is True
            and installed["owned_directory_removed"] is True, "complete production installation")
    cases = installed["cases"]
    proposed = {row["name"]: row for row in candidate["cases"]}
    steps = {row["label"]: row for row in installed["steps"]}
    require(len(cases) == len({row["name"] for row in cases}) == len(proposed) == 30
            and len(steps) == len(installed["steps"]), "complete unique compiler commands")
    for row in cases:
        actual = steps[row["name"] + "_compile"]
        expected = proposed[row["name"]]
        require(row["match"] is True and type(row["installed_exit_code"]) is int
                and row["installed_exit_code"] == actual["exit_code"]
                and (row["installed_exit_code"] == 0) == (row["original_exit_code"] == 0)
                and (row["original_exit_code"] == 0) == expected["expected_accept"]
                and row["files"] == expected["generated_files"], "actual original/production compilation")
    require(sum(row["installed_exit_code"] == 0 for row in cases) == 23,
            "twenty-three valid and seven refused types")
    require(all(row["exit_code"] == 0 for row in installed["steps"]
                if not row["label"].endswith("_compile") or row["label"] == "production_fresh_java_compile"),
            "successful Java and generation commands")
    for name, row in candidate["overlays"].items():
        require(sha(row["before_source"].encode()) == row["before_sha256"]
                and sha(row["after_source"].encode()) == row["after_sha256"]
                and sha(Path(name).read_bytes()) == row["after_sha256"], "exact installed Java epoch")
    unit = PRODUCER / "generator/src/test/java/com/skynet/codegenerator/cpp/CppQualifiedEntityTypeProjectionTest.java"
    require(sha(unit.read_bytes()) == candidate["candidate_unit_sha256"], "permanent regression source")

    require(base_sdk["complete"] is True and sdk["complete"] is True
            and sdk["producer_sources_unchanged"] is True and sdk["owned_directory_removed"] is True
            and sdk["before"] == base_sdk["after"]
            and len(sdk["before"]) == len(sdk["after"]) == len(sdk["cases"]) == 80
            and len(sdk["steps"]) == 6 and all(row["exit_code"] == 0 for row in sdk["steps"]),
            "complete eighty-SDK regeneration and checks")
    for before, after, outcome in zip(sdk["before"], sdk["after"], sdk["cases"], strict=True):
        require(before["manifest"] == after["manifest"] == outcome["manifest"]
                and set(before["files"]) == set(after["files"])
                and outcome["code_and_assist_identical"] is True, "complete unchanged SDK file set")
        filename = Path(after["manifest"]).name
        require(outcome["changed_files"] == [filename]
                and all(value == after["files"][name] for name, value in before["files"].items()
                        if name != filename), "only producer metadata may change")
        expected = copy.deepcopy(before["value"])
        for path, row in candidate["overlays"].items():
            key = Path(path).relative_to(PRODUCER).as_posix()
            require(expected["generator_implementation_sha256"][key] == row["before_sha256"],
                    "old SDK producer identity")
            expected["generator_implementation_sha256"][key] = row["after_sha256"]
        require(after["value"] == expected, "exact two producer fingerprint updates")
        directory = (ROOT / after["manifest"]).parent
        require(sha(after["text"].encode()) == after["files"][filename], "complete metadata bytes")
        for name, value in after["files"].items():
            require(sha((directory / name).read_bytes()) == value, "complete current SDK bytes")

    require(sql["complete"] is True and sql["inputs_unchanged"] is True
            and sql["global_inputs_unchanged"] is True and sql["global_references_unchanged"] is True
            and sql["installed_sha256"] == INSTALLED_SHA256
            and sql["production_cpp_java"] == {name: row["after_sha256"]
                                               for name, row in candidate["overlays"].items()}
            and sql["production_dependency_jars"] == installed["dependency_jars"]
            and sql["owned_directory_removed"] is True and len(sql["cases"]) == 3
            and sql["differences"] == [], "actual production SQLite matrix")
    require(all(row["exit_code"] == 0 for row in sql["steps"])
            and sum(len(row["results"]) for row in sql["cases"]) == 15,
            "complete source/native SQL commands and results")
    copied_sql = bound(COPIED_SQL, COPIED_SQL_SHA256)
    for case, previous in zip(sql["cases"], copied_sql["cases"], strict=True):
        require(all(case[key] == previous[key] for key in ("name", "input", "model", "cpp_files", "original_files")),
                "exact source-derived SQL model and complete SDK identity")
        require(len(case["results"]) == 5, "five complete query selections per model")
        for row in case["results"]:
            require(row["match"] is True and same(row["original"], row["cpp"]),
                    "complete type-aware original/production SQL result")
    require(sql["input_hashes_before"] == sql["input_hashes_after"], "complete SQL input epoch")
    for name, expected in sql["input_hashes_before"].items():
        if name == sql["adapted_driver_path"]:
            require(sha(sql["adapted_driver_source"].encode()) == expected == sql["adapted_driver_sha256"],
                    "retained actual SQL driver source")
        else:
            require(sha(Path(name).read_bytes()) == expected, "current SQL source/library/dependency")

    require(transition["complete"] is True and len(transition["edits"]) == 5,
            "bounded historical helper transition")
    source = transition["before_source"]
    for edit in transition["edits"]:
        require(source.count(edit["before"]) == edit["count"] == 1, "exact helper predicate boundary")
        source = source.replace(edit["before"], edit["after"])
    require(source == transition["after_source"]
            and sha(transition["before_source"].encode()) == transition["before_sha256"]
            and sha(source.encode()) == transition["after_sha256"]
            and (ROOT / transition["path"]).read_text() == source, "unchanged remaining helper bodies")
    for name, value in installed["input_hashes_before"].items():
        require(sha(Path(name).read_bytes()) == value, "current production input")
    for name, value in installed["dependency_jars"].items():
        require(sha(Path(name).read_bytes()) == value, "current Java dependency")
    require(sha(Path(installed["native_compiler"]["path"]).read_bytes())
            == installed["native_compiler"]["sha256"], "current native compiler")


@cache
def evidence():
    values = (bound(CANDIDATE, CANDIDATE_SHA256), bound(INSTALLED, INSTALLED_SHA256),
              bound(SDK, SDK_SHA256), bound(BASE_SDK, BASE_SDK_SHA256),
              bound(SQL, SQL_SHA256), bound(TRANSITION, TRANSITION_SHA256))
    validate(*values)
    return values


@cache
def artifact_epochs():
    sdk = bound(SDK, SDK_SHA256)
    return {ROOT / before["manifest"]: (before["files"][Path(before["manifest"]).name],
                                         after["files"][Path(after["manifest"]).name])
            for before, after in zip(sdk["before"], sdk["after"], strict=True)}


def current_input_matches(path, expected):
    java = {
        PRODUCER / "generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java": (
            "6c0edc21b6fd2e28776cf4f8e1e51e716ac762a48ebf237f128c90d2641c9c77",
            "a631a6b4b4cb5ea4c15f85650f07e1206f6980fefe61edf0006064fb17d5b4a5"),
        PRODUCER / "generator/src/main/java/com/skynet/codegenerator/cpp/CppRustLiteralMetadata.java": (
            "73df251f5c031a7526510eac3ce506b4f74ee98525482ab1f65f96725261b6e3",
            "ddbb520430e98bc272544916bbcdeefefeed17d527e2d66c3cf83e61f9beaf12"),
    }
    epoch = java.get(path) or artifact_epochs().get(path)
    if epoch is not None:
        before, after = epoch
        if expected != before:
            return False
        evidence()
        return sha(path.read_bytes()) == after
    return path.is_file() and sha(path.read_bytes()) == expected


def rejection_controls():
    original = evidence()
    cases = (
        ("incomplete_installation", 1, lambda r: r.update(complete=False)),
        ("missing_compiler_case", 1, lambda r: r["cases"].pop()),
        ("wrong_compiler_result", 1, lambda r: r["cases"][0].update(installed_exit_code=1)),
        ("changed_generated_file", 1, lambda r: r["cases"][0]["files"].update(invented="0" * 64)),
        ("incomplete_sdk", 2, lambda r: r.update(complete=False)),
        ("extra_metadata_change", 2, lambda r: r["after"][0]["value"].update(invented=True)),
        ("missing_sql_case", 4, lambda r: r["cases"].pop()),
        ("wrong_sql_value", 4, lambda r: r["cases"][0]["results"][0].update(cpp=None)),
        ("changed_helper_body", 5, lambda r: r.update(after_source="pass\n")),
    )
    results = []
    for name, index, mutate in cases:
        changed = list(original)
        changed[index] = copy.deepcopy(changed[index])
        mutate(changed[index])
        try:
            validate(*changed)
        except RuntimeError as error:
            results.append({"case": name, "rejected": True, "error": str(error)})
        else:
            raise RuntimeError("Projected-type corruption accepted: " + name)
    for name, row in original[0]["overlays"].items():
        require(not current_input_matches(Path(name), row["after_sha256"])
                and not current_input_matches(Path(name), "0" * 64), "source epoch cannot be relabeled")
        results.extend([{"case": Path(name).name + ":relabel", "rejected": True},
                        {"case": Path(name).name + ":unknown_epoch", "rejected": True}])
    path, (_, after) = next(iter(artifact_epochs().items()))
    require(not current_input_matches(path, after) and not current_input_matches(path, "0" * 64),
            "historical SDK epoch cannot be relabeled")
    results.extend([{"case": "sdk_metadata:relabel", "rejected": True},
                    {"case": "sdk_metadata:unknown_epoch", "rejected": True}])
    return results
