#!/usr/bin/env python3
"""Compare the actual locked entity derive with native narrow integer decoding."""

import argparse
import copy
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
from types import SimpleNamespace

from verify_audit_metadata_source_5_0_6 import identities
from verify_compiler_generated_business_5_0_6 import execution_inputs, read, references, require, sha, write
from verify_compiler_projection_generation_5_0_6 import native_command
from verify_primitive_const_governed_execution_5_0_6 import run_owned

ROOT = Path(__file__).resolve().parents[1]
RUNTIME = ROOT / ".reference/teaql-rs"
PARITY = ROOT / "tests/parity"
HEADER = ROOT / "teaql_runtime/include/teaql/runtime/generated_scalar_codec.hpp"
CPP = PARITY / "narrow_integer_codec_probe.cpp"
RUST = PARITY / "narrow_integer_codec_5_0_6.rs"
sys.path.insert(0, str(PARITY))
from compare_values import json_lines, same


def cases():
    signed = [-2**63, -4294967297, -4294967296, -2147483649, -2147483648,
              -65537, -65536, -32769, -32768, -1, 0, 1, 32767, 32768,
              65535, 65536, 2147483647, 2147483648, 4294967295, 4294967296, 2**63 - 1]
    unsigned = [0, 1, 32767, 32768, 65535, 65536, 2147483647, 2147483648,
                4294967295, 4294967296, 2**63 - 1, 2**63, 2**64 - 1]
    values = [dict(kind="i64", value=str(value)) for value in signed]
    values += [dict(kind="u64", value=str(value)) for value in unsigned]
    values += [dict(kind="null"), dict(kind="typed_null"), dict(kind="text", value="1"),
        dict(kind="bool", value=True), dict(kind="bool", value=False),
        dict(kind="f64", value="1"), dict(kind="f64", value="-1"), dict(kind="f64", value="1.25"),
        dict(kind="decimal", value="1"), dict(kind="decimal", value="4294967296"),
        dict(kind="json", value=None), dict(kind="json", value=1)]
    return [dict(carrier=carrier, optional=optional, present=present, value=value)
            for carrier in ("i16", "i32", "u16", "u32") for optional in (False, True)
            for present, value in [(False, dict(kind="null")), *[(True, value) for value in values]]]


def manifest():
    packages = ("teaql-core", "teaql-runtime", "teaql-macros", "teaql-data-service", "teaql-sql", "teaql-provider-sqlite")
    return ('[package]\nname="teaql_narrow_integer_codec_probe"\nversion="0.1.0"\nedition="2024"\n'
        '[workspace]\n[[bin]]\nname="narrow_integer_codec"\npath=' + json.dumps(str(RUST))
        + '\n[dependencies]\nserde_json="=1.0.150"\nchrono="=0.4.45"\n'
        + "".join(name + '={path=' + json.dumps(str(RUNTIME / name)) + '}\n' for name in packages[:3])
        + '[patch.crates-io]\n' + "".join(name + '={path=' + json.dumps(str(RUNTIME / name)) + '}\n' for name in packages))


def execute(path, diagnose):
    require(not path.exists() and shutil.disk_usage(ROOT).free > 3 * 1024 ** 3, "fresh narrow-codec receipt and owned output space")
    record = dict(complete=False, whole_goal_complete=False, diagnosis=diagnose,
        driver_source=Path(__file__).read_text(), cpp_source=CPP.read_text(), rust_source=RUST.read_text(),
        kernel_source=HEADER.read_text(), inputs_before=execution_inputs(), references_before=references(),
        cases=cases(), steps=[])
    owned = None

    def run(label, command, *, env=None, input_text=None):
        print(label, "running", flush=True)
        result = run_owned(command, cwd=ROOT, env=env, input=input_text, text=True, capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=list(map(str, command)), input=input_text,
            exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, "exit", result.returncode, flush=True)
        require(result.returncode == 0, label + " failed; inspect retained raw output")
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_narrow_integer_codec_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                (owned / "Cargo.toml").write_text(manifest())
                shutil.copyfile(RUNTIME / "Cargo.lock", owned / "Cargo.lock")
                from compiler_business_id_source_admission_5_0_6 import bound
                linker = bound("runtime")["capabilities"]["linker"]
                require(sha(Path(linker["path"]).read_bytes()) == linker["sha256"], "existing locked original-consumer linker")
                env = dict(os.environ, CARGO_TARGET_DIR=str(owned / "target"), CARGO_INCREMENTAL="0",
                    CARGO_PROFILE_DEV_DEBUG="0", CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=linker["path"],
                    TEAQL_LOG_ENDPOINT="off", PYTHONDONTWRITEBYTECODE="1")
                record["linker"] = linker
                run("original_resolve", ["cargo", "metadata", "--offline", "--format-version", "1", "--manifest-path", owned / "Cargo.toml"], env=env)
                require(identities(owned / "Cargo.lock") <= identities(RUNTIME / "Cargo.lock"), "reference registry dependency identities")
                record["consumer_lock"] = (owned / "Cargo.lock").read_text()
                run("original_build", ["cargo", "build", "--offline", "--locked", "--manifest-path", owned / "Cargo.toml", "-j", "2"], env=env)
                run("native_build", native_command(PARITY, CPP, owned / "native"), env=env)
                data = "".join(json.dumps(row, sort_keys=True) + "\n" for row in record["cases"])
                record["original"] = json_lines(run("original_run", [owned / "target/debug/narrow_integer_codec"], env=env, input_text=data))
                record["cpp"] = json_lines(run("native_run", [owned / "native"], env=env, input_text=data))
                require(len(record["original"]) == len(record["cpp"]) == len(record["cases"]), "complete actual narrow-codec observations")
                record["mismatches"] = [index for index, (left, right) in enumerate(zip(record["original"], record["cpp"], strict=True)) if not same(left, right)]
                require(bool(record["mismatches"]) is diagnose, "actual diagnosis or complete narrow-codec parity")
                record["complete"] = True
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        record.update(owned_directory_removed=owned is not None and not owned.exists(),
            inputs_after=execution_inputs(), references_after=references())
        write(path, record)
    require(record["inputs_before"] == record["inputs_after"] and record["references_before"] == record["references_after"], "unchanged complete codec sources and locked references")
    print("cases", len(record["cases"]), "mismatches", len(record["mismatches"]), flush=True)


