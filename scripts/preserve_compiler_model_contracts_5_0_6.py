#!/usr/bin/env python3
"""Reproduce every retained default SDK after adding compiled model metadata."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import sys
import tempfile

from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references
from verify_primitive_const_governed_execution_5_0_6 import run_owned
from preserve_compiler_projection_contracts_5_0_6 import BATCH, cases

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "docs/parity/compiler_model_configuration_full_5_0_6.json.gz"


def require(value, message):
    if not value:
        raise RuntimeError("Compiled model default SDKs: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


def execute(path):
    require(not path.exists(), "preserve original default SDK receipts")
    source = read(SOURCE)
    require(source["complete"] is True, "actual compiled-metadata generation checkpoint")
    record = dict(complete=False, whole_goal_complete=False, actual_cpp_compiler_processes=0,
                  reused_cpp_compiler_outcomes=406, generation_sha256=sha(SOURCE.read_bytes()),
                  driver_source=Path(__file__).read_text(), batch_source=BATCH, planned_cases=cases(),
                  input_hashes_before=capture_inputs(), references_before=references(), steps=[])

    def run(label, command):
        command = list(map(str, command))
        result = run_owned(command, cwd=ROOT, text=True, capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, "exit", result.returncode, flush=True)
        require(result.returncode == 0, "actual current SDK generation failed")
        return result

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_preserve_compiler_model_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                batch = owned / "PreserveDefaultSdks.java"
                batch.write_text(BATCH)
                inputs = owned / "cases.jsonl"
                inputs.write_text("".join(json.dumps(row, sort_keys=True) + "\n" for row in record["planned_cases"]))
                (owned / "cpp_classes").mkdir()
                command = next(step["command"] for step in source["steps"] if step["label"] == "cpp_javac")
                command = [part.replace(source["owned_directory"], directory) for part in command] + [str(batch)]
                run("current_javac", command)
                junit = next(step["command"] for step in source["steps"] if step["label"] == "cpp_junit")
                classpath = junit[junit.index("-cp") + 1].replace(source["owned_directory"], directory)
                result = run("complete_sdk_batch", [junit[0], "-cp", classpath, "PreserveDefaultSdks", inputs, owned])
                record["cases"] = [json.loads(line.removeprefix("CPP_SDK_FRAME ")) for line in result.stdout.splitlines()
                                   if line.startswith("CPP_SDK_FRAME ")]
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
        record.update(complete=True, owned_directory_removed=not Path(directory).exists(),
                      input_hashes_after=capture_inputs(), references_after=references())
        validate(record)
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        if "owned_directory" in record:
            record["owned_directory_removed"] = not Path(record["owned_directory"]).exists()
        write(path, record)


def validate(record):
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and type(record["actual_cpp_compiler_processes"]) is int and record["actual_cpp_compiler_processes"] == 0
            and record["reused_cpp_compiler_outcomes"] == 406, "explicit compiler outcome reuse only")
    require(record["generation_sha256"] == sha(SOURCE.read_bytes()) and record["driver_source"] == Path(__file__).read_text()
            and record["batch_source"] == BATCH, "current source and unchanged full SDK batch")
    require(record["planned_cases"] == cases() and len(record["cases"]) == 406, "complete original SDK membership")
    for actual, expected in zip(record["cases"], record["planned_cases"], strict=True):
        require(actual["id"] == expected["id"] and actual["match"] is True and actual["files"] == expected["files"],
                "complete unconfigured code/guidance bytes unchanged")
    require(record["input_hashes_before"] == record["input_hashes_after"]
            and record["references_before"] == record["references_after"]
            and record["owned_directory_removed"] is True, "frozen source and owned cleanup")
    require([step["label"] for step in record["steps"]] == ["current_javac", "complete_sdk_batch"]
            and all(type(step["exit_code"]) is int and step["exit_code"] == 0 for step in record["steps"]), "actual current Java compiler and batch")
    output = record["steps"][-1]["stdout"]
    require(record["cases"] == [json.loads(line.removeprefix("CPP_SDK_FRAME ")) for line in output.splitlines()
                                if line.startswith("CPP_SDK_FRAME ")]
            and "CPP_SDK_COUNT 406" in output, "raw complete generation output")


def controls(path):
    record = read(path)
    validate(record)
    mutations = (
        ("incomplete", lambda r: r.update(complete=False)),
        ("missing_sdk", lambda r: r["cases"].pop()),
        ("wrong_sdk", lambda r: r["cases"][0]["files"].update(invented="0" * 64)),
        ("invented_compilation", lambda r: r.update(actual_cpp_compiler_processes=406)),
        ("missing_process", lambda r: r["steps"].pop()),
        ("boolean_exit", lambda r: r["steps"][0].update(exit_code=False)),
        ("lost_frame", lambda r: r["steps"][-1].update(stdout=r["steps"][-1]["stdout"].replace("CPP_SDK_FRAME ", "MISSING ", 1))),
        ("missing_cleanup", lambda r: r.update(owned_directory_removed=False)),
    )
    rejected = []
    for name, mutate in mutations:
        changed = copy.deepcopy(record)
        mutate(changed)
        try:
            validate(changed)
        except RuntimeError as error:
            rejected.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Unconfigured SDK corruption accepted: " + name)
    return dict(complete=True, optimized=sys.flags.optimize, execution_sha256=sha(path.read_bytes()), controls=rejected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", type=Path)
    parser.add_argument("--controls", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.execute:
        execute(args.execute)
    elif args.controls and args.output:
        require(not args.output.exists(), "preserve default SDK controls")
        result = controls(args.controls)
        write(args.output, result)
        print(json.dumps(dict(complete=True, optimized=result["optimized"], controls=len(result["controls"]))))
    else:
        parser.error("Choose --execute or --controls with --output")


if __name__ == "__main__":
    main()
