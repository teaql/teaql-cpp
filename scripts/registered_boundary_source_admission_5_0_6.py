#!/usr/bin/env python3
"""Admit one already qualified dependency epoch for the registered boundary gate."""

import argparse
import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SELF = "runtime:scripts/registered_boundary_source_admission_5_0_6.py"
ENTRY = "runtime:tests/parity/verify_registered_boundary_hardened_5_0_6.py"
RELATION = "scripts/relation_analysis_cli_source_binding_5_0_6.py"
OLD = "c7f4961e7ef916503b9f29ecef8d37d0c2934a76b22681640f885d19f141217c"
NEW = "d7d4382057bcd572e3d4fa3b9b9ee13f9cbb9c812d9860911f12416a176afe86"
BASE = ROOT / "docs/parity/current_full_native_after_keyword_inputs_5_0_6.json.gz"
BASE_SHA = "b72551020e21f447a4664903e6da0706b66bdf3c7f11464bd33b7c46c593bd8d"
QUALIFIED = ROOT / "docs/parity/approved_generation_decisions_sources_summary_repaired_5_0_6.json.gz"
QUALIFIED_SHA = "cb611f10f18022bb98fc1715b45185b823cf8cde60d16e91117a0c682d21e92e"
RECORD = ROOT / "docs/parity/registered_boundary_dependency_sources_with_path_5_0_6.json.gz"
RECORD_SHA = "b29b9b3a064d4d72697ca59047fc6287ee6bc1f819bd409c1a84040b2ff52871"
BEFORE = ('    for path, wanted in manifest["dependencies"].items():\n'
          '        require(digest((ROOT / path).read_bytes()) == wanted, "Local imported dependency changed")\n')
AFTER = ('    sys.path.insert(0, str(ROOT / "scripts"))\n'
         '    from registered_boundary_source_admission_5_0_6 import dependency_matches\n'
         '    for path, wanted in manifest["dependencies"].items():\n'
         '        require(dependency_matches(path, wanted), "Local imported dependency changed")\n')
RELATION_EDITS = (
    ('    def validate_files(self):\n',
     '    def validate_files(self):\n'
     '        from generation_decision_source_admission_5_0_6 import current_input_matches as decision_matches\n'),
    ("            require(digest(ROOT / path) == wanted, 'current source support: ' + path)\n",
     "            require(digest(ROOT / path) == wanted or decision_matches(ROOT / path, wanted),\n"
     "                    'current source support: ' + path)\n"),
    ("            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],\n",
     "            require(digest(ROOT / contract[key]) == contract[key + '_sha256']\n"
     "                    or key == 'exception_manifest' and decision_matches(\n"
     "                        ROOT / contract[key], contract[key + '_sha256']),\n"),
)


