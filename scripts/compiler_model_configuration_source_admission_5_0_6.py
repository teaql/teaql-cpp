#!/usr/bin/env python3
"""Admit compiled metadata only through exact source/SDK transitions and executions."""

import argparse
import ast
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
RECORDS = {
    "generation": ("compiler_model_configuration_full_5_0_6.json.gz", "3d7a47edf73681f2c6e9700c5df0067fff8750eddcd94186053e3ca22350cef7"),
    "preserved": ("compiler_model_configuration_preserved_contracts_5_0_6.json.gz", "dcc4d3eac379d5ad6ef7d39f420c46428b15465c5573ef146aa1dabd5e018b03"),
    "sdk": ("compiler_model_configuration_sdks_5_0_6.json.gz", "161f766f664807c11618e284cd2eb72bebd47c5bb66706f1e0c71dc2c96be469"),
    "source": ("compiler_model_configuration_transition_5_0_6.json.gz", "96f9b65d30e21ff94ff9f90988e8d4c4918784307f74ad5f201d60fae403fbef"),
    "predicates": ("compiler_model_configuration_predicate_transition_5_0_6.json.gz", "5150cfcc2b40ac79d79b52ba6ab8aa5283accd7dd192aa2d17102dd8df39e6b6"),
    "inventory": ("compiler_model_configuration_inventory_transition_5_0_6.json.gz", "8c1a34f235ee2fa48f4f4ea60d5acae50e4bb5dc7cff308d1f0e2cf1c41f1a1d"),
    "generation_controls": ("compiler_model_configuration_controls_5_0_6.json.gz", "66e014938c34c82eac069f795ddb1903890f3749ce6a247c264c21032da353c5"),
    "generation_controls_optimized": ("compiler_model_configuration_controls_optimized_5_0_6.json.gz", "3d28de6e877ebac15a3c04cc07cb224ebdd282959422b30e01e70f0a44db2f3f"),
    "preserved_controls": ("compiler_model_configuration_preserved_controls_5_0_6.json.gz", "00e0e63e2d8cac5ea78908baaf18aa1a4717cedbecd75efcdfe112d023cdc0bd"),
    "preserved_controls_optimized": ("compiler_model_configuration_preserved_controls_optimized_5_0_6.json.gz", "1d5f4a9e59d9803ebf58b012ef1f738dc3f98d1982a7e01ec5a5f3684eb2c9ca"),
    "sdk_controls": ("compiler_model_configuration_sdk_controls_5_0_6.json.gz", "601445f02b5f6eb02bf8d19645297d1aa853a3d3a48cee300f0edfbbe76d52e5"),
    "sdk_controls_optimized": ("compiler_model_configuration_sdk_controls_optimized_5_0_6.json.gz", "0385c9aef463067bd4a7cf27f07e78ec8206adc63a31b3fc540dda541b0241d0"),
}


def require(value, message):
    if not value:
        raise RuntimeError("Compiled metadata source admission: " + message)


def sha(value):
    return hashlib.sha256(value).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable receipt: " + name)
    return json.loads(gzip.decompress(data))


def source_path(name):
    repository, relative = name.split(":", 1)
    require(repository in ("runtime", "producer"), "known source repository")
    return (ROOT if repository == "runtime" else PRODUCER) / relative


