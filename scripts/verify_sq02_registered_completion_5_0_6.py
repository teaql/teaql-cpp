#!/usr/bin/env python3
"""Reconcile the finite registered SQ02 cohort with its qualified source evidence."""

import argparse
import ast
from collections import Counter
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

import nominal_type_source_admission_5_0_6 as nominal
import verify_custom_comparison_bindings_5_0_6 as custom
import verify_direct_corpus_bindings_5_0_6 as direct
import verify_paired_source_bindings_5_0_6 as paired
from collect_source_control_execution_5_0_6 import native_hashes
from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references


ROOT = Path(__file__).resolve().parents[1]
SELF = Path(__file__).relative_to(ROOT).as_posix()
RUNTIME = "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3"
GENERATOR = "45e1572c4956ca963410d8ea436ead3c8857087a"
ANCHORS = {
    "source_control_installed_role_inventory": "4fc6da2f72aa93046801231db226375093830f64fad3255d1c136d5194fcc350",
    "source_control_complete_dispositions": "99547eb183395ac6cb68ee6a41fa4ae37a9eb0d394938a2c1c426d66a0ecaf43",
    "source_control_ordinary_complete_join": "2e364d6c013e9f064c8546c77d7dbc34cbbaea989b22d275857bfe798afbc8df",
    "source_control_optimized_complete_join": "4358132d777095e1423240572244793e1f11061a32aa02454d4090cbcd577f28",
    "source_control_source_owner_backrefs": "2e463ae85b115f84a35cba3f46ee00622bd1299291bcee5cac820723d0d7828f",
    "ap02_reverse_relation_nominal_type_native": "a561337835e1935d7c106e6c61875fcffa26a95fb9406271435b6bfcb0ef06ec",
    "ap02_reverse_relation_nominal_type_milestone": "a8db987d47d226279e9ed853306552345f995ec26d2307cc51ed653425aea7ba",
}
COUNTS = {
    "direct_comparison": 305,
    "named_custom_comparison": 160,
    "outside_directory_comparison": 13,
    "paired_source_native_comparison": 35,
    "source_control_or_invariant_requires_disposition": 203,
}
SOURCE_ROLE = "source_control_or_invariant_requires_disposition"
ORDINARY_ONLY = {"parity_p0_inventory_5_0_6", "generated_api_type_errors"}


