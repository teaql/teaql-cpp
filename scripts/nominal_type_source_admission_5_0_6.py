"""Admit the exact installed nominal-type change for historical execution receipts."""

import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path

from cross_context_corpus_extension_5_0_6 import source_input_matches as corpus_input_matches


ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
CANDIDATE = EVIDENCE / "ap02_reverse_relation_nominal_type_candidate_complete_5_0_6.json.gz"
CANDIDATE_SHA256 = "0524e025bf2a6ce49ccad07a5c475844cbfe5c2489285459c87bdd35d3ff04bc"
INSTALLED = EVIDENCE / "ap02_reverse_relation_nominal_type_installed_5_0_6.json.gz"
INSTALLED_SHA256 = "2aeaa2b94701b615e6b3c91a25b61834269df0396f7c5ed3095175e25c9a380a"
SDK = EVIDENCE / "ap02_reverse_relation_nominal_type_sdk_complete_5_0_6.json.gz"
SDK_SHA256 = "c7feefaf34dc57b321722bcbc9476304fe1ddd3226224e9937d5d64258dc8888"
TRANSITION = EVIDENCE / "ap02_reverse_relation_nominal_type_source_transition_5_0_6.json.gz"
TRANSITION_SHA256 = "349c12f75ecba508444603bfb3589456265a54f05e47f108a4251d60b42cb067"
JAVA_PACKAGE = PRODUCER / "generator/src/main/java/com/skynet/codegenerator/cpp"
JAVA = {
    JAVA_PACKAGE / "CppCoreGenerator.java": (
        "60c3b1e413af48d5acf02fff12318f083feb3a41508da7c585d87678fbed9fc0",
        "6c0edc21b6fd2e28776cf4f8e1e51e716ac762a48ebf237f128c90d2641c9c77"),
    JAVA_PACKAGE / "CppRustLiteralMetadata.java": (
        "09c664a667ba1009e73add94adfbf1a1f4f69d7ed4e8f7cc52b380175d03179c",
        "73df251f5c031a7526510eac3ce506b4f74ee98525482ab1f65f96725261b6e3"),
}


