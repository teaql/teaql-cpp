#!/usr/bin/env python3
"""Admit the exact approved generation change without rewriting frozen executions."""

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
RECORD = ROOT / "docs/parity/approved_generation_decisions_sources_summary_repaired_5_0_6.json.gz"
RECORD_SHA = "cb611f10f18022bb98fc1715b45185b823cf8cde60d16e91117a0c682d21e92e"
PRECEDING = ROOT / "docs/parity/approved_generation_decisions_sources_5_0_6.json.gz"
PRECEDING_SHA = "4ec1a6289873295caf98dd9c39bc72dcc840ed0fb7f88d78beaf01e6e28fbdc0"
JAVA_RECORD = ROOT / "docs/parity/approved_generation_decisions_java_5_0_6.json.gz"
JAVA_SHA = "dd610710e336b510e30b8dd2e5baa23d6079e7614f7cd5bcc859edcec9da23d8"
SELF = "runtime:scripts/generation_decision_source_admission_5_0_6.py"
OFFLINE = "runtime:scripts/compiler_offline_source_admission_5_0_6.py"
NARROW = "runtime:scripts/narrow_integer_registration_source_admission_5_0_6.py"
DYNAMIC = "runtime:scripts/compiler_dynamic_route_source_admission_5_0_6.py"
RELATION = "runtime:scripts/relation_analysis_cli_source_binding_5_0_6.py"
CUSTOM = "runtime:scripts/verify_custom_comparison_bindings_5_0_6.py"
READER = "runtime:scripts/verify_generation_decisions_5_0_6.py"
MANIFEST = "runtime:docs/compatibility_exceptions_5_0_6.json"
GOVERNANCE = "runtime:docs/governance_qualification_updates_5_0_6.json"
PRODUCER_DOC = "producer:docs/cpp_compiler_host.md"


