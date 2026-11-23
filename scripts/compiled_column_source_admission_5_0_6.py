#!/usr/bin/env python3
"""Admit the compiled-column guard and exact regenerated SDK/source transitions."""

import argparse
import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
RECORD = ROOT / "docs/parity/compiled_column_sources_repaired_5_0_6.json.gz"
RECORD_SHA = "d49aaafab67dee578b0bc57137a80114e72827c12d59820be600fdaf1bb0951a"
EXECUTION = ROOT / "docs/parity/compiled_column_java_5_0_6.json.gz"
EXECUTION_SHA = "955979037a3c66f21d34973c9658ded6df0951116ba69825b7addfebbcc4c21d"
SDKS = ROOT / "docs/parity/compiled_column_sdks_complete_5_0_6.json.gz"
SDKS_SHA = "50399eccab04e5694d27a9f35534635d63f98e079d440c89ced28f9bdf09e91e"
SELF = "runtime:scripts/compiled_column_source_admission_5_0_6.py"
READERS = {
    "runtime:scripts/generation_decision_source_admission_5_0_6.py": {"current_input_matches"},
    "runtime:scripts/verify_option_keyword_policy_5_0_6.py": {"validate"},
    "runtime:scripts/option_keyword_source_admission_5_0_6.py": {"validate", "current_input_matches", "preceding_source"},
    "runtime:scripts/compiler_offline_source_admission_5_0_6.py": {"source_epochs", "added_input_hashes"},
    "runtime:scripts/compiler_offline_input_source_admission_5_0_6.py": {"manifest_rows", "qualified_transition"},
}
JAVA_DRIVER = "runtime:scripts/verify_compiled_column_boundary_5_0_6.py"
SDK_DRIVER = "runtime:scripts/reproduce_compiled_column_sdks_5_0_6.py"
RECIPE = "runtime:scripts/generate_examples.py"