def require(condition, message):
    if not condition:
        raise RuntimeError("SQ02 registered completion: " + message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_bound(path, expected):
    require(digest(path) == expected, "immutable evidence identity: " + str(path))
    return json.loads(gzip.decompress(path.read_bytes()))


def load_evidence():
    values = {name: read_bound(ROOT / "docs/parity" / (name + "_5_0_6.json.gz"), expected)
              for name, expected in ANCHORS.items()}
    dispositions = values["source_control_complete_dispositions"]
    values["proofs"] = {name: read_bound(ROOT / name, expected)
                        for name, expected in dispositions["proofs"].items()}
    owners = values["source_control_source_owner_backrefs"]
    values["catalogs"] = {name: read_bound(ROOT / "docs/parity" / name, expected)
                          for name, expected in owners["catalog_hashes"].items()}
    ordinary = values["source_control_ordinary_complete_join"]
    values["ordinary_execution"] = read_bound(
        ROOT / ordinary["current_record"], ordinary["current_record_sha256"])
    return values


def validate(values, tests, hashes, binaries):
    inventory = values["source_control_installed_role_inventory"]
    require(inventory["complete"] is True and inventory["runtime_commit"] == RUNTIME
            and inventory["generator_commit"] == GENERATOR, "locked complete inventory")
    registered = {test["name"]: test for test in tests}
    require(len(registered) == len(tests) == inventory["registered_tests"] == 1101,
            "complete unique native registry")
    rows = inventory["rows"]
    entries = {row["test"]: row for row in rows}
    require(len(entries) == len(rows) == 716
            and Counter(row["role"] for row in rows) == COUNTS,
            "complete disjoint comparison and source/control roles")
    python_names = {test["name"] for test in tests
                    if any(value.endswith(".py") for value in test["command"])}
    require(python_names == set(entries), "unclassified Python registration")
    require(len({row["script"] for row in rows}) == 619,
            "complete Python entry source set")

    transition = nominal.evidence()[3]
    changed = {row["path"]: row for row in transition["guards"]}
    for row in rows:
        test = registered[row["test"]]
        require(test["command"] == row["command"] and test["properties"] == row["properties"],
                "exact command, selection and fixture properties: " + row["test"])
        expected = row["script_sha256"]
        if row["script"] in changed:
            admitted = changed[row["script"]]
            require(expected == admitted["before_sha256"], "historical guard epoch")
            expected = admitted["after_sha256"]
        require(hashes.get("runtime:" + row["script"]) == expected,
                "qualified entry source: " + row["test"])
    for name, expected in inventory["support_files"].items():
        require(digest(ROOT / name) == expected, "qualified import support")

    dispositions = values["source_control_complete_dispositions"]
    source_names = {row["test"] for row in rows if row["role"] == SOURCE_ROLE}
    decisions = dispositions["rows"]
    require(dispositions["complete"] is True and len(decisions) == 203
            and {row["test"] for row in decisions} == source_names,
            "every source/control responsibility has a disposition")
    for row in decisions:
        entry = entries[row["test"]]
        require(row["command"] == entry["command"] and row["properties"] == entry["properties"]
                and row["script_sha256"] == entry["script_sha256"]
                and row["ordinary_passed"] is True and bool(row["category"]) and bool(row["scope"]),
                "source/control responsibility and ordinary outcome")
    ordinary = values["source_control_ordinary_complete_join"]
    optimized = values["source_control_optimized_complete_join"]
    require(ordinary["complete"] is True and len(ordinary["all_passed"]) == 203
            and set(ordinary["all_passed"]) == source_names, "all ordinary source/control outcomes")
    require(optimized["complete"] is True and len(optimized["unique_selected"]) == 201
            and set(optimized["unique_selected"]) == source_names - ORDINARY_ONLY
            and optimized["total_actual_selections"] == 241 and optimized["fixture_overlap"] == 40,
            "complete optimized cohort with explicit P0/compiler scope")
    require(binaries == values["ordinary_execution"]["native_elf_files"] and len(binaries) == 707,
            "unchanged native consumers for retained outcomes")
    literal_name = "docs/parity/source_control_literal_role_review_5_0_6.json.gz"
    require(len(values["proofs"]) == 10 and all(row["complete"] is True
            for name, row in values["proofs"].items() if name != literal_name),
            "complete declared helper/dynamic-source proofs")
    literals = values["proofs"][literal_name]
    require(literals["literal_roles_reviewed"] is True and len(literals["literal_programs"]) == 5
            and len(literals["fragments"]) == 3 and len(literals["assertion_files"]) == 18,
            "complete literal source-role review")

    backrefs = values["source_control_source_owner_backrefs"]
    owners = backrefs["owners"]
    require(backrefs["complete"] is True and len(owners) == len({row["test"] for row in owners}) == 64,
            "complete unique primary source owners")
    consumers = []
    for owner in owners:
        entry = entries[owner["test"]]
        require(owner["test"] in source_names and owner["command"] == entry["command"]
                and owner["script_sha256"] == entry["script_sha256"], "primary source owner identity")
        setup = {prop["name"]: prop["value"] for prop in entry["properties"]}.get("FIXTURES_SETUP", [])
        require(owner["fixture_setup"] == setup, "complete source fixture setup")
        for peer in owner["comparisons"]:
            consumer = entries[peer["test"]]
            required = {prop["name"]: prop["value"] for prop in consumer["properties"]}
            require(peer["role"] == consumer["role"] != SOURCE_ROLE
                    and bool(set(setup).intersection(required.get("FIXTURES_REQUIRED", []))),
                    "comparison requires its source fixture")
            consumers.append(peer["test"])
    require(len(consumers) == len(set(consumers)) == 513
            and set(consumers) == set(entries) - source_names, "complete comparison/source ownership")

    native = values["ap02_reverse_relation_nominal_type_native"]
    affected = {test["name"] for test in native["registry"]}
    require(native["complete"] is True and len(affected) == 6
            and affected == {row["test"] for row in rows if row["script"] in changed}
            and len(native["runs"]) == 2 and {row["optimized"] for row in native["runs"]} == {False, True},
            "actual current execution of every changed entry")
    for test in native["registry"]:
        require(test["command"] == registered[test["name"]]["command"]
                and test["properties"] == registered[test["name"]]["properties"], "current native command")
    for run in native["runs"]:
        document = ET.fromstring(run["junit"])
        cases = list(document.iter("testcase"))
        require(run["exit_code"] == 0 and run["junit_accepted"] is True and len(cases) == 6
                and {case.get("name") for case in cases} == affected
                and all(case.get("status") == "run" for case in cases)
                and not any(list(document.iter(tag)) for tag in ("failure", "error", "skipped")),
                "complete actual affected JUnit outcomes")
    require(not any(isinstance(node, ast.Assert) for node in ast.walk(ast.parse(
        (ROOT / "scripts/nominal_type_source_admission_5_0_6.py").read_text()))),
        "new imported admission uses unconditional predicates")

    expected = dict(native["input_hashes_before"])
    require(native["inputs_unchanged"] is True and expected == native["input_hashes_after"],
            "unchanged native source epoch")
    milestone = values["ap02_reverse_relation_nominal_type_milestone"]
    require(milestone["complete"] is True and len(milestone["source_changes"]) == 1,
            "bounded post-native metadata update")
    for name, change in milestone["source_changes"].items():
        require(name == "runtime:docs/cpp_generator_parity_ledger.json"
                and expected[name] == change["before"], "exact metadata-only ledger epoch")
        expected[name] = change["after"]
    require("runtime:" + SELF not in expected, "new aggregate driver is not historical execution")
    expected["runtime:" + SELF] = digest(ROOT / SELF)
    require(hashes == expected, "no unqualified source/configuration changes outside this aggregate")
    return {"registered_tests": 1101, "python_commands": 716, "comparison_commands": 513,
            "source_control_commands": 203, "primary_source_owners": 64,
            "ordinary_source_control_passes": 203, "optimized_source_control_passes": 201,
            "changed_entry_passes_per_mode": 6, "native_consumers_reused": len(binaries)}


def controls(values, tests, hashes, binaries):
    mutations = (
        ("missing_registration", lambda v, t, h, b: t.pop()),
        ("wrong_command", lambda v, t, h, b: next(test for test in t if test["name"]
         == v["source_control_installed_role_inventory"]["rows"][0]["test"])["command"].append("--wrong-selection")),
        ("missing_role", lambda v, t, h, b: v["source_control_installed_role_inventory"]["rows"].pop()),
        ("missing_disposition", lambda v, t, h, b: v["source_control_complete_dispositions"]["rows"].pop()),
        ("missing_ordinary_result", lambda v, t, h, b: v["source_control_ordinary_complete_join"]["all_passed"].pop()),
        ("missing_optimized_result", lambda v, t, h, b: v["source_control_optimized_complete_join"]["unique_selected"].pop()),
        ("missing_consumer", lambda v, t, h, b: v["source_control_source_owner_backrefs"]["owners"][0]["comparisons"].pop()),
        ("incomplete_helper_proof", lambda v, t, h, b: next(iter(v["proofs"].values())).update(complete=False)),
        ("wrong_source", lambda v, t, h, b: h.update({"runtime:" + v["source_control_installed_role_inventory"]["rows"][0]["script"]: "0" * 64})),
        ("wrong_native_consumer", lambda v, t, h, b: b.update({next(iter(b)): "0" * 64})),
        ("failed_current_execution", lambda v, t, h, b: v["ap02_reverse_relation_nominal_type_native"]["runs"][0].update(exit_code=1)),
        ("unknown_new_source", lambda v, t, h, b: h.update({"runtime:scripts/unqualified.py": "0" * 64})),
    )
    results = []
    for name, mutate in mutations:
        args = tuple(copy.deepcopy(value) for value in (values, tests, hashes, binaries))
        mutate(*args)
        try:
            validate(*args)
        except RuntimeError as error:
            results.append({"case": name, "rejected": True, "error": str(error)})
        else:
            raise RuntimeError("SQ02 corruption accepted: " + name)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    args = parser.parse_args()
    require(not args.record.exists(), "preserve historical evidence")
    values = load_evidence()
    inputs = capture_inputs()
    refs = references()
    require(refs == {"teaql-rs": RUNTIME, "teaql_code_gen": GENERATOR}, "locked references")
    tests = json.loads(subprocess.check_output(
        ["ctest", "--preset", "native", "--show-only=json-v1"], cwd=ROOT, text=True))["tests"]
    binaries = native_hashes()
    summary = validate(values, tests, inputs, binaries)
    source_controls = controls(values, tests, inputs, binaries)
    direct_state = direct.collect(tests)
    direct_rows = direct.validate(direct_state)
    require(digest(ROOT / direct.ADAPTER) == direct.ADAPTER_SHA256,
            "shared value framing, type and complete-cardinality comparator")
    direct_controls = direct.controls(direct_state, direct_rows)
    require(digest(custom.MANIFEST) == custom.MANIFEST_SHA256, "locked custom source manifest")
    manifest = custom.read(custom.MANIFEST)
    custom_rows = custom.validate(manifest, tests, ROOT / "build_native_current")
    custom_controls = custom.controls(manifest, tests, ROOT / "build_native_current")
    paired_rows = paired.validate(manifest, tests, ROOT / "build_native_current")
    paired_controls = paired.controls(manifest, tests, ROOT / "build_native_current", paired_rows)
    require(len(direct_rows) == 305 and len(custom_rows) == 160 and len(paired_rows) == 35,
            "current source/corpus/member bindings")
    require(capture_inputs() == inputs and references() == refs, "stable qualification inputs")
    record = {"complete": True, "complete_sq02": True, "whole_goal_complete": False,
              "scope": "Complete registered SQ02 provenance and acceptance-boundary reconciliation at this exact epoch; G14 semantics, G17 and P0-P7 remain independent",
              "runtime_commit": RUNTIME, "generator_commit": GENERATOR,
              "optimized": sys.flags.optimize, "summary": summary,
              "anchors": ANCHORS, "input_hashes": inputs, "references": refs,
              "native_elf_files": binaries, "controls": source_controls,
              "binding_controls": {"direct": direct_controls, "custom": custom_controls,
                                   "paired": paired_controls},
              "execution_reuse": "203 ordinary / 201 optimized registrations and six current guards per mode; full 201 compiler cases are ordinary-only, with two optimized compiler controls; no new full native execution",
              "owned_bytes_removed": 0, "owned_directory_removed": True}
    args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print("SQ02 reconciled: 513 comparisons, 203 source/control responsibilities, 64 primary owners; "
          "12 aggregate corruptions and current direct/custom/paired binding controls reject")


if __name__ == "__main__":
    main()