def require(condition, message):
    if not condition:
        raise ValueError("Generation decision source admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(Path(path).read_bytes()))


def source_path(name):
    scope, relative = name.split(":", 1)
    require(scope in ("runtime", "producer") and relative and not Path(relative).is_absolute()
            and ".." not in Path(relative).parts, "canonical scoped source path")
    return (ROOT if scope == "runtime" else PRODUCER) / relative


def function_changes(before, after, expected):
    trees = [ast.parse(text) for text in (before, after)]
    functions = [{node.name: ast.dump(node) for node in tree.body if isinstance(node, ast.FunctionDef)}
                 for tree in trees]
    require(functions[0].keys() == functions[1].keys()
            and {name for name in functions[0] if functions[0][name] != functions[1][name]} == set(expected),
            "exact identity-only function changes")
    for tree in trees:
        for node in tree.body:
            if isinstance(node, ast.FunctionDef) and node.name in expected:
                node.body = [ast.Pass()]
    require(ast.dump(trees[0]) == ast.dump(trees[1]), "all other execution, comparison and control bodies stay exact")


def validate(record):
    import verify_generation_decisions_5_0_6 as decisions

    require(sha(JAVA_RECORD.read_bytes()) == JAVA_SHA, "immutable actual Java compiler/JUnit execution")
    java = read(JAVA_RECORD)
    decisions.validate_java(java)
    rows = record["paths"]
    expected = {OFFLINE, NARROW, DYNAMIC, RELATION, CUSTOM, READER, MANIFEST, GOVERNANCE, PRODUCER_DOC}
    expected |= {"producer:" + name for name in decisions.CHANGED}
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and set(rows) == expected and record["java_receipt_sha256"] == JAVA_SHA,
            "one factory, three Java contracts, two source guards and explicit decision data")
    for name, row in rows.items():
        require(row["after_sha256"] == sha(row["after_source"].encode())
                == sha(source_path(name).read_bytes()), "exact current source image: " + name)
        if row["before_source"] is None:
            require(name == READER and row["before_sha256"] is None, "only the new decision reader has no preimage")
        else:
            require(row["before_sha256"] == sha(row["before_source"].encode()), "complete source preimage")
        if name in {"producer:" + key for key in decisions.CHANGED}:
            require(row == java["source_images"][name.split(":", 1)[1]], "actual freshly compiled Java source transition")
    require(rows[READER]["after_sha256"] == java["inputs_after"][READER], "unchanged actual Java collector/reader")
    for name in (OFFLINE, NARROW, DYNAMIC, RELATION, CUSTOM, GOVERNANCE):
        require(rows[name]["before_sha256"] == java["inputs_after"][name], "actual preceding execution frame: " + name)
    require(rows[MANIFEST]["after_sha256"] == java["inputs_after"][MANIFEST], "approved exception data used by the Java collector")
    function_changes(rows[OFFLINE]["before_source"], rows[OFFLINE]["after_source"],
                     {"validate_reader_continuation", "validate_sources", "source_epochs", "added_input_hashes"})
    function_changes(rows[NARROW]["before_source"], rows[NARROW]["after_source"],
                     {"validate_sources", "current_source_matches", "validate_reader_correction",
                      "reader_sources", "corrected_source_matches", "validate_continuation"})
    function_changes(rows[DYNAMIC]["before_source"], rows[DYNAMIC]["after_source"], {"validate_sources"})
    function_changes(rows[CUSTOM]["before_source"], rows[CUSTOM]["after_source"], {"validate"})
    relation = rows[RELATION]
    edits = [
        ('    def validate_files(self):\n', '    def validate_files(self):\n'
         '        from generation_decision_source_admission_5_0_6 import current_input_matches as decision_matches\n'),
        ("            require(digest(ROOT / path) == wanted, 'current source support: ' + path)\n",
         "            require(digest(ROOT / path) == wanted or decision_matches(ROOT / path, wanted),\n"
         "                    'current source support: ' + path)\n"),
        ("            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],\n",
         "            require(digest(ROOT / contract[key]) == contract[key + '_sha256']\n"
         "                    or key == 'exception_manifest' and decision_matches(\n"
         "                        ROOT / contract[key], contract[key + '_sha256']),\n"),
    ]
    changed = relation["before_source"]
    for before_text, after_text in edits:
        require(changed.count(before_text) == 1, "one exact relationship identity guard")
        changed = changed.replace(before_text, after_text, 1)
    require(changed == relation["after_source"], "all relationship source, command, native and corruption checks remain exact")
    contract = json.loads(rows[MANIFEST]["after_source"])
    decisions.validate_exceptions(contract)
    prior = copy.deepcopy(contract)
    for name in decisions.EXCEPTIONS:
        prior.pop(name)
    require(prior == json.loads(rows[MANIFEST]["before_source"]), "all earlier exception data remains exact")
    before, after = [json.loads(rows[GOVERNANCE][key]) for key in ("before_source", "after_source")]
    require(before["pending_compatibility_decisions"] == ["option_model", "column_attribute"]
            and after["pending_compatibility_decisions"] == [], "only answered choices leave the pending list")
    restored = copy.deepcopy(after)
    approved = restored.pop("approved_compatibility_decisions")
    require(approved == {name: dict(approved_on="2026-11-20", contract=decisions.EXCEPTIONS[name]["documentation"])
                         for name in decisions.EXCEPTIONS}, "exact independently approved governance scopes")
    restored["pending_compatibility_decisions"] = before["pending_compatibility_decisions"]
    require(restored == before, "GP11/GP16 and all other governance dispositions remain exact")
    require(sha(PRECEDING.read_bytes()) == PRECEDING_SHA
            and record["preceding_source_proof_sha256"] == PRECEDING_SHA, "first qualification and failed native epoch stay immutable")
    preceding = read(PRECEDING)
    require(record["helper_before_source"] == preceding["helper_after_source_normalized"].replace(
        '__RECORD_SHA256__', PRECEDING_SHA, 1), "exact preceding helper image")
    require(all(name in rows and rows[name]["before_source"] == row["before_source"]
                and rows[name]["before_sha256"] == row["before_sha256"]
                for name, row in preceding["paths"].items()), "all original source preimages remain exact")
    current = Path(__file__).read_text()
    binding = 'RECORD_SHA = "' + RECORD_SHA + '"\n'
    require(current.count(binding) == 1 and current.replace(binding, 'RECORD_SHA = "__RECORD_SHA256__"\n', 1)
            == record["helper_after_source_normalized"], "complete new helper image; only its own receipt hash is normalized")


