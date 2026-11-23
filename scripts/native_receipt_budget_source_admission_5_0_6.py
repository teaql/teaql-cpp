#!/usr/bin/env python3
"""Admit five measured receipt timeouts without changing test semantics."""

import argparse
import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SELF = "runtime:scripts/native_receipt_budget_source_admission_5_0_6.py"
PROJECTION = "runtime:scripts/projection_trailing_source_admission_5_0_6.py"
CMAKE = "runtime:tests/CMakeLists.txt"
MODULE = "runtime:modules/source_contracts.json"
BASE = ROOT / "docs/parity/current_full_native_after_keyword_inputs_5_0_6.json.gz"
BASE_SHA = "b72551020e21f447a4664903e6da0706b66bdf3c7f11464bd33b7c46c593bd8d"
RECORD = ROOT / "docs/parity/native_receipt_budget_sources_with_boundary_path_5_0_6.json.gz"
RECORD_SHA = "65b8964422e4cd0baa6ee016d160f7187a2fde9dca49cf65536c81e813b4ca32"
BOUNDARY = ROOT / "docs/parity/registered_boundary_dependency_sources_with_path_5_0_6.json.gz"
BOUNDARY_SHA = "b29b9b3a064d4d72697ca59047fc6287ee6bc1f819bd409c1a84040b2ff52871"
TIMEOUTS = {
    "generated_credential_mutation_receipt_contracts": 60,
    "parity_audit_cross_context_corpus_contracts": 120,
    "parity_ap02_reverse_relation_type_corpus": 120,
    "parity_ap02_reverse_relation_sql_receipt": 120,
    "parity_ap02_cross_context_partial_sql_receipt": 120,
}
EDITS = (
    ('    current = Path(__file__).read_text()\n',
     '    from native_receipt_budget_source_admission_5_0_6 import preceding_source\n'
     '    current = preceding_source(SELF, Path(__file__).read_text())\n'),
    ('def source_rows():\n'
     '    return {**sources(), SELF: dict(before_source=None, before_sha256=None,\n'
     '        after_source=Path(__file__).read_text(), after_sha256=sha(Path(__file__).read_bytes()))}\n',
     'def source_rows():\n'
     '    from native_receipt_budget_source_admission_5_0_6 import source_rows as budget_rows\n'
     '    return {**sources(), SELF: dict(before_source=None, before_sha256=None,\n'
     '        after_source=Path(__file__).read_text(), after_sha256=sha(Path(__file__).read_bytes())),\n'
     '        **budget_rows()}\n'),
    ('def added_input_hashes():\n'
     '    return {SELF: sha(Path(__file__).read_bytes())} if sources() else {}\n',
     'def added_input_hashes():\n'
     '    from native_receipt_budget_source_admission_5_0_6 import added_input_hashes as budget_added\n'
     '    return {SELF: sha(Path(__file__).read_bytes()), **budget_added()} if sources() else {}\n'),
)


