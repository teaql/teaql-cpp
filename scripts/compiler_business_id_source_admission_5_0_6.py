#!/usr/bin/env python3
"""Admit strong Business ID mapping through exact executed source transitions."""

import argparse
import ast
import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
RECORDS = {
    "runtime": ("compiler_business_id_storage_execution_5_0_6.json.gz", "f5b599143ac30e993b6a94a3d8bb185256c56b35213a93b2844a8acca3448ac6"),
    "admission": ("compiler_business_id_storage_admission_5_0_6.json.gz", "c07ea8781e159ed56167099afb83ddfb550d9fa667ac63729b1a119d6a59bf98"),
    "sdk": ("compiler_business_id_storage_sdks_5_0_6.json.gz", "f56c40f4312d0ffebfff689b3dbfa7c54d63b507371212ed0a34b9e292323dfd"),
    "java": ("compiler_business_id_storage_units_java_5_0_6.json.gz", "eb74c7ce1e253e2b788110034c4fe8e68608636aa6d01bc865b97d0945340ed9"),
    "worker": ("compiler_business_id_storage_worker_units_5_0_6.json.gz", "124c4f1b0d0f37d9f003047be041f54c0a57eeb0da819e42cf628d17ec794102"),
    "source": ("compiler_business_id_storage_source_transition_5_0_6.json.gz", "a72e6ef0a2c859aee6584cc96280ada5dca8d94a0748f3441b78bd2d763470a0"),
    "readers": ("compiler_business_id_storage_reader_transition_5_0_6.json.gz", "71795b62631493db67b099760a207a01e4f8f17746297ff6d3c2b97405024dd7"),
}
READER_CHANGES = {
    "compiler_scalar_source_admission_5_0_6.py": ["registered_epoch_matches", "current_input_matches", "frame_matches", "java_inventory_matches"],
    "verify_compiler_business_id_storage_5_0_6.py": ["validate"],
    "verify_compiler_json_consumers_5_0_6.py": ["validate"],
    "verify_compiler_expression_consumers_5_0_6.py": ["validate"],
    "reproduce_evaluated_generator_sdks_5_0_6.py": ["validate"],
}


def require(value, message):
    if not value:
        raise RuntimeError("Business ID source admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable executed receipt: " + name)
    return json.loads(gzip.decompress(data))


def source_path(name):
    repository, relative = name.split(":", 1)
    path = Path(relative)
    require(repository in ("runtime", "producer") and not path.is_absolute()
            and ".." not in path.parts, "repository-relative source identity")
    return (ROOT if repository == "runtime" else PRODUCER) / path


@cache
def source_epochs():
    from compiler_scalar_source_admission_5_0_6 import source_epochs as preceding_epochs
    result = {path: list(rows) for path, rows in preceding_epochs().items()}
    for key in ("source", "readers"):
        for name, row in bound(key)["paths"].items():
            result.setdefault(source_path(name), []).append(row)
    row = bound("admission")["source_transition"]
    result.setdefault(ROOT / "scripts/verify_compiler_business_id_storage_5_0_6.py", []).append(row)
    for before, after in zip(bound("sdk")["before"], bound("sdk")["after"], strict=True):
        name = Path(before["manifest"]).name
        result.setdefault(ROOT / before["manifest"], []).append(dict(
            before_sha256=before["files"][name], after_sha256=after["files"][name]))
    return result


def registered_epoch_matches(path, expected):
    path = Path(path)
    if type(expected) is not str or re.fullmatch(r"[0-9a-f]{64}", expected) is None or not path.is_file():
        return False
    observed = sha(path.read_bytes())
    pending, visited = [expected], set()
    while pending:
        candidate = pending.pop()
        if candidate == observed:
            return True
        if candidate in visited:
            continue
        visited.add(candidate)
        pending.extend(row["after_sha256"] for row in source_epochs().get(path, [])
                       if row["before_sha256"] == candidate)
    return False


def current_input_matches(path, expected):
    path = Path(path)
    if not registered_epoch_matches(path, expected):
        return False
    if sha(path.read_bytes()) != expected:
        evidence()
    return True


def frame_matches(current, historical):
    return all(name in current and (current[name] == value
               or registered_epoch_matches(source_path(name), value))
               for name, value in historical.items())


