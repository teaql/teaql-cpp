#!/usr/bin/env python3
"""Admit exact scalar consumer sources while preserving preceding execution."""

import argparse
import ast
import copy
from functools import cache
from pathlib import Path
import sys

from verify_compiler_generated_business_5_0_6 import read, require, sha, write

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "docs/parity"
RECORDS = {
    "date": ("compiler_scalar_consumers_date_traits_5_0_6.json.gz", "a38ea335ecbb5f8fb024b0687204f5035275029ea527605cb2531d51e1e05af0"),
    "remaining": ("compiler_scalar_consumers_remaining_5_0_6.json.gz", "485688d4fc64635f49a386cf6e4408dcd880306504217cb78e8367da4e2c46e1"),
    "text": ("compiler_scalar_family_text_5_0_6.json.gz", "6d7a86e1f7d70085634d380d4b4ff85bd21ec683ff8aba0fdc2641b379c06ce2"),
    "family": ("compiler_scalar_family_remaining_5_0_6.json.gz", "b79d58b3937258a4d14addc56ea759b9c6c857ad45262084c9309f8d0ccec3eb"),
    "source": ("compiler_scalar_consumers_sources_5_0_6.json.gz", "22009bb3369b21e5d05bc5c721b1ff724410dcc58785894261248c0f8f592455"),
}
CPP = "runtime:tests/parity/compiler_scalar_consumers_probe.cpp"
RUST = "runtime:tests/parity/compiler_scalar_consumers_probe.rs"
COLLECTOR = "runtime:scripts/verify_compiler_scalar_consumers_5_0_6.py"
FAMILY = "runtime:scripts/verify_compiler_scalar_family_5_0_6.py"
READER = "runtime:scripts/verify_compiler_scalar_consumer_results_5_0_6.py"
PRECEDING = "runtime:scripts/compiler_scalar_governed_source_admission_5_0_6.py"
LEAF = "runtime:scripts/compiler_scalar_consumer_source_admission_5_0_6.py"


@cache
def bound(key):
    name, digest = RECORDS[key]
    path = EVIDENCE / name
    require(sha(path.read_bytes()) == digest, "immutable actual scalar consumer receipt: " + name)
    return read(path)


def functions(source):
    return {node.name: ast.dump(node, include_attributes=False) for node in ast.parse(source).body
            if isinstance(node, (ast.FunctionDef, ast.ClassDef))}


def validate_sources(record):
    import compiler_scalar_governed_source_admission_5_0_6 as previous
    from compiler_dynamic_route_source_admission_5_0_6 import registered_epoch_matches
    previous.evidence()
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and set(record["paths"]) == {CPP, RUST, COLLECTOR, FAMILY, READER, PRECEDING}, "five new fixture/reader sources and two preceding matcher forwards only")
    for name, row in record["paths"].items():
        path = ROOT / name.split(":", 1)[1]
        require(name.startswith("runtime:") and ".." not in path.relative_to(ROOT).parts
                and row["after_sha256"] == sha(row["after_source"].encode())
                and (row["after_sha256"] == sha(path.read_bytes())
                     or name == READER and registered_epoch_matches(path,row["after_sha256"])), "exact qualified source image")
        if name == PRECEDING:
            require(row["before_sha256"] == sha(row["before_source"].encode()), "exact preceding matcher source")
            before, after = functions(row["before_source"]), functions(row["after_source"])
            require(before.keys() == after.keys() and [key for key in before if before[key] != after[key]] == ["source_matches", "frame_matches"],
                    "preceding evidence, collection, semantic checks and controls unchanged")
        else:
            require(row["before_source"] is None and row["before_sha256"] is None, "new source identity")
    rows = record["paths"]
    for key in ("date", "remaining"):
        actual = bound(key)
        require(actual["complete"] is True and actual["driver_source"] == rows[COLLECTOR]["after_source"]
                and actual["cpp_source"] == rows[CPP]["after_source"] and actual["rust_source"] == rows[RUST]["after_source"],
                "unchanged actual seven-model generation, input and observer bodies")
    for key in ("text", "family"):
        actual = bound(key)
        require(actual["complete"] is True
                and ast.dump(ast.parse(actual["driver_source"])) == ast.dump(ast.parse(rows[FAMILY]["after_source"]))
                and actual["observer_sources"]["checker"]["cpp"] == rows[CPP]["after_source"]
                and actual["observer_sources"]["checker"]["rust"] == rows[RUST]["after_source"],
                "family source formatting preserves the complete module AST and actual observers")