def require(condition, message):
    if not condition:
        raise RuntimeError("Native receipt budget admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def validate(record):
    require(record.get("schema") == "teaql.native_receipt_budget.sources.v1"
            and record.get("complete") is True and record.get("whole_goal_complete") is False
            and record.get("fresh_business_execution") is False
            and record.get("native_acceptance") is False,
            "source-only transition without a native or whole-goal claim")
    require(sha(BASE.read_bytes()) == BASE_SHA and record["base_sha256"] == BASE_SHA,
            "immutable complete failed-native input frame")
    base = read(BASE)
    require(len(base["hashes"]) == 12149 and base["references"] == {
        "teaql-rs": "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3",
        "teaql_code_gen": "45e1572c4956ca963410d8ea436ead3c8857087a"},
        "complete locked pre-repair source epoch")
    rows = record["paths"]
    require(set(rows) == {CMAKE, MODULE, PROJECTION}, "exact three existing source inputs")
    for name, row in rows.items():
        require(type(row["before_source"]) is str and type(row["after_source"]) is str
                and row["before_sha256"] == sha(row["before_source"].encode())
                == base["hashes"][name]
                and row["after_sha256"] == sha(row["after_source"].encode()),
                "complete before/after source images: " + name)
    cmake = rows[CMAKE]["before_source"]
    for name, old_timeout in TIMEOUTS.items():
        before = f"set_tests_properties({name} PROPERTIES TIMEOUT {old_timeout})"
        require(cmake.count(before) == 1, "one registered timeout for " + name)
        cmake = cmake.replace(before, f"set_tests_properties({name} PROPERTIES TIMEOUT 600)", 1)
    require(cmake == rows[CMAKE]["after_source"],
            "only five time budgets change; commands, fixtures and other declarations stay exact")
    old, new = [json.loads(rows[MODULE][key]) for key in ("before_source", "after_source")]
    require(old["native_registration_sha256"] == rows[CMAKE]["before_sha256"],
            "actual preceding declaration identity")
    old["native_registration_sha256"] = rows[CMAKE]["after_sha256"]
    require(old == new, "only derived native declaration identity changes in module bindings")
    projection = rows[PROJECTION]["before_source"]
    for before, after in EDITS:
        require(projection.count(before) == 1, "one exact identity forwarding edit")
        projection = projection.replace(before, after, 1)
    require(projection == rows[PROJECTION]["after_source"],
            "only source identity forwarding changes; all existing validation and controls stay exact")
    for name, row in rows.items():
        require(sha((ROOT / name.split(":", 1)[1]).read_bytes()) == row["after_sha256"],
                "installed source matches qualified transition: " + name)
    require(record["boundary_sources_sha256"] == BOUNDARY_SHA
            and sha(BOUNDARY.read_bytes()) == BOUNDARY_SHA,
            "exact separately qualified boundary dependency repair")
    boundary = read(BOUNDARY)
    normalized = boundary["helper_after_source_normalized"]
    placeholder = 'RECORD_SHA = "__BOUNDARY_SOURCE_SHA__"\n'
    require(normalized.count(placeholder) == 1, "one bounded boundary-helper source binding")
    expected = normalized.replace(placeholder, 'RECORD_SHA = "' + BOUNDARY_SHA + '"\n', 1)
    require(sha((ROOT / "scripts/registered_boundary_source_admission_5_0_6.py").read_bytes())
            == sha(expected.encode()), "complete known boundary helper checked before import")
    binding = 'RECORD_SHA = "' + RECORD_SHA + '"\n'
    current = Path(__file__).read_text()
    require(current.count(binding) == 1
            and current.replace(binding, 'RECORD_SHA = "__BUDGET_SOURCE_SHA__"\n', 1)
            == record["helper_after_source_normalized"], "complete bounded helper source")


@cache
def sources():
    require(sha(RECORD.read_bytes()) == RECORD_SHA, "immutable bounded timeout source transition")
    record = read(RECORD)
    validate(record)
    return record["paths"]


def source_rows():
    sources()
    from registered_boundary_source_admission_5_0_6 import source_rows as boundary_rows
    return {**sources(), SELF: dict(before_source=None, before_sha256=None,
        after_source=Path(__file__).read_text(), after_sha256=sha(Path(__file__).read_bytes())),
        **boundary_rows()}


def added_input_hashes():
    sources()
    from registered_boundary_source_admission_5_0_6 import added_input_hashes as boundary_added
    return {SELF: sha(Path(__file__).read_bytes()), **boundary_added()}


def preceding_source(name, current):
    row = sources().get(name)
    require(name == PROJECTION and row is not None and type(current) is str
            and current == row["after_source"], "one exact preceding helper image")
    return row["before_source"]


def controls(record):
    def alter(row, name, before, after):
        item = row["paths"][name]
        require(item["after_source"].count(before) == 1, "one deliberate corruption site")
        item["after_source"] = item["after_source"].replace(before, after, 1)
        item["after_sha256"] = sha(item["after_source"].encode())

    def alter_module(row):
        item = row["paths"][MODULE]
        module = json.loads(item["after_source"])
        module["bindings"][0]["command"].append("--unqualified")
        item["after_source"] = json.dumps(module, indent=2) + "\n"
        item["after_sha256"] = sha(item["after_source"].encode())

    changes = (
        ("numeric_complete", lambda r: r.update(complete=1)),
        ("promoted_goal", lambda r: r.update(whole_goal_complete=True)),
        ("claimed_business_execution", lambda r: r.update(fresh_business_execution=True)),
        ("claimed_native_acceptance", lambda r: r.update(native_acceptance=True)),
        ("wrong_schema", lambda r: r.update(schema="other")),
        ("wrong_base", lambda r: r.update(base_sha256="0" * 64)),
        ("wrong_boundary_proof", lambda r: r.update(boundary_sources_sha256="0" * 64)),
        ("numeric_boundary_proof", lambda r: r.update(boundary_sources_sha256=1)),
        ("missing_input", lambda r: r["paths"].pop(MODULE)),
        ("extra_input", lambda r: r["paths"].update({"runtime:unknown": {}})),
        ("wrong_preimage", lambda r: r["paths"][CMAKE].update(before_sha256="0" * 64)),
        ("wrong_afterimage", lambda r: r["paths"][CMAKE].update(after_sha256="0" * 64)),
        ("unlimited_timeout", lambda r: alter(r, CMAKE,
            "set_tests_properties(generated_credential_mutation_receipt_contracts PROPERTIES TIMEOUT 600)",
            "set_tests_properties(generated_credential_mutation_receipt_contracts PROPERTIES TIMEOUT 0)")),
        ("changed_registration", lambda r: alter(r, CMAKE,
            "add_test(NAME generated_credential_mutation_receipt_contracts COMMAND",
            "add_test(NAME changed_receipt_contracts COMMAND")),
        ("changed_module_binding", alter_module),
        ("removed_projection_checks", lambda r: alter(r, PROJECTION,
            'def validate(record):', 'def skipped_validation(record):')),
        ("changed_helper", lambda r: r.update(helper_after_source_normalized="pass\n")),
    )
    rejected = []
    for name, change in changes:
        altered = copy.deepcopy(record)
        change(altered)
        try:
            validate(altered)
        except (RuntimeError, ValueError, TypeError, KeyError):
            rejected.append(name)
        else:
            raise RuntimeError("Corrupt timeout source proof accepted: " + name)
    for name, current in ((CMAKE, record["paths"][CMAKE]["after_source"]),
                          (PROJECTION, "unqualified source"), (PROJECTION, None)):
        try:
            preceding_source(name, current)
        except (RuntimeError, TypeError, KeyError):
            rejected.append("invalid_preceding_source_" + str(len(rejected)))
        else:
            raise RuntimeError("Unqualified preceding source accepted")
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "preserve prior evidence")
    record = read(RECORD)
    sources()
    rejected = controls(record)
    result = dict(complete=True, whole_goal_complete=False, native_acceptance=False,
        optimized=__import__("sys").flags.optimize > 0, source_receipt_sha256=RECORD_SHA,
        timeout_changes=5, controls=rejected)
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(), mtime=0))
    print("Five exact receipt budgets; " + str(len(rejected)) + " corruptions rejected")


if __name__ == "__main__":
    main()
