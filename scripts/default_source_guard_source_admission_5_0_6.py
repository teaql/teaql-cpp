#!/usr/bin/env python3
"""Qualify configured-source requirements and exact historical source/artifact forwarding."""
import argparse
import ast
import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
RECORD = ROOT / "docs/parity/default_source_guard_sources_5_0_6.json.gz"
RECORD_SHA = "9f6a49000e0f1984374ab26bd97f44ed892251f6679f298bd071bd3c4e6402a7"
JAVA = ROOT / "docs/parity/default_source_guard_java_complete_5_0_6.json.gz"
JAVA_SHA = "2227e427f19bd2d895e537917736f51b2bdd2cd60204a3e368b98db110934a98"
SDKS = ROOT / "docs/parity/default_source_guard_sdks_5_0_6.json.gz"
SDKS_SHA = "ace82b9d13f354d18a09a0dd7b636deebaa19fabdbeef88ba41d979a8dcbdf21"
CORE = "producer:generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java"
SERVICE = "producer:generator/src/main/java/com/skynet/codegenerator/DomainGenerationService.java"
REQUIREMENT = "producer:model/src/main/java/com/skynet/codegenerator/SourceGenerationRequirement.java"
TEST = "producer:generator/src/test/java/com/skynet/codegenerator/DomainGenerationSourceTest.java"
SELF = "runtime:scripts/default_source_guard_source_admission_5_0_6.py"
READERS = {
    "runtime:scripts/text_alias_source_admission_5_0_6.py":
        {"validate_execution", "validate_sdks", "validate", "current_input_matches",
         "qualified_transition", "preceding_source", "sdk_snapshot_matches"},
    "runtime:scripts/compiler_offline_source_admission_5_0_6.py":
        {"source_epochs", "added_input_hashes"},
}

def require(condition, message):
    if not condition:
        raise ValueError("Default source guard admission: " + message)

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
    functions = [{node.name: ast.dump(node) for node in tree.body if isinstance(node, ast.FunctionDef)}
                 for tree in trees]
    require(functions[0].keys() == functions[1].keys()
            and {key for key in functions[0] if functions[0][key] != functions[1][key]} == expected,
            "exact identity-only reader functions")
    for tree in trees:
        for node in tree.body:
            if isinstance(node, ast.FunctionDef) and node.name in expected:
                node.body = [ast.Pass()]
    require(ast.dump(trees[0]) == ast.dump(trees[1]), "execution, comparison and controls remain unchanged")