def require(condition, message):
    if not condition:
        raise RuntimeError("Nominal-type admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read_bound(path, expected):
    data = path.read_bytes()
    require(sha(data) == expected, "immutable evidence identity: " + path.name)
    return json.loads(gzip.decompress(data))


def validate(candidate, installed, sdk, transition):
    require(candidate["complete"] is True and candidate["installed"] is False,
            "reviewed copied candidate")
    require(installed["complete"] is True and installed["installed"] is True
            and installed["candidate_sha256"] == CANDIDATE_SHA256
            and installed["input_hashes_before"] == installed["input_hashes_after"]
            and installed["inputs_unchanged"] is True and installed["junit_tests"] == 163
            and installed["runtime_cpp_unchanged"] is True
            and installed["sql_libraries_unchanged"] is True
            and installed["owned_directory_removed"] is True,
            "complete production generation and Java execution")
    require(len(installed["cases"]) == len({row["name"] for row in installed["cases"]}) == 22,
            "complete production compiler results")
    steps = {row["label"]: row for row in installed["steps"]}
    require(len(steps) == len(installed["steps"]), "unique production commands")
    for row in installed["cases"]:
        result = steps[row["name"] + "_compile"]
        valid = row["original_exit_code"] == 0
        require(row["match"] is True and type(row["installed_exit_code"]) is int
                and row["installed_exit_code"] == result["exit_code"]
                and (result["exit_code"] == 0) == valid,
                "actual original/installed compiler result")
    require(sum(row["installed_exit_code"] == 0 for row in installed["cases"]) == 16,
            "sixteen accepted and six rejected types")
    require(all(row["exit_code"] == 0 for row in installed["steps"]
                if not row["label"].endswith("_compile") or row["label"] == "fresh_java_compile"),
            "successful production generation commands")
    require(len(installed["sql_reproductions"]) == 3
            and sum(len(row["reused_results"]) for row in installed["sql_reproductions"]) == 15
            and all(item["match"] is True for row in installed["sql_reproductions"]
                    for item in row["reused_results"]), "exact declared SQL reuse")
    for path, (before, after) in JAVA.items():
        row = candidate["candidate_overlays"][str(path)]
        require(row["before_sha256"] == sha(row["before_source"].encode()) == before
                and row["after_sha256"] == sha(row["after_source"].encode()) == after
                and installed["input_hashes_before"][str(path)] == after
                and sha(path.read_bytes()) == after, "exact installed Java source epoch")

    require(sdk["complete"] is True and sdk["producer_sources_unchanged"] is True
            and sdk["owned_directory_removed"] is True
            and len(sdk["before"]) == len(sdk["after"]) == len(sdk["cases"]) == 80
            and all(row["exit_code"] == 0 for row in sdk["steps"]),
            "complete eighty-SDK reproduction")
    for before, after, outcome in zip(sdk["before"], sdk["after"], sdk["cases"], strict=True):
        require(before["manifest"] == after["manifest"] == outcome["manifest"]
                and outcome["code_and_assist_identical"] is True
                and set(before["files"]) == set(after["files"]), "complete SDK identity and file set")
        name = Path(before["manifest"]).name
        require(outcome["changed_files"] == [name]
                and all(before["files"][key] == value for key, value in after["files"].items()
                        if key != name), "only producer metadata changed")
        expected = copy.deepcopy(before["value"])
        for path, (old, current) in JAVA.items():
            key = path.relative_to(PRODUCER).as_posix()
            require(expected["generator_implementation_sha256"][key] == old,
                    "historical SDK producer identity")
            expected["generator_implementation_sha256"][key] = current
        require(after["value"] == expected, "exact two producer fingerprint updates")
        directory = (ROOT / after["manifest"]).parent
        for key, value in after["files"].items():
            require(sha((directory / key).read_bytes()) == value, "current complete SDK bytes")

    require(transition["complete"] is True and len(transition["guards"]) == 6
            and transition["candidate_sha256"] == CANDIDATE_SHA256
            and transition["installed_sha256"] == INSTALLED_SHA256
            and transition["sdk_sha256"] == SDK_SHA256, "bounded guard transition")
    for row in transition["guards"]:
        expected = row["before_source"]
        for edit in row["edits"]:
            require(expected.count(edit["before"]) == edit["count"], "exact guard predicate boundary")
            expected = expected.replace(edit["before"], edit["after"])
        require(expected == row["after_source"]
                and sha(row["before_source"].encode()) == row["before_sha256"]
                and sha(row["after_source"].encode()) == row["after_sha256"]
                and (ROOT / row["path"]).read_text() == expected, "unchanged remaining guard bodies")
    for name, wanted in installed["input_hashes_before"].items():
        require(sha(Path(name).read_bytes()) == wanted, "current installed execution input")
    for name, wanted in installed["dependency_jars"].items():
        require(sha(Path(name).read_bytes()) == wanted, "current Java dependency")
    for name, wanted in installed["runtime_cpp_inputs"].items():
        require(sha((ROOT / name).read_bytes()) == wanted, "current runtime source for SQL reuse")
    for name, wanted in installed["sql_libraries"].items():
        require(sha(Path(name).read_bytes()) == wanted, "current linked SQL library")
    require(sha(Path(installed["native_compiler"]["path"]).read_bytes())
            == installed["native_compiler"]["sha256"], "current native compiler")


@cache
def evidence():
    rows = (read_bound(CANDIDATE, CANDIDATE_SHA256), read_bound(INSTALLED, INSTALLED_SHA256),
            read_bound(SDK, SDK_SHA256), read_bound(TRANSITION, TRANSITION_SHA256))
    validate(*rows)
    return rows


def historical_input_matches(path, expected):
    if path in JAVA:
        before, after = JAVA[path]
        if expected != before:
            return False
        evidence()
        return sha(path.read_bytes()) == after
    return path.is_file() and sha(path.read_bytes()) == expected


def cross_context_input_matches(path, expected):
    if path in JAVA:
        return historical_input_matches(path, expected)
    return corpus_input_matches(path, expected)


def rejection_controls():
    candidate, installed, sdk, transition = evidence()
    controls = []
    mutations = [
        ("incomplete_installation", 1, lambda r: r.update(complete=False)),
        ("missing_compiler_case", 1, lambda r: r["cases"].pop()),
        ("changed_compiler_result", 1, lambda r: r["cases"][0].update(installed_exit_code=1)),
        ("missing_sql_result", 1, lambda r: r["sql_reproductions"][0]["reused_results"].pop()),
        ("incomplete_sdk_reproduction", 2, lambda r: r.update(complete=False)),
        ("changed_sdk_code", 2, lambda r: r["cases"][0].update(code_and_assist_identical=False)),
        ("extra_sdk_metadata_change", 2, lambda r: r["after"][0]["value"].update(invented=True)),
        ("changed_source_epoch", 0, lambda r: next(iter(r["candidate_overlays"].values()))
         .update(before_sha256="0" * 64)),
        ("changed_guard_body", 3, lambda r: r["guards"][0].update(after_source="pass\n")),
    ]
    original = (candidate, installed, sdk, transition)
    for name, index, mutate in mutations:
        changed = list(original)
        changed[index] = copy.deepcopy(original[index])
        mutate(changed[index])
        try:
            validate(*changed)
        except (RuntimeError, KeyError):
            controls.append(name)
        else:
            raise RuntimeError("Nominal-type corruption accepted: " + name)
    for path, (_, after) in JAVA.items():
        require(not historical_input_matches(path, after)
                and not historical_input_matches(path, "0" * 64), "historical epoch cannot be relabeled")
        controls.extend([path.name + ":relabel", path.name + ":unknown_epoch"])
    return controls
