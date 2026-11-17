#!/usr/bin/env python3
"""Join exact compiled scalar SDKs to direct Checker/Fix and sample consumers."""

import argparse
import base64
import itertools
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

from verify_audit_metadata_source_5_0_6 import identities
from verify_compiler_generated_business_5_0_6 import (
    execution_inputs, manifest, native_inputs, read, references, require, run_owned, sha, write,
)
from verify_compiler_governed_execution_5_0_6 import LOCK, prepare
from verify_compiler_projection_generation_5_0_6 import hashes, native_command
from verify_compiler_scalar_governed_5_0_6 import selected_models, typed

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "tests/parity/compiler_scalar_consumers_probe.cpp"
RUST = ROOT / "tests/parity/compiler_scalar_consumers_probe.rs"
CLOCK = ROOT / "build_native_current/libteaql_sample_fixed_clock.so"
WALL = 1787110200123
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import json_lines, same


def inputs(case):
    result = []
    values = [case["values"][0], None] if case["optional"] else [case["values"][0]]
    for status, root, gateway, loaded, logic, value in itertools.product(
        ("create", "update", "unknown"), (None, 1, 2), (0, 1, 2),
        ("all", "no_value", "identity"), ("noop", "fix", "reject"), values,
    ):
        replacement = None if case["optional"] else case["values"][1]
        result.append(dict(kind="checker", status=status, root=root, gateway=gateway,
            loaded=loaded, logic=logic, value=value, next_value=replacement,
            typed=typed(case, value), next_typed=typed(case, replacement), wall_millis=WALL))
    for name, scale, seed, rounds, root, fault in (
        ("tiny", "Tiny", 0, 1, True, "none"),
        ("repeat", "Tiny", 17, 2, True, "none"),
        ("small", "Small", 0, 1, True, "none"),
        ("medium", "Medium", 17, 1, True, "none"),
        ("missing_root", "Tiny", 0, 1, False, "none"),
        ("trigger", "Tiny", 0, 1, True, "trigger"),
    ):
        result.append(dict(kind="sample", name=name, scale=scale, seed=seed,
                           rounds=rounds, root=root, fault=fault, wall_millis=WALL))
    return result