def require(condition, message):
    if not condition:
        raise ValueError("Compiled column source admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(Path(path).read_bytes()))


def source_path(name):
    scope, relative = name.split(":", 1)
    require(scope in ("runtime", "producer") and relative and not Path(relative).is_absolute()
            and ".." not in Path(relative).parts, "canonical source identity")
    return (ROOT if scope == "runtime" else PRODUCER) / relative


def validate(record):
    import verify_compiled_column_boundary_5_0_6 as java
    import reproduce_compiled_column_sdks_5_0_6 as sdks
    from option_keyword_source_admission_5_0_6 import function_changes
    from text_alias_source_admission_5_0_6 import current_input_matches as text_matches, preceding_source as text_preimage

    require(sha(EXECUTION.read_bytes()) == EXECUTION_SHA and sha(SDKS.read_bytes()) == SDKS_SHA,
            "immutable actual Java and eighty-SDK executions")
    execution, artifacts = read(EXECUTION), read(SDKS)
    java.validate(execution)
    sdks.validate(artifacts)
    require(set(execution["compiled_sources"]) == {
        name for name in execution["steps"][0]["command"] if name.endswith(".java")},
        "complete actual Java compiler input membership")
    expected = {*READERS, JAVA_DRIVER, SDK_DRIVER, RECIPE,
                *("producer:" + name for name in execution["source_images"]),
                *("runtime:" + row["manifest"] for row in artifacts["after"])}
    require(record.get("complete") is True and record.get("whole_goal_complete") is False
            and record["execution_sha256"] == EXECUTION_SHA and record["sdks_sha256"] == SDKS_SHA
            and set(record["paths"]) == expected and len(expected) == 90,
            "two producer files, exact reader/recipe transitions and all eighty manifest paths")
    require(record.get("schema") == "teaql.compiled_column.source_transition.v2", "current reader repair schema")
    preceding_path = ROOT / "docs/parity/compiled_column_sources_5_0_6.json.gz"
    failed_path = ROOT / "docs/parity/compiled_column_native_5_0_6.json.gz"
    require(record["preceding_source_sha256"] == sha(preceding_path.read_bytes()) == "b1ff6fb4b4d179904455a132df2817b6a8625f84bb98101bd7beffddeace5ecf"
            and record["failed_native_sha256"] == sha(failed_path.read_bytes()) == "75516f240e08bb5660f1b1d7f9a599235032ef6087e91ce79574c80eb903a659",
            "original source proof and failed native epoch stay immutable")
    preceding = read(preceding_path)
    require(all(name in record["paths"] and record["paths"][name]["before_source"] == row["before_source"]
                and record["paths"][name]["before_sha256"] == row["before_sha256"]
                for name, row in preceding["paths"].items()), "all earlier source preimages stay exact")
    rows = record["paths"]
    for name, row in rows.items():
        require(row["after_sha256"] == sha(row["after_source"].encode())
                and (row["after_sha256"] == sha(source_path(name).read_bytes())
                     or text_matches(source_path(name), row["after_sha256"])), "complete current source: " + name)
        if name in (JAVA_DRIVER, SDK_DRIVER):
            require(row["before_source"] is None and row["before_sha256"] is None, "new bounded collectors")
        else:
            require(row["before_sha256"] == sha(row["before_source"].encode()), "complete exact source preimage")
        if name.startswith("producer:"):
            require(row == execution["source_images"][name.split(":", 1)[1]],
                    "actual freshly compiled guard and regression tests")
        elif name in READERS or name == RECIPE:
            require(row["before_sha256"] == execution["inputs_before"][name],
                    "preceding actual Java execution frame")
        if name in READERS:
            function_changes(row["before_source"], row["after_source"], READERS[name])
    row = rows[RECIPE]
    old = "str(ROOT / model), str(output)]"
    new = "str(ROOT / model), str(output), '--qualify-inheritance']"
    require(row["before_source"].count(old) == 1
            and row["before_source"].replace(old, new, 1) == row["after_source"],
            "only explicitly select qualification for retained development examples")
    require(rows[JAVA_DRIVER]["after_sha256"] == execution["inputs_before"][JAVA_DRIVER]
            and rows[SDK_DRIVER]["after_source"] == artifacts["driver_source"]
            and rows[RECIPE]["after_sha256"] == artifacts["input_hashes_before"][RECIPE],
            "actual collector and recipe sources")
    for before, after in zip(artifacts["before"], artifacts["after"], strict=True):
        row = rows["runtime:" + before["manifest"]]
        leaf = Path(before["manifest"]).name
        require(row["before_source"] == before["text"] and row["after_source"] == after["text"]
                and row["before_sha256"] == before["files"][leaf]
                and row["after_sha256"] == after["files"][leaf], "actual regenerated manifest source images")
    binding = 'RECORD_SHA = "' + RECORD_SHA + '"\n'
    current = Path(__file__).read_text()
    if current.replace(binding, 'RECORD_SHA = "__RECORD_SHA256__"\n', 1) != record["helper_after_source_normalized"]:
        current = text_preimage(SELF, current)
    require(current.count(binding) == 1
            and current.replace(binding, 'RECORD_SHA = "__RECORD_SHA256__"\n', 1)
            == record["helper_after_source_normalized"], "complete helper image with only its own receipt hash normalized")


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
    from text_alias_source_admission_5_0_6 import current_input_matches as text_matches

    path = Path(path).resolve()
    if type(expected) is not str or re.fullmatch(r"[0-9a-f]{64}", expected) is None or not path.is_file():
        return False
    scope = "runtime:" if path.is_relative_to(ROOT) else "producer:" if path.is_relative_to(PRODUCER) else None
    if scope is None:
        return False
    base = ROOT if scope == "runtime:" else PRODUCER
    row = source_rows().get(scope + path.relative_to(base).as_posix())
    return (row is not None and expected in (row["before_sha256"], row["after_sha256"])) or text_matches(path, expected)


def qualified_transition(name, before, after):
    from text_alias_source_admission_5_0_6 import qualified_transition as text_transition

    row = source_rows().get(name)
    return (row is not None and row["before_sha256"] == before
            and (row["after_sha256"] == after or text_transition(name, row["after_sha256"], after))) \
        or text_transition(name, before, after)


def preceding_source(name, current):
    row = sources().get(name)
    require(name in READERS and row is not None and current == row["after_source"],
            "one exact preceding reader image")
    return row["before_source"]


def controls(record):
    changes = [
        ("numeric_complete", lambda x: x.update(complete=1)),
        ("goal_promoted", lambda x: x.update(whole_goal_complete=True)),
        ("missing_guard", lambda x: x["paths"].pop(
            "producer:generator/src/main/java/com/skynet/codegenerator/cpp/CppCompilerModelConfiguration.java")),
        ("missing_sdk", lambda x: x["paths"].pop("runtime:examples/generated/manifest.json")),
        ("wrong_java", lambda x: x.update(execution_sha256="0" * 64)),
        ("wrong_sdks", lambda x: x.update(sdks_sha256="0" * 64)),
        ("changed_reader", lambda x: x["paths"][next(iter(READERS))].update(after_source="pass")),
        ("changed_preimage", lambda x: x["paths"][RECIPE].update(before_sha256="0" * 64)),
        ("changed_helper", lambda x: x.update(helper_after_source_normalized="pass\n")),
        ("implicit_formal_entry", lambda x: x["paths"][RECIPE].update(
            after_source=x["paths"][RECIPE]["before_source"])),
        ("changed_collector", lambda x: x["paths"][JAVA_DRIVER].update(after_source="pass")),
        ("changed_sdk_provenance", lambda x: x["paths"]["runtime:examples/generated/manifest.json"].update(
            after_sha256="0" * 64)),
    ]
    results = []
    for name, change in changes:
        altered = copy.deepcopy(record)
        change(altered)
        try:
            validate(altered)
        except (ValueError, RuntimeError, KeyError, TypeError, IndexError) as error:
            results.append(dict(name=name, rejected=True, reason=str(error)))
        else:
            raise ValueError("Invalid compiled column source transition admitted: " + name)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(not args.record or not args.record.exists(), "preserve prior control evidence")
    sources()
    results = controls(read(RECORD))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True, whole_goal_complete=False,
            source_receipt_sha256=RECORD_SHA, execution_sha256=EXECUTION_SHA, sdks_sha256=SDKS_SHA,
            results=results), sort_keys=True).encode(), mtime=0))
    print("Compiled column source/SDK transitions qualified; 12 corruptions rejected")


if __name__ == "__main__":
    main()
