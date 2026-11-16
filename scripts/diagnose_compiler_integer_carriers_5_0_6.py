#!/usr/bin/env python3
"""Compare complete original compilation and C++ mappings for integer carriers."""

import argparse
import base64
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

from verify_compiler_generated_business_5_0_6 import execution_inputs, read, references, require, write
from verify_compiler_governed_execution_5_0_6 import prepare
from verify_compiler_source_service_5_0_6 import artifact_sha
from verify_primitive_const_governed_execution_5_0_6 import run_owned

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
WORKER = PRODUCER / "scripts/cpp_rust_compiler_job.py"
EVALUATOR = PRODUCER / "tools/cpp_evaluation/CppEvaluationProbe.java"
SOURCE_READER = PRODUCER / "tools/cpp_model_source/CppModelSource.java"


def cases():
    result = []
    for name, carrier, optional in (
            ("optional_i16", "i16", True), ("optional_i32", "i32", True),
            ("optional_u16", "u16", True), ("optional_u32", "u32", True),
            ("optional_i64", "i64", True), ("required_i16", "i16", False)):
        model = ('<root name="compiled_integer_carrier" org="teaql" data_service="sqlite">'
            '<_type_definition name="bounded_value" parent="integer" rustType="' + carrier + '"/>'
            '<gateway name="string()"/>'
            '<reading gateway="gateway(contextRoot=true)" value="bounded_value()'
            + ('?' if optional else '') + '"/></root>')
        result.append(dict(name=name, carrier=carrier, optional=optional, model=model))
    return result


def execute(path):
    require(not path.exists() and shutil.disk_usage(ROOT).free > 3 * 1024 ** 3, "fresh integer diagnosis and output space")
    record = dict(complete=False, whole_goal_complete=False, semantic_acceptance=False,
        driver_source=Path(__file__).read_text(), worker_source=WORKER.read_text(),
        preparation_steps=[], evaluation_steps=[], cases=[], inputs_before=execution_inputs(), references_before=references())
    owned = None
    try:
        with tempfile.TemporaryDirectory(prefix="teaql_integer_carrier_diagnosis_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                capability_path, classpath = prepare(owned, record)
                config = record["capabilities"]
                for case in cases():
                    current = owned / case["name"]
                    current.mkdir()
                    package = dict(schema="teaql.model_source_package.v1", entrance_file="main.xml",
                                   files={"main.xml": base64.b64encode(case["model"].encode()).decode()})
                    source, sdk, report = current / "model.json", current / "cpp", current / "job.json.gz"
                    source.write_text(json.dumps(package, sort_keys=True))
                    record["cases"].append(dict(case, source_package=package))
                for label, key in (("original", "reference_generator"), ("cpp", "cpp_generator")):
                    base = config[key]["command"]
                    cp = base[base.index("-cp") + 1]
                    calls = [(label + "_evaluation_javac", ["javac", "--release", "17", "-cp", cp,
                        "-d", str(owned / ("original_classes" if label == "original" else "cpp_classes")),
                        str(EVALUATOR), str(SOURCE_READER)]),
                        (label + "_evaluate", [base[0], "-cp", cp, "CppEvaluationProbe",
                            *[str(owned / row["name"] / "model.json") for row in record["cases"]]])]
                    for name, command in calls:
                        result = run_owned(command, cwd=ROOT, text=True, capture_output=True, timeout=900)
                        record["evaluation_steps"].append(dict(label=name, command=command,
                            exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
                        require(result.returncode == 0, "independent original/current evaluation")
                        if name.endswith("_evaluate"):
                            values = [json.loads(line.removeprefix("TEAQL_EVALUATION_JSON=")) for line in result.stdout.splitlines()
                                      if line.startswith("TEAQL_EVALUATION_JSON=")]
                            require(len(values) == len(record["cases"]), "complete independent model evaluations")
                            for row, value in zip(record["cases"], values, strict=True):
                                row[label + "_evaluation"] = value
                require(all(row["original_evaluation"] == row["cpp_evaluation"] for row in record["cases"]), "exact independent original/current evaluation agreement")
                config["schema"] = "teaql.rust_configuration.job.v4"
                config["artifacts"] = {name: artifact_sha(Path(name)) for name in config["artifacts"]}
                capability_path.write_text(json.dumps(config, sort_keys=True))
                for row in record["cases"]:
                    current = owned / row["name"]
                    source, sdk, report = current / "model.json", current / "cpp", current / "job.json.gz"
                    command = [config["cpp_generator"]["command"][0], "-cp", classpath,
                        "CppCompilerServiceProbe", str(source), str(sdk), str(Path(sys.executable).resolve()),
                        str(WORKER), str(capability_path), str(owned / "service_jobs"), str(report), "--controller"]
                    print(row["name"], "running", flush=True)
                    result = run_owned(command, cwd=ROOT, env=dict(os.environ, TEAQL_LOG_ENDPOINT="off",
                        PYTHONDONTWRITEBYTECODE="1"), text=True, capture_output=True, timeout=900)
                    row.update(controller=dict(command=command, exit_code=result.returncode,
                        stdout=result.stdout, stderr=result.stderr, sdk_exists=sdk.exists(), report_exists=report.exists()))
                    if not report.is_file():
                        require(result.returncode == 1 and row["original_evaluation"]["response"]["errors"]
                                and not sdk.exists(), "independently confirmed public refusal precedes worker dispatch")
                        command = command[:-1]
                        result = run_owned(command, cwd=ROOT, env=dict(os.environ, TEAQL_LOG_ENDPOINT="off",
                            PYTHONDONTWRITEBYTECODE="1"), text=True, capture_output=True, timeout=900)
                        row["service"] = dict(command=command, exit_code=result.returncode,
                            stdout=result.stdout, stderr=result.stderr)
                    row.update(sdk_exists=sdk.exists(), job=read(report) if report.is_file() else None)
                    write(path, record)
                    require(type(result.returncode) is int and result.returncode in (0, 1, 2)
                            and row["job"] is not None, "complete actual compiler-job outcome required")
                    print(row["name"], "exit", result.returncode, row["job"]["outcome"], flush=True)
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
    require(record["inputs_before"] == record["inputs_after"]
            and record["references_before"] == record["references_after"], "stable complete integer diagnosis inputs")
    for row in record["cases"]:
        job = row["job"]
        require(job["owned_directory_removed"] is True and job["publication_directory_removed"] is True
                and not Path(job["owned_directory"]).exists()
                and not Path(job["publication_directory"]).exists(), "integer compiler-job cleanup")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", required=True, type=Path)
    args = parser.parse_args()
    execute(args.record)


if __name__ == "__main__":
    main()