def validate(records):
    from verify_compiler_model_configuration_5_0_6 import validate as validate_generation
    from preserve_compiler_model_contracts_5_0_6 import validate as validate_preserved
    from reproduce_compiler_model_sdks_5_0_6 import validate as validate_sdks

    validate_generation(records["generation"])
    validate_preserved(records["preserved"])
    validate_sdks(records["sdk"])
    for kind, count in (("generation", 19), ("preserved", 8), ("sdk", 8)):
        for mode in (0, 1):
            controls = records[kind + "_controls" + ("_optimized" if mode else "")]
            require(controls["complete"] is True and type(controls["optimized"]) is int and controls["optimized"] == mode
                    and controls["execution_sha256"] == RECORDS[kind][1] and len(controls["controls"]) == count
                    and all(row["rejected"] is True for row in controls["controls"]), "complete ordinary/optimized execution controls")
    source = records["source"]
    require(source["complete"] is True and len(source["paths"]) == 14, "fourteen exact implementation/recipe paths")
    for name, row in source["paths"].items():
        require(registered_epoch_matches(source_path(name), row["after_sha256"])
                and row["after_sha256"] == sha(row["after_source"].encode()), "actual qualified source bytes")
        if row["before_source"] is not None:
            require(sha(row["before_source"].encode()) == row["before_sha256"], "exact source preimage")
        if name.startswith("producer:"):
            require(records["generation"]["input_hashes_after"][name] == row["after_sha256"], "actual complete compiler execution source")
        elif name.endswith("verify_compiler_model_configuration_5_0_6.py") or name.endswith("compiler_model_configuration_probe.cpp"):
            require(records["generation"]["input_hashes_after"][name] == row["after_sha256"], "actual native qualification source")
        elif name.endswith("preserve_compiler_model_contracts_5_0_6.py"):
            require(records["preserved"]["input_hashes_after"][name] == row["after_sha256"], "actual 406-SDK controller source")
        else:
            require(records["sdk"]["input_hashes_after"][name] == row["after_sha256"], "actual retained SDK reproduction source")
    predicates = records["predicates"]
    require(predicates["complete"] is True and set(predicates["paths"]) == {
        "scripts/verify_compiler_source_service_5_0_6.py", "scripts/verify_cpp_compiler_job_5_0_6.py",
        "scripts/compiler_projection_source_admission_5_0_6.py", "scripts/compiler_source_service_source_admission_5_0_6.py",
        "scripts/reproduce_compiler_projection_sdks_5_0_6.py"}, "five exact historical reader boundaries")
    for name, row in predicates["paths"].items():
        after = row["before_source"]
        for edit in row["edits"]:
            require(after.count(edit["before"]) == 1, "one exact source-version predicate/import edit")
            after = after.replace(edit["before"], edit["after"], 1)
        require(after == row["after_source"] and sha(after.encode()) == row["after_sha256"]
                and sha(row["before_source"].encode()) == row["before_sha256"]
                and registered_epoch_matches(ROOT / name, row["after_sha256"]), "all other historical data/control bodies preserved")
    inventory = records["inventory"]
    name = "scripts/compiler_projection_source_admission_5_0_6.py"
    require(inventory["complete"] is True and set(inventory["paths"]) == {name}, "one exact final inventory source-version guard")
    row = inventory["paths"][name]
    after = row["before_source"]
    for edit in row["edits"]:
        require(after.count(edit["before"]) == 1, "one exact inventory predicate edit")
        after = after.replace(edit["before"], edit["after"], 1)
    require(after == row["after_source"] and sha(after.encode()) == row["after_sha256"]
            and sha(row["before_source"].encode()) == row["before_sha256"]
            and predicates["paths"][name]["after_sha256"] == row["before_sha256"]
            and (ROOT / name).read_text() == after, "qualified inventory continuation preserves every other reader body")
    validate_legacy_worker(source)


def validate_legacy_worker(source):
    """The new metadata version adds branches; every v1 worker operation remains."""
    row = source["paths"]["producer:scripts/cpp_rust_compiler_job.py"]
    before, after = ast.parse(row["before_source"]), ast.parse(row["after_source"])
    after.body = [node for node in after.body if not (
        isinstance(node, ast.FunctionDef) and node.name in ("configuration_source", "validate_configuration")
        or isinstance(node, ast.Assign) and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name)
        and node.targets[0].id in ("MODEL_CAPABILITIES", "MODEL_CONFIGURATION"))]
    old_load = next(node for node in before.body if isinstance(node, ast.FunctionDef) and node.name == "load_capabilities")
    new_load = next(node for node in after.body if isinstance(node, ast.FunctionDef) and node.name == "load_capabilities")
    old_guard = next(node for node in ast.walk(old_load) if isinstance(node, ast.Compare)
                     and ast.unparse(node.left) == "config['schema']")
    replaced = 0
    for node in ast.walk(new_load):
        if (isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id == "require"
                and isinstance(node.args[0], ast.Compare) and ast.unparse(node.args[0].left) == "config['schema']"):
            node.args[0] = copy.deepcopy(old_guard)
            replaced += 1
    generate = next(node for node in after.body if isinstance(node, ast.FunctionDef) and node.name == "generate")

    class RemoveMetadata(ast.NodeTransformer):
        def __init__(self):
            self.removed = 0

        def visit_If(self, node):
            if ast.unparse(node.test) == "config['schema'] == MODEL_CAPABILITIES":
                self.removed += 1
                return None
            return self.generic_visit(node)

    removed = RemoveMetadata()
    removed.visit(generate)
    require(replaced == removed.removed == 1 and ast.dump(after, include_attributes=False)
            == ast.dump(before, include_attributes=False), "all existing v1 worker bodies remain unchanged")


@cache
def evidence():
    records = {key: bound(key) for key in RECORDS}
    validate(records)
    return records


@cache
def source_epochs():
    from compiler_source_service_source_admission_5_0_6 import source_epochs as preceding_epochs

    result = {path: list(rows) for path, rows in preceding_epochs().items()}
    for name, row in bound("source")["paths"].items():
        result.setdefault(source_path(name), []).append(row)
    for key in ("predicates", "inventory"):
        for name, row in bound(key)["paths"].items():
            result.setdefault(ROOT / name, []).append(row)
    sdk = bound("sdk")
    for old, new in zip(sdk["before"], sdk["after"], strict=True):
        name = Path(old["manifest"]).name
        result.setdefault(ROOT / old["manifest"], []).append(dict(
            before_sha256=old["files"][name], after_sha256=new["files"][name]))
    return result