def require(condition, message):
    if not condition:
        raise RuntimeError("Registered boundary source admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


@cache
def relation_transition():
    require(sha(QUALIFIED.read_bytes()) == QUALIFIED_SHA, "immutable prior qualified source transition")
    qualified = read(QUALIFIED)
    require(qualified["complete"] is True and qualified["whole_goal_complete"] is False,
            "prior source qualification keeps its original scope")
    row = qualified["paths"]["runtime:" + RELATION]
    require(row["before_sha256"] == OLD == sha(row["before_source"].encode())
            and row["after_sha256"] == NEW == sha(row["after_source"].encode()),
            "one exact previously qualified helper epoch")
    reconstructed = row["before_source"]
    for before, after in RELATION_EDITS:
        require(reconstructed.count(before) == 1, "one exact prior identity-guard edit")
        reconstructed = reconstructed.replace(before, after, 1)
    require(reconstructed == row["after_source"]
            and (ROOT / RELATION).read_text() == row["after_source"],
            "only qualified source identity guards changed; current helper remains exact")
    return row


def validate(record):
    require(record.get("schema") == "teaql.registered_boundary.dependency_sources.v1"
            and record.get("complete") is True and record.get("whole_goal_complete") is False
            and record.get("native_acceptance") is False
            and record.get("fresh_business_execution") is False,
            "bounded source admission cannot claim native or whole-goal completion")
    require(sha(BASE.read_bytes()) == BASE_SHA and record["base_sha256"] == BASE_SHA
            and record["qualified_transition_sha256"] == QUALIFIED_SHA,
            "exact frozen source frame and prior qualification")
    relation_transition()
    require(set(record["paths"]) == {ENTRY}, "one existing entry identity guard changes")
    row = record["paths"][ENTRY]
    require(row["before_sha256"] == sha(row["before_source"].encode())
            == read(BASE)["hashes"][ENTRY]
            and SELF not in read(BASE)["hashes"]
            and row["after_sha256"] == sha(row["after_source"].encode()),
            "complete entry preimage and afterimage")
    require(row["before_source"].count(BEFORE) == 1
            and row["before_source"].replace(BEFORE, AFTER, 1) == row["after_source"]
            and (ROOT / ENTRY.split(":", 1)[1]).read_text() == row["after_source"],
            "only the imported dependency predicate forwards; all original checks remain exact")
    binding = 'RECORD_SHA = "' + RECORD_SHA + '"\n'
    current = Path(__file__).read_text()
    require(current.count(binding) == 1
            and current.replace(binding, 'RECORD_SHA = "__BOUNDARY_SOURCE_SHA__"\n', 1)
            == record["helper_after_source_normalized"], "complete bounded admission helper")


@cache
def sources():
    require(sha(RECORD.read_bytes()) == RECORD_SHA, "immutable bounded dependency source repair")
    record = read(RECORD)
    validate(record)
    return record["paths"]


def source_rows():
    return {**sources(), SELF: dict(before_source=None, before_sha256=None,
        after_source=Path(__file__).read_text(), after_sha256=sha(Path(__file__).read_bytes()))}


def added_input_hashes():
    sources()
    return {SELF: sha(Path(__file__).read_bytes())}


def dependency_matches(relative, expected):
    if (type(relative) is not str or type(expected) is not str
            or not re.fullmatch(r"[0-9a-f]{64}", expected)):
        return False
    path = Path(relative)
    if path.is_absolute() or ".." in path.parts or not (ROOT / path).is_file():
        return False
    sources()
    actual = sha((ROOT / path).read_bytes())
    if actual == expected:
        return True
    return relative == RELATION and expected == OLD and actual == NEW


def controls(record):
    changes = (
        ("numeric_complete", lambda r: r.update(complete=1)),
        ("claimed_native_acceptance", lambda r: r.update(native_acceptance=True)),
        ("claimed_business_execution", lambda r: r.update(fresh_business_execution=True)),
        ("claimed_whole_goal", lambda r: r.update(whole_goal_complete=True)),
        ("wrong_schema", lambda r: r.update(schema="unknown")),
        ("wrong_epoch", lambda r: r.update(base_sha256="0" * 64)),
        ("unqualified_transition", lambda r: r.update(qualified_transition_sha256="0" * 64)),
        ("missing_entry", lambda r: r["paths"].pop(ENTRY)),
        ("extra_entry", lambda r: r["paths"].update({"runtime:unqualified": {}})),
        ("changed_preimage", lambda r: r["paths"][ENTRY].update(before_sha256="0" * 64)),
        ("changed_afterimage", lambda r: r["paths"][ENTRY].update(after_sha256="0" * 64)),
        ("weakened_entry", lambda r: r["paths"][ENTRY].update(after_source="pass\n")),
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
            raise RuntimeError("Corrupted dependency transition accepted: " + name)
    for name, path, expected in (
        ("unknown_hash", RELATION, "0" * 64),
        ("nonstring_hash", RELATION, 1),
        ("absolute_path", str(ROOT / RELATION), OLD),
        ("parent_traversal", "../" + RELATION, OLD),
        ("unknown_dependency", "scripts/unknown_source.py", OLD),
    ):
        require(dependency_matches(path, expected) is False, "unqualified imported dependency rejects")
        rejected.append(name)
    require(dependency_matches(RELATION, OLD) is True
            and dependency_matches(RELATION, NEW) is True,
            "qualified earlier and current dependency identities accept")
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "preserve earlier evidence")
    record = read(RECORD)
    sources()
    rejected = controls(record)
    result = dict(complete=True, whole_goal_complete=False, native_acceptance=False,
        optimized=__import__("sys").flags.optimize > 0, controls=rejected,
        qualified_dependency=RELATION, before=OLD, after=NEW)
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(), mtime=0))
    print("One qualified dependency epoch; " + str(len(rejected)) + " corruptions rejected")


if __name__ == "__main__":
    main()