@cache
def evidence():
    record = bound("source")
    validate_sources(record)
    return record


def source_rows():
    import compiler_scalar_governed_source_admission_5_0_6 as previous
    rows = dict(previous.evidence()["paths"])
    rows.update(evidence()["paths"])
    return rows


def source_matches(path, expected):
    import compiler_scalar_governed_source_admission_5_0_6 as previous
    path = Path(path).resolve()
    if not path.is_relative_to(ROOT) or not path.is_file():
        return False
    name = "runtime:" + str(path.relative_to(ROOT))
    rows = source_rows()
    if expected == sha(path.read_bytes()):
        return True
    images = {rows[name][key] for key in ("before_sha256", "after_sha256") if name in rows and rows[name].get(key) is not None}
    if name == previous.TEMPORAL:
        images.update(row[key] for row in previous.bound("temporal_source")["paths"][name].values()
                      for key in ("source_before_sha256", "source_after_sha256"))
    if name == previous.COLLECTOR:
        images.update(sha(previous.bound(key)["driver_source"].encode()) for key in ("date", "remaining"))
    if name == FAMILY:
        images.update(sha(bound(key)["driver_source"].encode()) for key in ("text", "family"))
    return expected in images


def frame_matches(current, historical):
    import compiler_scalar_governed_source_admission_5_0_6 as previous
    import compiler_dynamic_route_source_admission_5_0_6 as continuation
    rows = source_rows()
    additions = {name: row["after_sha256"] for name,row in rows.items() if row["before_source"] is None}
    additions[previous.LEAF] = rows[previous.LEAF]["after_sha256"]
    # Fresh validation/milestone records bind this leaf separately, avoiding a
    # cycle between its source image and the receipt hash embedded above.
    additions[LEAF] = sha(Path(__file__).read_bytes())
    additions["runtime:scripts/compiler_dynamic_route_source_admission_5_0_6.py"] = sha(Path(continuation.__file__).read_bytes())
    return (historical.keys() <= current.keys()
            and current.keys() - historical.keys() <= additions.keys()
            and all(current[name] == additions[name]
                    or current[name] == sha(continuation.source_path(name).read_bytes())
                    and continuation.registered_epoch_matches(continuation.source_path(name), additions[name])
                    for name in current.keys() - historical.keys())
            and all(current[name] == value or name in rows and source_matches(ROOT / name.split(":",1)[1],value)
                    or continuation.registered_epoch_matches(continuation.source_path(name),value)
                    for name,value in historical.items()))


def controls():
    record = evidence()
    changes = (
        ("unfinished",lambda r:r.update(complete=False)),
        ("missing_source",lambda r:r["paths"].pop(CPP)),
        ("forged_image",lambda r:r["paths"][RUST].update(after_source="fn main() {}\n")),
        ("unqualified_preimage",lambda r:r["paths"][PRECEDING].update(before_sha256="0"*64)),
    )
    results=[]
    for name,mutate in changes:
        changed=copy.deepcopy(record)
        mutate(changed)
        try:
            validate_sources(changed)
        except (RuntimeError,KeyError,ValueError) as error:
            results.append(dict(case=name,rejected=True,error=str(error)))
        else:
            raise RuntimeError("Accepted scalar consumer source corruption: "+name)
    from verify_compiler_generated_business_5_0_6 import execution_inputs
    current=execution_inputs()
    require(not frame_matches(dict(current,**{"runtime:unknown.cpp":"0"*64}),bound("family")["inputs_after"])
            and not frame_matches({key:value for key,value in current.items() if key != CPP},bound("family")["inputs_after"])
            and not source_matches(ROOT/"tests/CMakeLists.txt","0"*64),"unknown additions, missing inputs and unqualified hashes reject")
    results.append(dict(case="unknown_or_missing_source",rejected=True))
    return results


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check",action="store_true",required=True)
    parser.add_argument("--controls",type=Path)
    args=parser.parse_args()
    evidence()
    if args.controls:
        require(not args.controls.exists(),"preserve source corruption evidence")
        write(args.controls,dict(complete=True,whole_goal_complete=False,optimized=sys.flags.optimize,controls=controls()))
    print("Verified exact scalar consumer source admission; optimized=",sys.flags.optimize)


if __name__=="__main__":
    main()
