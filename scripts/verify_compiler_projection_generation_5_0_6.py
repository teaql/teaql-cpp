#!/usr/bin/env python3
"""Join actual locked compiler facts to fresh C++ generation and native metadata."""

import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import signal
import sys
import tempfile

from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references
from verify_generated_privacy_metadata import LAUNCHER
from verify_primitive_const_governed_execution_5_0_6 import run_owned
import verify_rust_configuration_protocol_5_0_6 as original_gate

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
SOURCE = ROOT / "docs/parity/rust_configuration_protocol_5_0_6.json.gz"
SOURCE_SHA = "fa35e2b78fdd4120cefd80a6fd7613583cf38b703e3910017adf39227dd0ee9d"
LOCK = original_gate.LOCK
JAVA_SEED = ROOT / "docs/parity/ap02_const_function_java_candidate_v5_5_0_6.json.gz"
BUILD_SEED = original_gate.SEED
OBSERVER = ROOT / "tests/parity/compiler_projection_metadata_probe.cpp"
CPP_SOURCE = PRODUCER / "generator/src/main/java/com/skynet/codegenerator/cpp"


def require(value, message):
    if not value:
        raise RuntimeError("Compiler projection generation: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


def hashes(directory):
    return {str(path.relative_to(directory)): sha(path.read_bytes())
            for path in sorted(directory.rglob("*")) if path.is_file()}


def manifest_sha(manifest):
    return sha("".join(name + "\0" + manifest[name] + "\n"
                       for name in sorted(manifest, key=lambda s: s.encode())).encode())


def document(row):
    source = read(SOURCE)
    facts = row.get("protocol", {}).get("projections", [])
    return dict(schema="teaql.rust_configuration.generation.v1",
                generator_commit=original_gate.protocol.GENERATOR,
                runtime_commit=original_gate.protocol.RUNTIME, target=source["toolchain"]["target"],
                model_sha256=row["model_sha256"], compiler_lock_sha256=source["lock_sha256"],
                source_manifest=row["generated_hashes_before"],
                source_manifest_sha256=manifest_sha(row["generated_hashes_before"]),
                mir_sha256=row.get("mir_sha256", "0" * 64), outcome=row["outcome"],
                dependency_lock_sha256=sha(row["cargo_lock"].encode()),
                compiler_environment_sha256=manifest_sha(source["rust_environment"]),
                projections=[{key: fact[key] for key in
                              ("source_module", "owner", "relation", "cardinality", "resolved_type")}
                             for fact in facts],
                diagnostic=None if facts else dict(code="E0425", message="cannot find type DoesNotExist in the crate root"))


def native_command(include, observer, output):
    seed = read(BUILD_SEED)
    command = list(next(s["command"] for s in seed["steps"] if s["label"] == "current_cpp_compile"))
    command = [item for item in command if item != "-fsyntax-only"
               and not item.startswith("-I" + seed["owned_directory"])
               and not item.startswith(seed["owned_directory"])]
    libraries = [ROOT / "build_native_current" / package / ("lib" + package + ".a")
                 for package in ("teaql_provider_sqlite", "teaql_runtime", "teaql_sql",
                                 "teaql_data_service", "teaql_core", "teaql_memory")]
    return [command[0], "-I" + str(include), *command[1:], str(observer), "-Wl,--start-group",
            *map(str, libraries), "-Wl,--end-group",
            str(ROOT / ".tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a"),
            "-pthread", "-ldl", "-lm", "-o", str(output)]


def mips_command(include):
    entries = json.loads((ROOT / "build_mips_current/compile_commands.json").read_text())
    entry = next(s for s in entries if s["file"].endswith("/tests/parity/generated_dynamic_json_privacy_probe.cpp"))
    command = list(entry.get("arguments") or shlex.split(entry["command"]))
    for flag in ("-MD", "-MT", "-MF", "-o"):
        while flag in command:
            index = command.index(flag)
            del command[index:index + (1 if flag == "-MD" else 2)]
    command.remove("-c")
    command.remove(entry["file"])
    require(str(ROOT / ".tools/toolchains/openwrt-25.12.5") in command[0], "current locked MIPS compiler")
    return [command[0], "-I" + str(include), *command[1:], "-fsyntax-only", str(OBSERVER)]


def execute(path, check_mips):
    require(not path.exists() and sha(SOURCE.read_bytes()) == SOURCE_SHA, "fresh receipt and immutable actual compiler facts")
    source = read(SOURCE)
    original_gate.validate(source)
    original_gate.protocol.verify_toolchain(original_gate.toolchain_identity(), source["toolchain"])
    require(shutil.disk_usage(ROOT).free > 2 * 1024 ** 3, "owned output space")
    record = dict(complete=False, whole_goal_complete=False, source_receipt_sha256=SOURCE_SHA,
                  original_rust_compiler_processes=0, original_compiler_outcomes_explicitly_reused=True,
                  driver_source=Path(__file__).read_text(), observer_source=OBSERVER.read_text(),
                  input_hashes_before=capture_inputs(), references_before=references(),
                  check_mips=check_mips, steps=[], cases=[])

    def run(label, command, *, cwd=ROOT, expected=0):
        command = list(map(str, command))
        outcome = run_owned(command, cwd=cwd, env=dict(os.environ, TEAQL_LOG_ENDPOINT="off"),
                            text=True, capture_output=True, timeout=600)
        record["steps"].append(dict(label=label, command=command, cwd=str(cwd),
                                    exit_code=outcome.returncode, stdout=outcome.stdout, stderr=outcome.stderr))
        write(path, record)
        print(label, "exit", outcome.returncode, flush=True)
        require(outcome.returncode == expected, label + " unexpected process result; inspect retained stdout/stderr")
        return outcome

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_compiler_projection_generation_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                seed = read(BUILD_SEED)
                launcher = owned / "FrozenPrivacy.java"
                launcher.write_text(LAUNCHER)
                original_java = next(s["command"] for s in seed["steps"] if s["label"] == "original_fresh_java")
                original_java = [s.replace(seed["owned_directory"], directory) for s in original_java]
                run("original_javac", original_java)
                original_cp = next(s["command"][2] for s in seed["steps"] if s["label"] == "original_generate").replace(seed["owned_directory"], directory)
                java_seed = read(JAVA_SEED)
                javac = next(s["command"] for s in java_seed["steps"] if s["label"] == "javac")
                cp = javac[javac.index("-cp") + 1]
                sources = sorted({Path(s) for s in javac if s.endswith(".java") and not s.startswith("/tmp/")}
                                 | set(CPP_SOURCE.glob("*.java"))
                                 | set((PRODUCER / "generator/src/test/java/com/skynet/codegenerator/cpp").glob("*Test.java"))
                                 | {PRODUCER / ("generator/src/test/java/com/skynet/codegenerator/" + name)
                                    for name in ("CppGenerationTest.java", "CppIdentityGenerationTest.java")})
                classes = owned / "cpp_classes"
                classes.mkdir()
                run("cpp_javac", ["javac", "--release", "17", "-cp", cp, "-d", classes, *sources])
                cpp_cp = str(classes) + ":" + ":".join(str(PRODUCER / p) for p in (
                    "generator/src/main/resources", "model/src/main/resources", "generator/src/test/resources")) + ":" + cp
                tests = sorted({str(p.relative_to(PRODUCER / "generator/src/test/java")).removesuffix(".java").replace("/", ".")
                                for p in sources if "/src/test/java/" in str(p) and p.name.endswith("Test.java")})
                junit = run("junit", ["java", "-cp", cpp_cp, "org.junit.runner.JUnitCore", *tests], cwd=PRODUCER / "generator")
                matched = re.search(r"OK \((\d+) tests\)", junit.stdout)
                require(matched is not None, "terminal Java contract test outcome")
                record["java_test_count"] = int(matched[1])
                record["java_sources"] = {str(p): sha(p.read_bytes()) for p in sources}
                for prior in source["cases"]:
                    name = prior["case"]["name"]
                    work = owned / name
                    work.mkdir()
                    model = work / "model.xml"
                    model.write_text(prior["model"])
                    original = work / "original"
                    run(name + "_original_generate", ["java", "-cp", original_cp, "FrozenPrivacy", model, original])
                    regenerated = hashes(original)
                    require(regenerated == prior["generated_hashes_before"] == prior["generated_hashes_after"],
                            "all unchanged original source bytes permit explicit compiler outcome reuse")
                    payload = document(prior)
                    document_path = work / "resolution.json"
                    document_path.write_text(json.dumps(payload, sort_keys=True))
                    cpp = work / "cpp"
                    command = ["java", "-cp", cpp_cp, "com.skynet.codegenerator.cpp.CppCoreGenerator", model, cpp,
                               "--rust-configuration=" + str(document_path), "--rust-toolchain-lock=" + str(LOCK)]
                    row = dict(name=name, model=prior["model"], source_manifest=regenerated,
                               compiler_outcome_explicitly_reused=True, resolution_document=document_path.read_text())
                    record["cases"].append(row)
                    if prior["outcome"] == "compiler_refusal":
                        refusal = run(name + "_cpp_refusal", command, expected=1)
                        require("REFERENCE_COMPILER_REFUSAL" in refusal.stderr and not cpp.exists(),
                                "typed source compiler refusal before generated output")
                        row["outcome"] = "compiler_refusal"
                        continue
                    run(name + "_cpp_generate", command)
                    row["cpp_hashes"] = hashes(cpp)
                    row["cpp_models"] = (cpp / "include/cpp_inheritance_graph/models.hpp").read_text()
                    row["provenance"] = json.loads((cpp / "RUST_CONFIGURATION.json").read_text())
                    binary = work / "metadata"
                    run(name + "_native_compile", native_command(cpp / "include", OBSERVER, binary))
                    observation = run(name + "_native_execute", [binary])
                    row["descriptors"] = json.loads(observation.stdout)
                    require(row["descriptors"] == prior["descriptors"], "six actual original/native descriptor results")
                    row["outcome"] = "resolved"
                    if name == "complete":
                        unbound = work / "unbound"
                        run("unbound_generate", command[:5] + [str(unbound)])
                        syntax = native_command(unbound / "include", OBSERVER, work / "unused")
                        syntax = syntax[:syntax.index("-Wl,--start-group")]
                        failure = run("unbound_compile", [*syntax, "-fsyntax-only"], expected=1)
                        require("const fn" in failure.stderr, "original unbound C++ type expression failure retained")
                        for corrupt in ("stale_model", "missing_projection"):
                            changed = copy.deepcopy(payload)
                            if corrupt == "stale_model":
                                changed["model_sha256"] = "0" * 64
                            else:
                                changed["projections"].pop()
                            bad = work / (corrupt + ".json")
                            bad.write_text(json.dumps(changed, sort_keys=True))
                            output = work / corrupt
                            rejected = run(corrupt + "_refusal", command[:5] + [str(output), "--rust-configuration=" + str(bad),
                                "--rust-toolchain-lock=" + str(LOCK)], expected=1)
                            require(("BINDING_MISMATCH" if corrupt == "stale_model" else "MEMBERSHIP_MISMATCH") in rejected.stderr
                                    and not output.exists(), "bound CLI rejects mismatched inputs before writing output")
                        if check_mips:
                            rejected = run("mips_target_guard", mips_command(cpp / "include"), expected=1)
                            require("Rust configuration projections require their qualified x86_64 Linux target" in rejected.stderr,
                                    "actual MIPS rejection is the explicit qualified-target guard")
                    write(path, record)
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
        record.update(owned_directory_removed=not Path(record["owned_directory"]).exists(),
                      input_hashes_after=capture_inputs(), references_after=references(), complete=True)
        validate(record)
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        if "owned_directory" in record:
            record["owned_directory_removed"] = not Path(record["owned_directory"]).exists()
        write(path, record)


def validate(record):
    from compiler_source_service_source_admission_5_0_6 import current_input_matches as service_input_matches

    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["original_rust_compiler_processes"] == 0
            and record["original_compiler_outcomes_explicitly_reused"] is True, "honest completed generation scope")
    require(record["source_receipt_sha256"] == SOURCE_SHA and sha(SOURCE.read_bytes()) == SOURCE_SHA,
            "immutable actual reference compiler epoch")
    require(service_input_matches(Path(__file__), sha(record["driver_source"].encode())) and record["observer_source"] == OBSERVER.read_text(),
            "exact executed observer and generation driver")
    require(record["owned_directory_removed"] is True
            and record["input_hashes_before"] == record["input_hashes_after"]
            and record["references_before"] == record["references_after"], "closed source frame and owned cleanup")
    require(record["references_after"] == {"teaql-rs": original_gate.protocol.RUNTIME,
            "teaql_code_gen": original_gate.protocol.GENERATOR}, "exact locked reference revisions")
    require(all(service_input_matches(Path(p), value) for p, value in record["java_sources"].items()),
            "current tested Java source inputs")
    require(record["java_test_count"] >= 236 and len(record["cases"]) == 4, "all Java contracts and four source models")
    steps = {step["label"]: step for step in record["steps"]}
    require(len(steps) == len(record["steps"]), "unique actual subprocess outcomes")
    refused = {"unbound_compile", "stale_model_refusal", "missing_projection_refusal", "compiler_refusal_cpp_refusal"}
    if record["check_mips"]:
        refused.add("mips_target_guard")
    expected_steps = refused | {"original_javac", "cpp_javac", "junit", "unbound_generate", "compiler_refusal_original_generate"}
    for name in ("complete", "partial", "ignored"):
        expected_steps.update(name + suffix for suffix in ("_original_generate", "_cpp_generate", "_native_compile", "_native_execute"))
    require(set(steps) == expected_steps, "all actual generation, native compiler and execution processes")
    require(re.search(r"OK \(" + str(record["java_test_count"]) + r" tests\)", steps["junit"]["stdout"]) is not None,
            "actual Java contract count")
    for label, step in steps.items():
        require(type(step["exit_code"]) is int and step["exit_code"] == (1 if label in refused else 0),
                "actual terminal subprocess outcomes")
    originals = read(SOURCE)["cases"]
    for row, prior in zip(record["cases"], originals, strict=True):
        require(row["name"] == prior["case"]["name"] and row["model"] == prior["model"]
                and row["source_manifest"] == prior["generated_hashes_before"] == prior["generated_hashes_after"]
                and row["compiler_outcome_explicitly_reused"] is True,
                "exact complete original SDK permits prior compiler result reuse")
        require(json.loads(row["resolution_document"]) == document(prior), "exact source-derived generation document")
        if prior["outcome"] == "compiler_refusal":
            require(row["outcome"] == "compiler_refusal" and "cpp_hashes" not in row, "refusal yields no C++ SDK")
            continue
        require(row["outcome"] == "resolved" and row["descriptors"] == prior["descriptors"]
                and row["descriptors"] == json.loads(steps[row["name"] + "_native_execute"]["stdout"]),
                "six actual native descriptors match compiled Rust results")
        payload = json.loads(row["resolution_document"])
        expected = {key: payload[key] for key in ("schema", "generator_commit", "runtime_commit", "target",
                    "model_sha256", "compiler_lock_sha256", "source_manifest_sha256", "mir_sha256",
                    "dependency_lock_sha256", "compiler_environment_sha256")}
        expected["resolution_sha256"] = sha(row["resolution_document"].encode())
        require(row["provenance"] == expected and "RUST_CONFIGURATION.json" in row["cpp_hashes"], "exact generated binding provenance")
        require("<[crate::" not in row["cpp_models"] and "#error \"Rust configuration projections require" in row["cpp_models"],
                "resolved C++ model types and explicit target guard")
    require("const fn" in steps["unbound_compile"]["stderr"]
            and "BINDING_MISMATCH" in steps["stale_model_refusal"]["stderr"]
            and "MEMBERSHIP_MISMATCH" in steps["missing_projection_refusal"]["stderr"]
            and "REFERENCE_COMPILER_REFUSAL" in steps["compiler_refusal_cpp_refusal"]["stderr"],
            "specific original syntax and structured generation refusal diagnostics")
    if record["check_mips"]:
        require("Rust configuration projections require their qualified x86_64 Linux target"
                in steps["mips_target_guard"]["stderr"], "actual qualified-target MIPS refusal")