def validate_java(record):
    from projection_trailing_source_admission_5_0_6 import current_input_matches as projection_matches

    require(record.get("complete") is True and record.get("whole_goal_complete") is False
            and record["inputs_before"] == record["inputs_after"]
            and record["references_before"] == record["references_after"], "complete stable Java frame")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists()
            and type(record["owned_bytes_removed"]) is int and record["owned_bytes_removed"] > 0,
            "measured owned Java cleanup")
    steps = record["steps"]
    require([step["label"] for step in steps] == ["fresh_current_javac", "current_shared_junit",
            "current_cpp_junit", "counterfactual_javac", "counterfactual_source_regressions"]
            and all(type(step["exit_code"]) is int for step in steps)
            and [step["exit_code"] for step in steps] == [0, 0, 0, 0, 1]
            and "OK (98 tests)" in steps[1]["stdout"] and "OK (303 tests)" in steps[2]["stdout"]
            and "Tests run: 11,  Failures: 4" in steps[-1]["stdout"],
            "401 passing tests and four actual missing-guard failures")
    seed = read(ROOT / "docs/parity/compiler_text_alias_repaired_complete_5_0_6.json.gz")
    for index, name in ((1, "shared_test_classes"), (2, "cpp_test_classes")):
        command = steps[index]["command"]
        require(record[name] == seed[name]
                and command[command.index("org.junit.runner.JUnitCore") + 1:] == seed[name],
                "complete preserved JUnit class membership")
    require(len(record["compiled_java_sources"]) == 331
            and set(record["compiled_java_sources"]) == {arg for arg in steps[0]["command"] if arg.endswith(".java")}
            and all(sha(Path(path).read_bytes()) == digest or projection_matches(path, digest)
                    for path, digest in record["compiled_java_sources"].items()),
            "complete current main/test Java inputs")
    images = record["source_images"]
    require(set(images) == {name.split(":", 1)[1] for name in (CORE, SERVICE, REQUIREMENT, TEST)},
            "three existing producer sources and one new interface")
    for name, row in images.items():
        if name == REQUIREMENT.split(":", 1)[1]:
            require(row["before_source"] is None and row["before_sha256"] is None, "one added interface")
        else:
            require(type(row["before_source"]) is str
                    and row["before_sha256"] == sha(row["before_source"].encode()), "exact producer preimages")
        require(row["after_sha256"] == sha(row["after_source"].encode())
                == sha((PRODUCER / name).read_bytes()), "actual freshly compiled producer source")
    service = images[SERVICE.split(":", 1)[1]]["after_source"]
    counterfactual = service
    for line in ("        requireSourceConfiguration(snapshot.getScope());\n",
                 "        requireSourceConfiguration(scope);\n"):
        require(counterfactual.count(line) == 1, "one public-entry guard")
        counterfactual = counterfactual.replace(line, "", 1)
    require(record["counterfactual_dispatcher_source"] == counterfactual,
            "only the two entry checks are disabled in the actual counterfactual")
    for name in ("unconfiguredCppSourceRequiresLockedCompilerBeforeParsing",
                 "unconfiguredParsedCppSourceCannotUseTheDomainFallback",
                 "unconfiguredUploadCannotPublishAnInterpretedCppSdk",
                 "unconfiguredDynamicRoutesCannotPublishAnInterpretedCppSdk"):
        require(name in steps[-1]["stdout"], "exact exposed source-entry regression: " + name)

def validate_sdks(record):
    from projection_trailing_source_admission_5_0_6 import sdk_snapshot_matches as projection_snapshot

    from reproduce_compiler_projection_sdks_5_0_6 import snapshot
    require(record.get("complete") is True and record.get("whole_goal_complete") is False
            and record["before"] == record["observed_before"] and record["resume_expected_provenance"] is False
            and len(record["before"]) == len(record["after"]) == 80
            and (record["after"] == snapshot() or projection_snapshot(record["after"], snapshot())),
            "eighty fresh current SDKs from the exact preimage")
    for before, after in zip(record["before"], record["after"], strict=True):
        expected = copy.deepcopy(before["value"])
        expected["generator_implementation_sha256"][CORE.split(":", 1)[1]] = sha(source_path(CORE).read_bytes())
        leaf = Path(before["manifest"]).name
        require(before["manifest"] == after["manifest"] and before["files"].keys() == after["files"].keys()
                and all(after["files"][key] == digest for key, digest in before["files"].items() if key != leaf)
                and after["value"] == expected, "only source-requirement emitter provenance changes")
    expected_actions = [("reproduce_cpp_contract_fixtures.py", False), ("generate_examples.py", False),
        ("reproduce_cpp_independent_sdks_5_0_6.py", False), ("reproduce_cpp_contract_fixtures.py", True),
        ("generate_examples.py", True), ("reproduce_cpp_independent_sdks_5_0_6.py", False)]
    require([(step["recipe"], "--check" in step["arguments"]) for step in record["actions"]] == expected_actions
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0
                    for step in record["steps"] + record["actions"] + [record["preparation_step"]]),
            "all actual packaging/generation/check commands")
    require(record["preparation_inputs_before"] == record["preparation_inputs_after"]
            and record["references_before"] == record["references_after"]
            and record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(),
            "stable packaging/references and owned cleanup")
    old, new = record["input_hashes_before"], record["input_hashes_after"]
    require(old.keys() == new.keys()
            and {key for key in old if old[key] != new[key]} ==
                {"runtime:" + row["manifest"] for row in record["before"]},
            "exact eighty-manifest transition")
    require(sum(len(row["files"]) - 1 for row in record["after"]) == 3560, "all retained code/Assist files")