def validate(record):
    from narrow_integer_source_admission_5_0_6 import registered_epoch_matches, frame_matches, bound
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["diagnosis"] is False and record["mismatches"] == [], "completed narrow-codec qualification")
    require(registered_epoch_matches(Path(__file__), sha(record["driver_source"].encode()))
            and registered_epoch_matches(HEADER, sha(record["kernel_source"].encode()))
            and record["cpp_source"] == CPP.read_text() and record["rust_source"] == RUST.read_text(), "exact executed sources")
    require(record["inputs_before"] == record["inputs_after"] and frame_matches(execution_inputs(), record["inputs_after"])
            and record["references_before"] == record["references_after"] == references(), "unchanged source and reference frames")
    require(record["owned_directory_removed"] is True and not Path(record["owned_directory"]).exists(), "owned outputs removed")
    require(same(record["cases"], cases()) and len(record["cases"]) == 376, "complete narrow-codec input membership")
    owned = Path(record["owned_directory"])
    expected = {
        "original_resolve": ["cargo", "metadata", "--offline", "--format-version", "1", "--manifest-path", str(owned / "Cargo.toml")],
        "original_build": ["cargo", "build", "--offline", "--locked", "--manifest-path", str(owned / "Cargo.toml"), "-j", "2"],
        "native_build": list(map(str, native_command(PARITY, CPP, owned / "native"))),
        "original_run": [str(owned / "target/debug/narrow_integer_codec")],
        "native_run": [str(owned / "native")],
    }
    require([step["label"] for step in record["steps"]] == list(expected), "all actual build and execution commands")
    for step in record["steps"]:
        require(step["command"] == expected[step["label"]] and type(step["exit_code"]) is int
                and step["exit_code"] == 0, "exact successful command invocation")
    data = "".join(json.dumps(row, sort_keys=True) + "\n" for row in cases())
    for step, key in ((record["steps"][3], "original"), (record["steps"][4], "cpp")):
        require(step["input"] == data and same(json_lines(step["stdout"]), record[key])
                and len(record[key]) == 376, "complete authentic raw inputs and outputs")
    from compare_narrow_integer_codec_5_0_6 import corpus
    expected_outputs = [row["expected"] for row in corpus()["cases"]]
    require(same(record["original"], expected_outputs) and same(record["cpp"], record["original"]), "all actual reference/native values and errors")
    executed = bound("runtime")
    require(record["consumer_lock"] == executed["consumer_lock"] and record["linker"] == executed["linker"]
            and identities(SimpleNamespace(read_text=lambda: record["consumer_lock"])) <= identities(RUNTIME / "Cargo.lock")
            and sha(Path(record["linker"]["path"]).read_bytes()) == record["linker"]["sha256"], "locked dependencies and linker")


def controls(record):
    validate(record)
    mutations = (
        ("incomplete", lambda r: r.update(complete=False)),
        ("whole_goal_claim", lambda r: r.update(whole_goal_complete=True)),
        ("relabeled_diagnosis", lambda r: r.update(diagnosis=True)),
        ("driver_source", lambda r: r.update(driver_source=r["driver_source"] + "# forged\n")),
        ("kernel_source", lambda r: r.update(kernel_source=r["kernel_source"] + "// forged\n")),
        ("cpp_observer", lambda r: r.update(cpp_source=r["cpp_source"] + "// forged\n")),
        ("rust_observer", lambda r: r.update(rust_source=r["rust_source"] + "// forged\n")),
        ("lost_case", lambda r: r["cases"].pop()),
        ("lost_output", lambda r: r["cpp"].pop()),
        ("changed_output", lambda r: r["cpp"][0].update(value=999)),
        ("raw_output", lambda r: r["steps"][3].update(stdout="")),
        ("raw_input", lambda r: r["steps"][4].update(input="{}\n")),
        ("wrong_command", lambda r: r["steps"][1]["command"].append("--different")),
        ("boolean_status", lambda r: r["steps"][0].update(exit_code=False)),
        ("cleanup", lambda r: r.update(owned_directory_removed=False)),
        ("linker", lambda r: r["linker"].update(sha256="0" * 64)),
        ("empty_dependency_graph", lambda r: r.update(consumer_lock="version = 4\npackage = []\n")),
        ("foreign_linker", lambda r: r.update(linker=dict(path="/bin/true", sha256=sha(Path("/bin/true").read_bytes())))),
    )
    result = []
    for name, mutate in mutations:
        changed = copy.deepcopy(record)
        mutate(changed)
        try:
            validate(changed)
        except (RuntimeError, KeyError, IndexError) as error:
            result.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted narrow-codec receipt corruption: " + name)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--diagnose", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    if args.check:
        require(not args.diagnose, "diagnosis is not qualification")
        result = controls(read(args.record))
        if args.controls:
            require(not args.controls.exists(), "preserve prior controls")
            write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize, controls=result))
        print("Verified all narrow integer outcomes and", len(result), "controls; optimized=", sys.flags.optimize)
    else:
        require(args.controls is None, "controls require receipt check")
        execute(args.record, args.diagnose)


if __name__ == "__main__":
    main()