@cache
def sources():
    require(sha(RECORD.read_bytes()) == RECORD_SHA, "immutable complete generation-decision source proof")
    record = read(RECORD)
    validate(record)
    return record["paths"]


def source_rows():
    return {**sources(), SELF: dict(before_source=None, before_sha256=None,
                                    after_source=Path(__file__).read_text(), after_sha256=sha(Path(__file__).read_bytes()))}


def added_input_hashes():
    return {name: row["after_sha256"] for name, row in source_rows().items() if row["before_source"] is None}


def current_input_matches(path, expected):
    path = Path(path).resolve()
    if type(expected) is not str or re.fullmatch(r"[0-9a-f]{64}", expected) is None or not path.is_file():
        return False
    prefix = "runtime:" if path.is_relative_to(ROOT) else "producer:" if path.is_relative_to(PRODUCER) else None
    if prefix is None:
        return False
    relative = path.relative_to(ROOT if prefix == "runtime:" else PRODUCER).as_posix()
    row = source_rows().get(prefix + relative)
    return row is not None and expected in (row["before_sha256"], row["after_sha256"])


def previous_source(name, current):
    row = sources().get(name)
    require(name == NARROW and row is not None and current == row["after_source"],
            "only the exact narrow-reader continuation may consume its prior image")
    return row["before_source"]


def controls(record):
    variants = [
        ("missing_source", lambda x: x["paths"].pop(OFFLINE)),
        ("invented_source", lambda x: x["paths"].update({"runtime:invented.py": x["paths"][OFFLINE]})),
        ("incomplete_proof", lambda x: x.update(complete=1)),
        ("goal_promoted", lambda x: x.update(whole_goal_complete=True)),
        ("wrong_java_execution", lambda x: x.update(java_receipt_sha256="0" * 64)),
        ("factory_changed", lambda x: x["paths"]["producer:generator/src/main/java/com/skynet/codegenerator/cpp/CppCompilerServiceConfiguration.java"].update(after_source="return interpreted;")),
        ("source_guard_changed", lambda x: x["paths"][OFFLINE].update(after_sha256="0" * 64)),
        ("prior_source_changed", lambda x: x["paths"][NARROW].update(before_sha256="0" * 64)),
        ("approval_changed", lambda x: x["paths"][MANIFEST].update(after_source="{}")),
        ("governance_promoted", lambda x: x["paths"][GOVERNANCE].update(after_source="{}")),
        ("helper_changed", lambda x: x.update(helper_after_source_normalized="pass\n")),
    ]
    results = []
    for name, mutation in variants:
        changed = copy.deepcopy(record)
        mutation(changed)
        try:
            validate(changed)
        except (ValueError, KeyError, TypeError) as error:
            results.append(dict(name=name, rejected=True, error=str(error)))
        else:
            raise ValueError("Invalid source proof admitted: " + name)
    import verify_generation_decisions_5_0_6 as decisions
    java = read(JAVA_RECORD)
    for name, mutation in (
        ("unfinished_java", lambda x: x.update(complete=1)),
        ("missing_regression", lambda x: x.update(regression_failure_count=4)),
        ("boolean_test_count", lambda x: x.update(java_test_count=True)),
        ("failed_javac", lambda x: x["steps"][0].update(exit_code=1)),
        ("missing_test_class", lambda x: x["test_classes"].pop()),
        ("lost_cleanup", lambda x: x.update(owned_directory_removed=False)),
        ("changed_epoch", lambda x: x["inputs_after"].update({READER: "0" * 64})),
    ):
        changed = copy.deepcopy(java)
        mutation(changed)
        try:
            decisions.validate_java(changed)
        except (ValueError, KeyError, TypeError) as error:
            results.append(dict(name=name, rejected=True, error=str(error)))
        else:
            raise ValueError("Invalid Java receipt admitted: " + name)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error("Preserve existing source qualification receipts")
    sources()
    results = controls(read(RECORD))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True, whole_goal_complete=False,
            source_receipt_sha256=RECORD_SHA, java_receipt_sha256=JAVA_SHA, results=results), sort_keys=True).encode(), mtime=0))
    print("Exact generation source transition qualified; 18 invalid source/Java controls rejected")


if __name__ == "__main__":
    main()