@cache
def executed_records():
    require(sha(JAVA.read_bytes()) == JAVA_SHA and sha(SDKS.read_bytes()) == SDKS_SHA,
            "immutable actual Java and SDK records")
    java, sdks = read(JAVA), read(SDKS)
    validate_java(java)
    validate_sdks(sdks)
    require(java["inputs_after"] == sdks["preparation_inputs_before"]
            == {name: digest for name, digest in sdks["input_hashes_before"].items()
                if name != "runtime:docs/cpp_generator_parity_ledger.json"},
            "same implementation through Java qualification and SDK reproduction")
    return java, sdks

def validate(record):
    from projection_trailing_source_admission_5_0_6 import current_input_matches as projection_matches, preceding_source as projection_preimage

    java, sdks = executed_records()
    expected = {CORE, SERVICE, REQUIREMENT, TEST, *READERS,
                *("runtime:" + row["manifest"] for row in sdks["after"])}
    require(record.get("schema") == "teaql.default_source_guard.source_transition.v1"
            and record.get("complete") is True and record.get("whole_goal_complete") is False
            and record["java_sha256"] == JAVA_SHA and record["sdks_sha256"] == SDKS_SHA
            and set(record["paths"]) == expected and len(expected) == 86, "complete exact source continuation")
    for name, row in record["paths"].items():
        if name == REQUIREMENT:
            require(row["before_source"] is None and row["before_sha256"] is None, "added requirement source")
        else:
            require(type(row["before_source"]) is str
                    and row["before_sha256"] == sha(row["before_source"].encode()), "complete source preimage")
        require(row["after_sha256"] == sha(row["after_source"].encode())
                and (row["after_sha256"] == sha(source_path(name).read_bytes())
                     or projection_matches(source_path(name), row["after_sha256"])), "complete current source: " + name)
        if name.startswith("producer:"):
            require(row == java["source_images"][name.split(":", 1)[1]], "actual compiled production/regression image")
        if name in READERS:
            require(row["before_sha256"] == java["inputs_before"][name], "reader before-image belongs to actual Java epoch")
            function_changes(row["before_source"], row["after_source"], READERS[name])
    for before, after in zip(sdks["before"], sdks["after"], strict=True):
        row = record["paths"]["runtime:" + before["manifest"]]
        require(row["before_source"] == before["text"] and row["after_source"] == after["text"],
                "actual regenerated manifest images")
    binding = 'RECORD_SHA = "' + RECORD_SHA + '"\n'
    current = Path(__file__).read_text()
    if current.replace(binding, 'RECORD_SHA = "__SOURCE_SHA__"\n', 1) != record["helper_after_source_normalized"]:
        current = projection_preimage(SELF, current)
    require(current.count(binding) == 1 and current.replace(binding, 'RECORD_SHA = "__SOURCE_SHA__"\n', 1)
            == record["helper_after_source_normalized"], "complete qualified helper image")

@cache
def sources():
    require(sha(RECORD.read_bytes()) == RECORD_SHA, "immutable exact source continuation")
    record = read(RECORD)
    validate(record)
    return record["paths"]

def source_rows():
    return {**sources(), SELF: dict(before_source=None, before_sha256=None,
        after_source=Path(__file__).read_text(), after_sha256=sha(Path(__file__).read_bytes()))}

def added_input_hashes():
    return {name: row["after_sha256"] for name, row in source_rows().items() if row["before_source"] is None}

def current_input_matches(path, expected):
    from projection_trailing_source_admission_5_0_6 import current_input_matches as projection_matches

    path = Path(path).resolve()
    if type(expected) is not str or re.fullmatch(r"[0-9a-f]{64}", expected) is None or not path.is_file():
        return False
    scope = "runtime:" if path.is_relative_to(ROOT) else "producer:" if path.is_relative_to(PRODUCER) else None
    if scope is None:
        return False
    base = ROOT if scope == "runtime:" else PRODUCER
    row = source_rows().get(scope + path.relative_to(base).as_posix())
    return (row is not None and expected in (row["before_sha256"], row["after_sha256"])) or projection_matches(path, expected)

