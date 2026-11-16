#!/usr/bin/env python3
"""Inspect complete compiled SDKs at the pending column and Option-model boundaries."""

import argparse
import base64
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

from verify_compiler_generated_business_5_0_6 import execution_inputs, native_inputs, read, references, require, run_owned, write
from verify_compiler_governed_execution_5_0_6 import prepare
from verify_compiler_host_assembly_5_0_6 import HOST, WORKER, package
from verify_compiler_projection_generation_5_0_6 import hashes, native_command
from verify_compiler_source_service_5_0_6 import artifact_sha

ROOT = Path(__file__).resolve().parents[1]
OBSERVER = ROOT / "tests/parity/compiler_property_identity_probe.cpp"


def cases():
    result = []
    for row in read(ROOT / "tests/parity/field_attribute_models_5_0_6.json.gz")["cases"]:
        result.append(dict(name=row["name"], model=row["model"].replace("{namespace}", "compiled_identity_boundary"),
                           namespace="compiled_identity_boundary", boundary="pending_column_attribute"))
    result.append(dict(name="ordinary_column", model=result[0]["model"].replace("first&quot;,column=&quot;chosen", "chosen"),
                       namespace="compiled_identity_boundary", boundary="ordinary_control"))
    option = read(ROOT / "docs/parity/ap02_plain_option_model_diagnostic_5_0_6.json.gz")["cases"][0]
    result.append(dict(name="option_plain_self", model=option["model"], namespace="ap02_reverse_relation_consumer",
                       boundary="pending_option_model"))
    return result


def observer(namespace, entities):
    return ('#include <' + namespace + '.hpp>\n#include <iostream>\n' + OBSERVER.read_text()
        + '\nint main() {\n    std::pmr::set_default_resource(std::pmr::null_memory_resource());\n'
        '    teaql::GlobalContext global;\n    auto user_context = global.make_user_context();\n    {\n'
        '        auto resource = user_context->memory_resource();\n        teaql::Json::Array rows(resource);\n'
        + ''.join('        rows.emplace_back(describe_property_identities<' + namespace + '::' + row["type"] + '>(resource));\n'
                  for row in entities)
        + '        std::cout << teaql::Json(resource, std::move(rows)).dump(resource) << "\\n";\n    }\n'
        '    user_context.reset();\n    if (global.live_user_bytes() != 0) {\n'
        '        throw std::runtime_error("Compiled identity observer retains UserContext memory");\n    }\n}\n')


def execute(path, selected):
    require(not path.exists() and shutil.disk_usage(ROOT).free > 4 * 1024 ** 3, "fresh identity diagnosis and owned output space")
    record = dict(complete=False, semantic_acceptance=False, whole_goal_complete=False,
                  driver_source=Path(__file__).read_text(), observer_source=OBSERVER.read_text(),
                  preparation_steps=[], steps=[], cases=[], inputs_before=execution_inputs(),
                  references_before=references(), native_before=native_inputs(), selected_case=selected)
    owned = None

    def run(label, command, *, expected=None):
        print(label, "running", flush=True)
        result = run_owned(command, cwd=ROOT, env=dict(os.environ, PYTHONDONTWRITEBYTECODE="1", TEAQL_LOG_ENDPOINT="off"),
                           text=True, capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=list(map(str, command)), exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, "exit", result.returncode, flush=True)
        require(type(result.returncode) is int and (expected is None or result.returncode == expected), "actual identity diagnostic command outcome")
        return result

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_identity_boundaries_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                capability_path, classpath = prepare(owned, record)
                config = record["capabilities"]
                config["schema"] = "teaql.rust_configuration.job.v4"
                run("host_probe_javac", ["javac", "--release", "17", "-cp", classpath,
                    "-d", owned / "cpp_classes", HOST], expected=0)
                config["artifacts"] = {name: artifact_sha(Path(name)) for name in config["artifacts"]}
                capability_path.write_text(json.dumps(config, sort_keys=True))
                for case in cases():
                    if selected is not None and case["name"] != selected:
                        continue
                    name = case["name"]
                    current = owned / name
                    current.mkdir()
                    source, cpp, report = current / "model.json", current / "cpp", current / "job.json.gz"
                    source.write_text(json.dumps(package(case["model"]), sort_keys=True))
                    row = dict(case, source_package=package(case["model"]))
                    record["cases"].append(row)
                    result = run(name + "_host", [config["cpp_generator"]["command"][0], "-cp", classpath,
                        "CppCompilerHostProbe", source, cpp, Path(sys.executable).resolve(), WORKER,
                        capability_path, owned / "service_jobs", report])
                    row.update(host_exit_code=result.returncode, sdk_exists=cpp.exists(),
                               job=read(report) if report.is_file() else None, cpp_sources=hashes(cpp))
                    require(row["job"] is not None and result.returncode in (0, 1, 2), "actual compiler report for each complete model")
                    if row["job"]["outcome"] != "resolved":
                        continue
                    source.write_bytes(base64.b64decode(row["job"]["source_package_bytes_base64"], validate=True))
                    run(name + "_original_generate", [*config["reference_generator"]["command"], source,
                        current / "original", current / "inventory.json", "--source-package", "--semantic-storage"], expected=0)
                    row["original_sources"] = hashes(current / "original")
                    require(row["original_sources"] == row["job"]["original_sources_before"], "independent complete original SDK identity")
                    text = observer(case["namespace"], row["job"]["original_inventory"]["entities"])
                    (current / "observer.cpp").write_text(text)
                    row["native_observer"] = text
                    run(name + "_native_build", native_command(cpp / "include", current / "observer.cpp", current / "native"), expected=0)
                    row["cpp_properties"] = json.loads(run(name + "_native_run", [current / "native"], expected=0).stdout)
                    source_properties = {(entity["name"], prop["name"]): prop for entity in row["job"]["configuration"] for prop in entity["properties"]}
                    row["physical_differences"] = [dict(entity=prop["entity"], name=prop["name"],
                        original=source_properties[(prop["entity"], prop["name"])]["column"], cpp=prop["physical_column"])
                        for entity in row["cpp_properties"] for prop in entity
                        if prop["physical_column"] != source_properties[(prop["entity"], prop["name"])]["column"]]
                record["complete"] = True
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        record.update(owned_directory_removed=owned is not None and not owned.exists(), inputs_after=execution_inputs(),
                      references_after=references(), native_after=native_inputs())
        write(path, record)
    require(record["inputs_before"] == record["inputs_after"] and record["references_before"] == record["references_after"]
            and record["native_before"] == record["native_after"], "stable complete identity diagnostic inputs")
    print("Complete identity-boundary diagnosis; no compatibility decision inferred", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", required=True, type=Path)
    parser.add_argument("--case", choices=[row["name"] for row in cases()])
    args = parser.parse_args()
    execute(args.record, args.case)


if __name__ == "__main__":
    main()