def java_inventory_matches(current, historical):
    unknown = set(current) - set(historical)
    additions = {str(path): row["after_sha256"] for path, rows in source_epochs().items() for row in rows
                 if path.is_relative_to(PRODUCER / "generator/src/main/java")
                 and row.get("before_source", "existing") is None}
    if not unknown <= additions.keys():
        return False
    if unknown:
        evidence()
    return all(Path(name).is_file() and registered_epoch_matches(Path(name), additions[name]) for name in unknown)


def functions(source):
    return {node.name: ast.dump(node, include_attributes=False) for node in ast.parse(source).body
            if isinstance(node, (ast.FunctionDef, ast.ClassDef))}


def validate_sources(records):
    source, readers = records["source"], records["readers"]
    require(source["complete"] is True and source["whole_goal_complete"] is False
            and len(source["paths"]) == 7, "five producer inputs and two actual execution drivers")
    for name, row in source["paths"].items():
        require(row["after_sha256"] == sha(row["after_source"].encode())
                and registered_epoch_matches(source_path(name), row["after_sha256"]), "exact executed source image: " + name)
        require(row["before_source"] is None and row["before_sha256"] is None
                or row["before_source"] is not None and row["before_sha256"] == sha(row["before_source"].encode()), "exact source preimage")
        frame = records["sdk"]["input_hashes_after"] if name.endswith("reproduce_compiler_business_id_sdks_5_0_6.py") else records["runtime"]["inputs_after"]
        require(frame.get(name) == row["after_sha256"], "source belongs to actual successful compiler/SDK commands")
    worker = source["paths"]["producer:scripts/cpp_rust_compiler_job.py"]
    before, after = functions(worker["before_source"]), functions(worker["after_source"])
    require([name for name in before if before[name] != after.get(name)]
            == ["load_capabilities", "validate_inventory", "generate"]
            and set(after) - set(before) == {"semantic_storage_source", "validate_semantic_storage"},
            "only explicit semantic capability branches change the worker; prior scalar/control functions remain exact")
    require(readers["complete"] is True and readers["whole_goal_complete"] is False
            and set(readers["paths"]) == {"runtime:scripts/" + name for name in READER_CHANGES}, "five bounded source/version reader transitions")
    for name, row in readers["paths"].items():
        require(row["before_sha256"] == sha(row["before_source"].encode())
                and row["after_sha256"] == sha(row["after_source"].encode())
                and registered_epoch_matches(source_path(name), row["after_sha256"]), "exact reader source images")
        before, after = functions(row["before_source"]), functions(row["after_source"])
        require([key for key in before if before[key] != after.get(key)] == READER_CHANGES[Path(name).name],
                "only named source/provenance admission functions change")
        additions = {"manifest_value_matches"} if name.endswith("reproduce_evaluated_generator_sdks_5_0_6.py") else set()
        require(set(after) - set(before) == additions and not set(before) - set(after), "execution and control inventories remain exact")
    admission = records["admission"]
    row = admission["source_transition"]
    before, after = functions(row["before_source"]), functions(row["after_source"])
    require(row["before_source"] == records["runtime"]["driver_source"]
            and row["after_source"] == readers["paths"]["runtime:scripts/verify_compiler_business_id_storage_5_0_6.py"]["before_source"]
            and row["before_sha256"] == sha(row["before_source"].encode())
            and row["after_sha256"] == sha(row["after_source"].encode())
            and [name for name in before if before[name] != after.get(name)] == ["validate"]
            and set(after) - set(before) == {"execution_source_matches"}, "exact earlier envelope-reader repair with unchanged business execution")