def qualified_transition(name, before, after):
    from projection_trailing_source_admission_5_0_6 import qualified_transition as projection_transition

    row = source_rows().get(name)
    return (row is not None and row["before_sha256"] == before
            and (row["after_sha256"] == after or projection_transition(name, row["after_sha256"], after))) \
        or projection_transition(name, before, after)

def preceding_source(name, current):
    row = sources().get(name)
    if row is not None and current != row["after_source"]:
        from projection_trailing_source_admission_5_0_6 import preceding_source as projection_preimage
        current = projection_preimage(name, current)
    require(row is not None and row["before_source"] is not None and current == row["after_source"],
            "one exact preceding reader/emitter image")
    return row["before_source"]

def preceding_fingerprint(name):
    row = sources().get(name)
    require(row is not None and row["before_sha256"] is not None, "one exact preceding source fingerprint")
    return row["before_sha256"]

def sdk_snapshot_matches(archived, observed):
    from projection_trailing_source_admission_5_0_6 import sdk_snapshot_matches as projection_snapshot

    sources()
    record = read(SDKS)
    return (archived == record["before"] and (observed == record["after"]
            or projection_snapshot(record["after"], observed))) or projection_snapshot(archived, observed)

def controls(record):
    cases = [
        ("numeric_complete", lambda r: r.update(complete=1)),
        ("goal_promoted", lambda r: r.update(whole_goal_complete=True)),
        ("lost_requirement", lambda r: r["paths"].pop(REQUIREMENT)),
        ("lost_manifest", lambda r: r["paths"].pop("runtime:examples/generated/manifest.json")),
        ("changed_source", lambda r: r["paths"][SERVICE].update(after_source="class Altered {}")),
        ("changed_helper", lambda r: r.update(helper_after_source_normalized="pass")),
    ]
    results = []
    for name, mutate in cases:
        altered = copy.deepcopy(record)
        mutate(altered)
        try:
            validate(altered)
        except (ValueError, RuntimeError, KeyError, TypeError, IndexError) as error:
            results.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise ValueError("Invalid default source continuation admitted: " + name)
    for path, validator, mutations in [
        (JAVA, validate_java, [
            ("unfinished_java", lambda r: r.update(complete=False)),
            ("boolean_javac_exit", lambda r: r["steps"][0].update(exit_code=False)),
            ("hidden_counterfactual", lambda r: r["steps"][-1].update(exit_code=0)),
            ("missing_java_input", lambda r: r["compiled_java_sources"].pop(next(iter(r["compiled_java_sources"])))),
            ("changed_dispatcher", lambda r: r.update(counterfactual_dispatcher_source="class Altered {}")),
            ("missing_junit_class", lambda r: r["cpp_test_classes"].pop()),
            ("lost_existing_preimage", lambda r: r["source_images"][CORE.split(":", 1)[1]].update(
                before_source=None, before_sha256=None)),
        ]),
        (SDKS, validate_sdks, [
            ("numeric_sdk_complete", lambda r: r.update(complete=1)),
            ("missing_sdk", lambda r: r["after"].pop()),
            ("forged_sdk_file", lambda r: r["after"][0]["files"].update(invented="0" * 64)),
            ("lost_sdk_action", lambda r: r["actions"].pop()),
            ("boolean_packaging_exit", lambda r: r["preparation_step"].update(exit_code=False)),
            ("unreleased_sdk_output", lambda r: r.update(owned_directory_removed=False)),
        ]),
    ]:
        for name, mutate in mutations:
            altered = read(path)
            mutate(altered)
            try:
                validator(altered)
            except (ValueError, RuntimeError, KeyError, TypeError, IndexError) as error:
                results.append(dict(case=name, rejected=True, error=str(error)))
            else:
                raise ValueError("Invalid default source execution admitted: " + name)
    return results

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(not args.record or not args.record.exists(), "preserve source controls")
    sources()
    results = controls(read(RECORD))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True, whole_goal_complete=False,
            source_sha256=RECORD_SHA, results=results), sort_keys=True).encode(), mtime=0))
    print(f"Default source guard qualified; {len(results)} corruptions rejected")

if __name__ == "__main__":
    main()