def execute(path, selection):
    require(not path.exists() and shutil.disk_usage(ROOT).free > 4 * 1024 ** 3, "fresh scalar consumer record and owned build space")
    require(CLOCK.is_file(), "existing native test-only sample process clock")
    models = selected_models(selection)
    record = dict(complete=False, whole_goal_complete=False, driver_source=Path(__file__).read_text(),
        cpp_source=CPP.read_text(), rust_source=RUST.read_text(), preparation_steps=[], steps=[], models=[],
        selected_models=[row["case"]["name"] for row in models], inputs_before=execution_inputs(),
        references_before=references(), native_before=native_inputs(), clock_before=sha(CLOCK.read_bytes()), compiler_jobs_reused=True)
    owned = None

    def run(label, command, *, env=None, payload=None):
        print(label, "running", flush=True)
        result = run_owned(command, cwd=ROOT, env=env, input=payload, text=True, capture_output=True, timeout=1200)
        record["steps"].append(dict(label=label, command=list(map(str, command)), input=payload,
            exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, "exit", result.returncode, flush=True)
        require(type(result.returncode) is int and result.returncode == 0, "actual scalar consumer process failed: " + label)
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_scalar_consumers_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                _, classpath = prepare(owned, record)
                config = record["capabilities"]
                env = dict(os.environ, CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                    CARGO_TARGET_DIR=str(owned / "consumer_target"),
                    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=config["linker"]["path"],
                    TEAQL_LOG_ENDPOINT="off", PYTHONDONTWRITEBYTECODE="1")
                record["environment_overrides"] = {name: env[name] for name in (
                    "CARGO_INCREMENTAL", "CARGO_PROFILE_DEV_DEBUG", "CARGO_TARGET_DIR",
                    "CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER", "TEAQL_LOG_ENDPOINT")}
                runtime_env = dict(env, LD_PRELOAD=str(CLOCK), TEAQL_TEST_SAMPLE_WALL_MILLIS=str(WALL))
                record["process_clock"] = dict(path=str(CLOCK), source="tests/support/fixed_sample_clock.cpp",
                    sha256=record["clock_before"], environment={name:runtime_env[name] for name in ("LD_PRELOAD", "TEAQL_TEST_SAMPLE_WALL_MILLIS")})
                for source in models:
                    case = source["case"]
                    name = case["name"]
                    current = owned / name
                    current.mkdir()
                    model, document = current / "model.json", current / "resolution.json"
                    model.write_bytes(base64.b64decode(case["job"]["source_package_bytes_base64"], validate=True))
                    document.write_text(json.dumps(case["job"]["resolution_document"], sort_keys=True))
                    row = dict(name=name, kind=case["kind"], optional=case["optional"], carrier=case["carrier"],
                        compiler_receipt=source["compiler_receipt"], compiler_receipt_sha256=source["compiler_receipt_sha256"],
                        original_compiler_job_reused=True, source_package_bytes_base64=case["job"]["source_package_bytes_base64"],
                        resolution_document=case["job"]["resolution_document"], inputs=inputs(case))
                    record["models"].append(row)
                    run(name + "_cpp_generate", [config["cpp_generator"]["command"][0], "-cp", classpath, "CppCompiledModel",
                        model, current / "cpp", "--rust-configuration=" + str(document), "--rust-toolchain-lock=" + str(LOCK), "--source-package"], env=env)
                    row["cpp_sources"] = hashes(current / "cpp")
                    require(row["cpp_sources"] == case["cpp_sources"], "complete regenerated C++ SDK preserves every qualified file")
                    run(name + "_original_generate", [*config["reference_generator"]["command"], model,
                        current / "original", current / "inventory.json", "--source-package", "--semantic-storage"], env=env)
                    row["original_sources"] = hashes(current / "original")
                    require(row["original_sources"] == case["original_sources"], "entire independently regenerated original SDK preserved")
                    observer = RUST
                    rust_source = RUST.read_text()
                    row["rust_argument_adapter"] = None
                    if case["optional"] and case["kind"] != "timestamp":
                        before = "entity.update_value(desired);"
                        after = "entity.update_value(desired.map(teaql_core::Value::from).unwrap_or(teaql_core::Value::Null));"
                        require(rust_source.count(before) == 2, "two observer optional setter arguments")
                        rust_source = rust_source.replace(before, after)
                        observer = current / "observer.rs"
                        observer.write_text(rust_source)
                        row["rust_argument_adapter"] = dict(before=before, after=after, replacements=2)
                    row["rust_observer_source"] = rust_source
                    harness = current / "harness"
                    harness.mkdir()
                    text = manifest(current / "original", "domain", observer)
                    (harness / "Cargo.toml").write_text(text)
                    shutil.copyfile(ROOT / ".reference/teaql-rs/Cargo.lock", harness / "Cargo.lock")
                    run(name + "_original_resolve", ["cargo", "metadata", "--offline", "--format-version", "1", "--manifest-path", harness / "Cargo.toml"], env=env)
                    require(identities(harness / "Cargo.lock") <= identities(ROOT / ".reference/teaql-rs/Cargo.lock"), "exact locked original dependency identities")
                    run(name + "_original_build", ["cargo", "build", "--offline", "--locked", "--manifest-path", harness / "Cargo.toml", "-j", "2"], env=env)
                    command = native_command(current / "cpp/include", CPP, owned / "native_probe")
                    command[1:1] = ["-I" + str(ROOT / "tests/parity"), "-DTEAQL_EXPECTED_VALUE_TYPE=" + case["carrier"]]
                    run(name + "_native_build", command, env=env)
                    payload = "".join(json.dumps(value) + "\n" for value in row["inputs"])
                    row["original"] = json_lines(run(name + "_original_run", [owned / "consumer_target/debug/compiler_generated_business_probe"], env=runtime_env, payload=payload))
                    row["cpp"] = json_lines(run(name + "_native_run", [owned / "native_probe"], env=runtime_env, payload=payload))
                    row.update(consumer_manifest=text, consumer_lock=(harness / "Cargo.lock").read_text(),
                        original_sources_after=hashes(current / "original"), cpp_sources_after=hashes(current / "cpp"))
                record["complete"] = True
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        record.update(owned_directory_removed=owned is not None and not owned.exists(), inputs_after=execution_inputs(),
            references_after=references(), native_after=native_inputs(), clock_after=sha(CLOCK.read_bytes()))
        write(path, record)
    require(record["inputs_before"] == record["inputs_after"] and record["references_before"] == record["references_after"]
            and record["native_before"] == record["native_after"] and record["clock_before"] == record["clock_after"], "frozen scalar consumer source, artifacts and clock")
    print("Checker/sample execution collected; semantic comparison remains a separate exit", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", action="store_true", required=True)
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--case", action="append")
    args = parser.parse_args()
    execute(args.record, args.case)


if __name__ == "__main__":
    main()