def controls(record):
    tests = ("incomplete", "missing_model", "boolean_compiler_outcome", "missing_native_compiler",
             "wrong_descriptors", "stale_model_binding", "wrong_provenance", "changed_original_source",
             "invented_rust_compilation", "wrong_reference", "unresolved_cpp_type", "missing_target_guard",
             "stale_refusal_sdk", "invented_java_count", "wrong_refusal_diagnostic", "cleanup")
    rejected = []
    for name in tests:
        value = copy.deepcopy(record)
        if name == "incomplete": value["complete"] = False
        elif name == "missing_model": value["cases"].pop()
        elif name == "boolean_compiler_outcome": value["steps"][0]["exit_code"] = False
        elif name == "missing_native_compiler": value["steps"].pop(next(i for i, s in enumerate(value["steps"]) if s["label"] == "complete_native_compile"))
        elif name == "wrong_descriptors": value["cases"][0]["descriptors"][0]["limit"] = "incorrect"
        elif name == "stale_model_binding":
            payload = json.loads(value["cases"][0]["resolution_document"]); payload["model_sha256"] = "0" * 64
            value["cases"][0]["resolution_document"] = json.dumps(payload)
        elif name == "wrong_provenance": value["cases"][0]["provenance"]["target"] = "other"
        elif name == "changed_original_source": value["cases"][0]["source_manifest"]["lib/Cargo.toml"] = "0" * 64
        elif name == "invented_rust_compilation": value["original_rust_compiler_processes"] = 4
        elif name == "wrong_reference": value["references_after"]["teaql-rs"] = "wrong"
        elif name == "unresolved_cpp_type": value["cases"][0]["cpp_models"] += "<[crate::Unknown; 2]>"
        elif name == "missing_target_guard": value["cases"][0]["cpp_models"] = value["cases"][0]["cpp_models"].replace("#error \"Rust configuration projections require", "#error \"other")
        elif name == "stale_refusal_sdk": value["cases"][-1]["cpp_hashes"] = {"models.hpp": "0" * 64}
        elif name == "invented_java_count": value["java_test_count"] += 1
        elif name == "wrong_refusal_diagnostic": next(s for s in value["steps"] if s["label"] == "stale_model_refusal")["stderr"] = "unrelated failure"
        elif name == "cleanup": value["owned_directory_removed"] = False
        try:
            validate(value)
        except RuntimeError as error:
            rejected.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted compiler-generation corruption: " + name)
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--execute", action="store_true")
    mode.add_argument("--check", action="store_true")
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--mips-guard", action="store_true")
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    if args.execute:
        def stop(signum, frame):
            raise KeyboardInterrupt("Reap this generation job's active child")
        signal.signal(signal.SIGTERM, stop)
        execute(args.record.resolve(), args.mips_guard)
    else:
        record = read(args.record)
        validate(record)
        rejected = controls(record)
        if args.controls:
            require(not args.controls.exists(), "preserve previous control receipts")
            write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize,
                                     execution_sha256=sha(args.record.read_bytes()), controls=rejected))
        print("Compiler projection generation:", len(rejected), "corruptions rejected; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
