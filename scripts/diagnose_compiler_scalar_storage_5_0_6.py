#!/usr/bin/env python3
"""Collect complete original/compiler mapping outcomes for scalar type spellings."""

import argparse
import base64
import json
import os
from pathlib import Path
import sys
import tempfile

from verify_compiler_generated_business_5_0_6 import execution_inputs, read, sha, write
from verify_compiler_governed_execution_5_0_6 import prepare
from verify_primitive_const_governed_execution_5_0_6 import run_owned
from run_native_integration_epoch_hardened_5_0_6 import references

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
WORKER = PRODUCER / "scripts/cpp_rust_compiler_job.py"


def require(condition, message):
    if not condition:
        raise RuntimeError("Scalar storage diagnosis: " + message)


def cases():
    result = []
    for name, scalar, parent, rust_type in (
            ("qualified_integer", "integer", "number", "::core::primitive::i64"),
            ("qualified_double", "double", "number", "::core::primitive::f64"),
            ("qualified_bool", "bool", "any", "::core::primitive::bool")):
        model = ('<root name="compiled_scalar_storage" org="teaql" data_service="sqlite">'
                 '<_type_definition name="' + scalar + '" parent="' + parent + '" rustType="' + rust_type + '"/>'
                 '<gateway name="string()"/>'
                 '<reading gateway="gateway(contextRoot=true)" value="' + scalar + '()"/></root>')
        result.append(dict(name=name, scalar=scalar, rust_type=rust_type, model=model))
    return result


def execute(destination):
    require(not destination.exists(), "preserve existing diagnosis")
    record = dict(complete=False, whole_goal_complete=False, semantic_accepted=False,
                  driver_source=Path(__file__).read_text(), preparation_steps=[], steps=[], cases=[],
                  inputs_before=execution_inputs(), references_before=references())
    owned = None
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_scalar_storage_diagnosis_") as temporary:
            owned = Path(temporary)
            record["owned_directory"] = temporary
            try:
                capabilities, classpath = prepare(owned, record)
                config = record["capabilities"]
                for case in cases():
                    current = owned / case["name"]
                    current.mkdir()
                    package = dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
                                   files={"main.xml": base64.b64encode(case["model"].encode()).decode()})
                    packaged, output, report = current / "model.json", current / "cpp", current / "job.json.gz"
                    packaged.write_text(json.dumps(package, sort_keys=True))
                    command = [config["cpp_generator"]["command"][0], "-cp", classpath,
                               "CppCompilerServiceProbe", str(packaged), str(output),
                               str(Path(sys.executable).resolve()), str(WORKER), str(capabilities),
                               str(owned / "service_jobs"), str(report), "--controller"]
                    print(case["name"], "running", flush=True)
                    result = run_owned(command, cwd=ROOT,
                        env=dict(os.environ, TEAQL_LOG_ENDPOINT="off", PYTHONDONTWRITEBYTECODE="1"),
                        text=True, capture_output=True, timeout=900)
                    row = dict(case, source_package=package, command=command, exit_code=result.returncode,
                               stdout=result.stdout, stderr=result.stderr, sdk_exists=output.exists(),
                               job=read(report) if report.exists() else None)
                    record["cases"].append(row)
                    write(destination, record)
                    print(case["name"], "exit", result.returncode, flush=True)
                record["complete"] = True
            finally:
                record["owned_bytes_removed"] = sum(path.stat().st_size for path in owned.rglob("*") if path.is_file())
    finally:
        record.update(owned_directory_removed=owned is not None and not owned.exists(),
                      inputs_after=execution_inputs(), references_after=references())
        write(destination, record)
    require(record["inputs_before"] == record["inputs_after"]
            and record["references_before"] == record["references_after"], "stable diagnosis source and references")
    for row in record["cases"]:
        job = row["job"]
        original = next((step for step in (job or {}).get("steps", []) if step["label"] == "original_compile"), None)
        print(row["name"], "original_compile=", None if original is None else original["exit_code"],
              "native_generation=", row["exit_code"], "sdk=", row["sdk_exists"], flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", required=True, type=Path)
    execute(parser.parse_args().record.resolve())


if __name__ == "__main__":
    main()