def registered_epoch_matches(path, expected):
    from generator_evaluation_source_admission_5_0_6 import registered_epoch_matches as evaluation_epoch_matches
    return evaluation_epoch_matches(path, expected)


def preceding_registered_epoch_matches(path, expected):
    if not path.is_file():
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
        pending.extend(row["after_sha256"] for row in source_epochs().get(path, []) if row["before_sha256"] == candidate)
    return False


def current_input_matches(path, expected):
    from generator_evaluation_source_admission_5_0_6 import current_input_matches as evaluation_input_matches
    return evaluation_input_matches(path, expected)


def preceding_current_input_matches(path, expected):
    if not path.is_file():
        return False
    if sha(path.read_bytes()) == expected:
        return True
    if path not in source_epochs() or not registered_epoch_matches(path, expected):
        return False
    evidence()
    return True


def java_inventory_matches(current, historical):
    from generator_evaluation_source_admission_5_0_6 import java_inventory_matches as evaluation_inventory_matches
    return evaluation_inventory_matches(current, historical)


def preceding_java_inventory_matches(current, historical):
    unknown = set(current) - set(historical)
    additions = {str(path): row["after_sha256"] for path, rows in source_epochs().items() for row in rows
                 if path.is_relative_to(PRODUCER / "generator/src/main/java") and row.get("before_source", "existing") is None}
    if not unknown <= additions.keys():
        return False
    if unknown:
        evidence()
    return all(Path(name).is_file() and sha(Path(name).read_bytes()) == additions[name] for name in unknown)


def controls():
    records = evidence()
    mutations = (
        ("generation", "incomplete_execution", lambda r: r.update(complete=False)),
        ("generation", "lost_native_execution", lambda r: r["steps"].pop()),
        ("preserved", "lost_default_sdk", lambda r: r["cases"].pop()),
        ("sdk", "lost_retained_sdk", lambda r: r["after"].pop()),
        ("source", "unqualified_source", lambda r: next(iter(r["paths"].values())).update(after_source="invalid")),
        ("predicates", "lost_historical_body", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("inventory", "lost_inventory_guard_body", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("generation_controls_optimized", "lost_optimized_control", lambda r: r["controls"].pop()),
    )
    rejected = []
    for key, name, mutate in mutations:
        changed = dict(records)
        changed[key] = copy.deepcopy(records[key])
        mutate(changed[key])
        try:
            validate(changed)
        except (RuntimeError, KeyError, StopIteration) as error:
            rejected.append(dict(case=name, rejected=True, error=repr(error)))
        else:
            raise RuntimeError("Compiled model source corruption accepted: " + name)
    for path, rows in source_epochs().items():
        for row in rows:
            require(current_input_matches(path, row["after_sha256"]) and not current_input_matches(path, "0" * 64), "qualified current/prior epoch and unknown rejection")
            if row["before_sha256"] is not None:
                require(current_input_matches(path, row["before_sha256"]), "qualified predecessor source epoch")
            rejected.append(dict(case="source_epoch:" + str(path), after_sha256=row["after_sha256"], rejected=True))
    require(not java_inventory_matches({"/unknown/Unqualified.java"}, set()), "unknown Java source addition rejects")
    rejected.append(dict(case="unknown_java_addition", rejected=True))
    from compiler_projection_source_admission_5_0_6 import java_inventory_matches as legacy_inventory_matches
    current = {str(path) for repository in (ROOT / ".reference/teaql_code_gen", PRODUCER)
               for package in ("model", "generator") for path in (repository / package / "src/main/java").rglob("*.java")}
    additions = {str(PRODUCER / "generator/src/main/java/com/skynet/codegenerator/cpp" / (name + ".java")) for name in (
        "CppProjectionResolver", "CppCompilerProjectionResolver", "CppCompilerGeneration",
        "CppModelConfiguration", "CppCompilerModelConfiguration")}
    require(additions <= current and legacy_inventory_matches(current, current - additions)
            and not legacy_inventory_matches(current | {"/unknown/Unqualified.java"}, current - additions),
            "complete actual legacy Java inventory accepts all qualified versions and rejects unknown additions")
    rejected.append(dict(case="complete_legacy_java_inventory", rejected=True))
    return dict(complete=True, optimized=sys.flags.optimize, controls=rejected, current_helper_sha256=sha(Path(__file__).read_bytes()), valid_record_accepted=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    require(not args.output.exists(), "preserve previous source controls")
    result = controls()
    args.output.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(), mtime=0))
    print(json.dumps(dict(complete=True, optimized=result["optimized"], controls=len(result["controls"]))))


if __name__ == "__main__":
    main()