def validate(records):
    from verify_compiler_business_id_storage_5_0_6 import validate as validate_runtime
    import reproduce_compiler_business_id_sdks_5_0_6 as sdk
    validate_sources(records)
    validate_runtime(records["runtime"])
    engine = sdk.preceding
    names = ("__file__", "BASE_COMMIT", "BASE", "BASE_SHA", "load_recipe", "expected_manifest")
    saved = {name: getattr(engine, name) for name in names}
    try:
        sdk.validate(records["sdk"])
    finally:
        for name, value in saved.items():
            setattr(engine, name, value)
    admission = records["admission"]
    require(admission["complete"] is True and admission["whole_goal_complete"] is False
            and admission["execution_sha256"] == RECORDS["runtime"][1]
            and admission["original_gate_exit_code"] == 1
            and [step["optimized"] for step in admission["steps"]] == [False, True]
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0
                    and step["stdout"].startswith("BASELINE_ACCEPTED\n")
                    and step["stdout"].rstrip().endswith("REJECTED 33")
                    and len([line for line in step["stdout"].splitlines() if line.startswith("{")]) == 33
                    for step in admission["steps"]), "actual ordinary/optimized positive and 33 corruption results; original exit status retained")
    java = records["java"]
    require(java["complete"] is True and java["whole_goal_complete"] is False
            and [step["label"] for step in java["steps"]] == ["current_cpp_javac", "current_cpp_junit"]
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0 for step in java["steps"])
            and re.search(r"OK \(265 tests\)", java["steps"][1]["stdout"]), "fresh Java compilation and all 265 generator contracts")
    prepared = {name for part in java["steps"] for name in part["command"] if name.endswith(".java")}
    require(set(java["prepared_java_sources"]) == prepared
            and all(registered_epoch_matches(Path(name), value) for name, value in java["prepared_java_sources"].items()), "complete actually compiled Java input identities")
    worker = records["worker"]
    # This receipt also retains the wrong-cwd Java failure; only its successful worker steps enter this claim.
    require(worker["complete"] is True and worker["whole_goal_complete"] is False
            and worker["inputs_before"] == worker["inputs_after"], "standalone worker contracts and stable unit inputs")
    require(all(registered_epoch_matches(PRODUCER / name, value) for name, value in worker["inputs_after"].items()), "same worker/Java source state")
    steps = worker["steps"]
    require([step["optimized"] for step in steps] == [False, True]
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0
                    and re.search(r"Ran 18 tests\b", step["stderr"])
                    and step["stderr"].rstrip().endswith("OK") for step in steps), "all 18 ordinary and optimized worker contracts")
    for record in (java, admission):
        require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(), "owned unit/admission outputs removed")


@cache
def evidence():
    records = {key: bound(key) for key in RECORDS}
    validate(records)
    return records


def controls():
    records = evidence()
    mutations = (
        ("runtime", "incomplete_runtime", lambda r: r.update(complete=False)),
        ("runtime", "foreign_wrapper", lambda r: next(p for e in r["models"][0]["job"]["scalar_types"] for p in e["properties"] if p["semantic_kind"] == "business_id").update(semantic_type="ForeignNumber")),
        ("sdk", "missing_sdk", lambda r: r["after"].pop()),
        ("sdk", "changed_generated_body", lambda r: r["after"][0]["files"].update(invented="0" * 64)),
        ("java", "wrong_java_count", lambda r: r["steps"][1].update(stdout="OK (262 tests)\n")),
        ("worker", "lost_optimized_worker", lambda r: r["steps"].pop()),
        ("source", "unexecuted_source", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("readers", "changed_execution_body", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("admission", "lost_optimized_admission", lambda r: r["steps"].pop()),
        ("admission", "relabeled_original_exit", lambda r: r.update(original_gate_exit_code=0)),
    )
    result = []
    for key, name, mutate in mutations:
        changed = dict(records)
        changed[key] = copy.deepcopy(records[key])
        mutate(changed[key])
        try:
            validate(changed)
        except (RuntimeError, KeyError, IndexError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted Business ID source corruption: " + name)
    require(not java_inventory_matches({"/unknown/Unqualified.java"}, set()), "unknown Java source rejected")
    result.append(dict(case="unknown_java_source", rejected=True))
    require(not registered_epoch_matches(PRODUCER / "scripts/cpp_rust_compiler_job.py", "0" * 64)
            and not registered_epoch_matches(PRODUCER / "scripts/cpp_rust_compiler_job.py", None), "unknown and missing source identities rejected")
    result.append(dict(case="unknown_source_image", rejected=True))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    evidence()
    if args.controls:
        require(not args.controls.exists(), "preserve earlier source admission controls")
        result = dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize, controls=controls(),
                      receipts={key: value[1] for key, value in RECORDS.items()})
        args.controls.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(), mtime=0))
        print(json.dumps(dict(complete=True, optimized=sys.flags.optimize, controls=len(result["controls"]))))
    else:
        print("Verified exact strong Business ID sources, SDKs, execution and contracts")


if __name__ == "__main__":
    main()
