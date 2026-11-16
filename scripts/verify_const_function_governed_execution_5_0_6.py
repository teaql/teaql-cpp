#!/usr/bin/env python3
"""Carry qualified constant-function projections through governed graph Save."""

import argparse
import ast
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import signal
import sys
import types

from const_function_source_admission_5_0_6 import evidence as installed_evidence
from verify_const_function_sql_5_0_6 import NAMES

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "docs/parity"
ENGINE = ROOT / "scripts/verify_primitive_const_governed_execution_5_0_6.py"
ENGINE_SHA = "c889bf94646286acdcc80c54a84e1b3e6438b48afdfc77eb30ffc404b6131715"
SOURCE = EVIDENCE / "ap02_const_function_sql_5_0_6.json.gz"
SOURCE_SHA = "5202a86d855aa21668053086f31ab39294fc97e12b038bf50d50cb3560c253dc"
INSTALLED = EVIDENCE / "ap02_const_function_source_review_5_0_6.json.gz"
INSTALLED_SHA = "031bce156e34764608ab2b159d7609444976bf4fc5361a01c1c33554c55814c8"


def require(value, message):
    if not value:
        raise RuntimeError("Constant-function governed execution: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


def cases(source):
    require(source["complete"] is True and source["differences"] == []
            and tuple(row["name"] for row in source["cases"]) == NAMES,
            "all eight qualified generated SQLite consumers")
    relations, bindings = [], []
    for row in source["cases"]:
        text = row["input"]
        require(text.startswith("17, reverse_relation(")
                and text.endswith(", audit_value_max_len=21"), "exact accepted source envelope")
        text = text[len("17, "):-len(", audit_value_max_len=21")]
        edits = {
            'name="links"': 'name="primitive_' + row["name"] + '"',
            'target="Child"': 'target="Enrollment"',
            'foreign_key="owner"': 'foreign_key="school"',
            "crate::child::Child": "crate::enrollment::Enrollment",
        }
        for before, after in edits.items():
            require(text.count(before) == 1, "one source-qualified model identity substitution")
            text = text.replace(before, after, 1)
        relations.append(text)
        bindings.append(dict(source_name=row["name"], source_input=row["input"],
                             source_expression=row["source_expression"],
                             derived_relation=text, edits=list(map(list, edits.items()))))
    prefix = "13, " + ", ".join(relations)
    suffix = ', audit_value_max_len=17, audit_mask_fields="name"'
    invalid = ', reverse_relation(name="discarded",target="Enrollment",json_type="Vec<")'
    return {
        "complete": dict(name="complete", input=prefix + suffix, limit="17",
                         masked="PR*****...*****RY", bindings=bindings),
        "partial": dict(name="partial", input=prefix + invalid + suffix, limit="13",
                        masked="PR***...***RY", bindings=bindings),
    }


def load_engine(adaptations=None):
    from generator_evaluation_source_admission_5_0_6 import registered_epoch_matches
    require(registered_epoch_matches(ENGINE, ENGINE_SHA), "qualified complete execution/validation engine")
    require(sha(SOURCE.read_bytes()) == SOURCE_SHA
            and sha(INSTALLED.read_bytes()) == INSTALLED_SHA, "immutable actual source/installation evidence")
    installed_evidence()
    engine = types.ModuleType("const_function_governed_engine")
    engine.__file__ = str(ENGINE)
    exec(compile(ENGINE.read_text(), str(ENGINE), "exec"), engine.__dict__)
    engine.SOURCE, engine.SOURCE_SHA256 = SOURCE, SOURCE_SHA
    engine.INSTALLED, engine.INSTALLED_SHA256 = INSTALLED, INSTALLED_SHA
    engine.NAMES, engine.cases = NAMES, cases
    original_run = engine.run_owned
    preceding = read(EVIDENCE / "ap02_primitive_const_path_installed_5_0_6.json.gz")
    maven = preceding["steps"][0]["command"][0]

    def run(command, **kwargs):
        source_command = list(map(str, command))
        executed = list(source_command)
        if executed[0] == "mvn":
            executed[0] = maven
        changes = {}
        env = kwargs.get("env")
        key = "CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER"
        if env is not None and key in env:
            require(os.environ.get("CC"), "activate the locked native toolchain")
            env = dict(env)
            changes[key] = dict(before=env[key], after=os.environ["CC"])
            env[key] = os.environ["CC"]
            kwargs["env"] = env
        if adaptations is not None:
            adaptations.append(dict(source_command=source_command, executed_command=executed,
                                    environment_changes=changes))
        return original_run(executed, **kwargs)

    engine.run_owned = run
    return engine


def validate(record, sinks):
    from generator_evaluation_source_admission_5_0_6 import registered_epoch_matches
    require(registered_epoch_matches(Path(__file__), sha(record["adapter_source"].encode()))
            and record["adapter_sha256"] == sha(record["adapter_source"].encode())
            and registered_epoch_matches(ENGINE, record["execution_engine_sha256"]),
            "exact current adapter and unchanged complete engine")
    engine = load_engine()
    engine.validate(record, sinks)
    require(len(record["process_adaptations"]) == 41,
            "all original engine processes retain their actual launcher adaptations")
    for step, actual in zip(record["steps"], record["process_adaptations"], strict=True):
        require(list(map(str, step["command"])) == actual["source_command"],
                "recorded source command and actual process agree")
        expected = list(actual["source_command"])
        if expected[0] == "mvn":
            expected[0] = read(EVIDENCE / "ap02_primitive_const_path_installed_5_0_6.json.gz")["steps"][0]["command"][0]
        require(actual["executed_command"] == expected, "only the existing Maven path is resolved")
        require(set(actual["environment_changes"]) <= {"CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER"},
                "only the native Rust linker is normalized")
        for change in actual["environment_changes"].values():
            require(Path(change["after"]).is_file()
                    and sha(Path(change["after"]).read_bytes()) == record["native_compiler"]["sha256"],
                    "the actual Rust linker uses the same locked native compiler")


def execute(case_name, record_path, sink_path):
    adaptations = []
    engine = load_engine(adaptations)
    error = None
    try:
        engine.execute(case_name, record_path, sink_path)
    except BaseException as caught:
        error = caught
        raise
    finally:
        if record_path.exists():
            record = read(record_path)
            record.update(adapter_source=Path(__file__).read_text(),
                          adapter_sha256=sha(Path(__file__).read_bytes()),
                          execution_engine_sha256=ENGINE_SHA, process_adaptations=adaptations)
            record["complete"] = record.get("complete") is True and error is None
            write(record_path, record)
    validate(read(record_path), read(sink_path))


def controls(record, sinks):
    engine = load_engine()
    inherited = engine.controls(record, sinks)
    extra = (
        ("wrong_adapter", lambda r, s: r.update(adapter_source="pass\n")),
        ("missing_actual_launcher", lambda r, s: r["process_adaptations"].pop()),
        ("wrong_executed_command", lambda r, s: r["process_adaptations"][0].update(executed_command=["unqualified"])),
    )
    result = list(inherited)
    for name, mutate in extra:
        changed, changed_sinks = copy.deepcopy(record), copy.deepcopy(sinks)
        mutate(changed, changed_sinks)
        try:
            validate(changed, changed_sinks)
        except RuntimeError as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted governed adapter corruption: " + name)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--execute", action="store_true")
    action.add_argument("--check", action="store_true")
    parser.add_argument("--case", choices=("complete", "partial"), required=True)
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--sink-record", type=Path, required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    if args.execute:
        def stop_owned_processes(signum, frame):
            raise KeyboardInterrupt("Stop this execution and reap its owned children")
        signal.signal(signal.SIGTERM, stop_owned_processes)
        execute(args.case, args.record.resolve(), args.sink_record.resolve())
    else:
        record, sinks = read(args.record), read(args.sink_record)
        require(record["case"]["name"] == args.case, "exact selected receipt")
        validate(record, sinks)
        rejected = controls(record, sinks)
        if args.controls:
            require(not args.controls.exists(), "preserve prior control evidence")
            write(args.controls, dict(complete=True, whole_goal_complete=False,
                  optimized=sys.flags.optimize, controls=rejected,
                  execution_sha256=sha(args.record.read_bytes()), sinks_sha256=sha(args.sink_record.read_bytes())))
        print("Constant-function governed execution:", len(rejected), "corruptions rejected; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
