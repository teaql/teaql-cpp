#!/usr/bin/env python3
"""Compile unchanged locked Rust domains and qualify the MIR projection protocol."""

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
import subprocess
import sys
import tempfile
from xml.sax.saxutils import escape

import rust_configuration_protocol as protocol
from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references
from verify_generated_privacy_metadata import LAUNCHER
from verify_primitive_const_governed_execution_5_0_6 import run_owned

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / ".reference/teaql-rs"
ORIGINAL = ROOT / ".reference/teaql_code_gen"
LOCK = ROOT / "tests/parity/rust_configuration_protocol_5_0_6_toolchain.json"
SEED = ROOT / "docs/parity/ap02_generated_rust_mir_resolution_probe_5_0_6.json.gz"
SEED_SHA = "caecf2ea2dc53a7cbc6da1d3a25365f4686cd49e7bbeba12b0b9c385d26909c6"
ENTITIES = dict(platform="Platform", specialist="Specialist", school="School",
                academy="Academy", research_academy="ResearchAcademy", enrollment="Enrollment")
BASE = [
    ("Platform", "specialist_list", "list", "specialist::entity::Specialist"),
    ("Platform", "school_list", "list", "school::entity::School"),
    ("Specialist", "platform", "option", "platform::entity::Platform"),
    ("Specialist", "school_list", "list", "school::entity::School"),
    ("School", "platform", "option", "platform::entity::Platform"),
    ("School", "advisor", "option", "specialist::entity::Specialist"),
    ("School", "enrollment_list", "list", "enrollment::entity::Enrollment"),
    ("Academy", "platform", "option", "platform::entity::Platform"),
    ("Academy", "advisor", "option", "specialist::entity::Specialist"),
    ("ResearchAcademy", "platform", "option", "platform::entity::Platform"),
    ("ResearchAcademy", "advisor", "option", "specialist::entity::Specialist"),
    ("Enrollment", "school", "option", "school::entity::School"),
]
FUNCTION_POINTER = (
    "{ const fn f() -> usize { 2 } const fn g() -> usize { 3 } "
    "let h = if true { f } else { g }; let _ = h; 2usize }"
)


def require(value, message):
    protocol.require(value, "Rust configuration qualification: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


def command_output(command):
    return subprocess.check_output(command, text=True).strip()


def toolchain_identity():
    rustc = Path(command_output(["rustup", "which", "rustc"])).resolve()
    cargo = Path(command_output(["rustup", "which", "cargo"])).resolve()
    sysroot = Path(command_output([str(rustc), "--print", "sysroot"])).resolve()
    target = "x86_64-unknown-linux-gnu"
    files = {rustc, cargo}
    files.update(path.resolve() for path in (sysroot / "lib").glob("*.so*"))
    files.update(path.resolve() for path in (sysroot / "lib/rustlib" / target / "lib").rglob("*")
                 if path.is_file())
    return dict(host=target, target=target, rustc_version=command_output([str(rustc), "-vV"]),
                cargo_version=command_output([str(cargo), "-V"]),
                target_cfg=sorted(command_output([str(rustc), "--print", "cfg", "--target", target]).splitlines()),
                rustc=str(rustc), cargo=str(cargo), sysroot=str(sysroot),
                files={str(path.relative_to(sysroot)): sha(path.read_bytes()) for path in sorted(files)})


def relation(name, rust_type=None, many=True, target="Enrollment"):
    result = ('reverse_relation(name="' + name + '",target="' + target
              + '",foreign_key="school"' + (",many" if many else ""))
    return result + (',json_type="' + rust_type + '"' if rust_type is not None else "") + ")"


def cases():
    pointer = "<[crate::enrollment::Enrollment; " + FUNCTION_POINTER + "] as IntoIterator>::Item"
    school = [relation("protocol_pointer_many", pointer),
              relation("protocol_pointer_one", pointer, False),
              relation("protocol_alias_many", "<std::vec::Vec<crate::Enrollment> as IntoIterator>::Item"),
              relation("protocol_qualified_one",
                       "<::std::vec::Vec<crate::enrollment::Enrollment> as ::core::iter::IntoIterator>::Item", False),
              relation("protocol_omitted")]
    descendants = {
        "academy": "13, " + relation("protocol_inherited_many", "<Vec<Self> as IntoIterator>::Item", target="Academy"),
        "research_academy": "13, " + relation("protocol_inherited_one", "<Vec<Self> as IntoIterator>::Item", False,
                                             "ResearchAcademy"),
    }
    extra = [("School", "protocol_pointer_many", "list", "enrollment::entity::Enrollment"),
             ("School", "protocol_pointer_one", "option", "enrollment::entity::Enrollment"),
             ("School", "protocol_alias_many", "list", "enrollment::entity::Enrollment"),
             ("School", "protocol_qualified_one", "option", "enrollment::entity::Enrollment")]
    inherited = [("Academy", "protocol_inherited_many", "list", "academy::entity::Academy"),
                 ("ResearchAcademy", "protocol_inherited_one", "option", "research_academy::entity::ResearchAcademy")]
    complete = "13, " + ", ".join(school)
    partial = complete + ", " + relation("protocol_invalid", "Vec<")
    ignored = "13, " + ", ".join(school[:2]) + ", unknown={ let x = DoesNotExist; x }"
    tail = ", " + relation("protocol_after", "crate::Enrollment") + ", audit_value_max_len=17"
    result = []
    for name, declaration, selected, limit in (
        ("complete", complete, extra, "17"),
        ("partial", partial, extra, "13"),
        ("ignored", ignored, extra[:2], "13"),
    ):
        absent = [("School", "protocol_omitted"), ("School", "protocol_invalid")]
        if name == "complete":
            selected = selected + [("School", "protocol_after", "list", "enrollment::entity::Enrollment")]
        else:
            absent.append(("School", "protocol_after"))
        if name == "ignored":
            absent += [("School", "protocol_alias_many"), ("School", "protocol_qualified_one")]
        result.append(dict(name=name, attributes=dict(descendants, school=declaration + tail),
                           expected=BASE + selected + inherited, absent=absent,
                           school_limit=limit, compiler_exit=0))
    refused = copy.deepcopy(result[0])
    refused.update(name="compiler_refusal", expected=[], compiler_exit=101)
    refused["attributes"]["school"] = refused["attributes"]["school"].replace(
        "crate::enrollment::Enrollment; " + FUNCTION_POINTER, "crate::DoesNotExist; " + FUNCTION_POINTER, 1)
    result.append(refused)
    return result


def model_source(case):
    text = (ROOT / "tests/generator/inheritance_graph.xml").read_text()
    for entity, declaration in case["attributes"].items():
        pattern = r"(<" + entity + r"\b[^>]*?)(\s*/>)"
        require(len(re.findall(pattern, text)) == 1, "one fixture entity declaration")
        encoded = escape(declaration, {'"': "&quot;"})
        text = re.sub(pattern, lambda match: match[1] + ' _audit_value_max_len="' + encoded + '"' + match[2], text, count=1)
    return text


def hashes(directory):
    return {str(path.relative_to(directory)): sha(path.read_bytes())
            for path in sorted(directory.rglob("*")) if path.is_file()}


def execute(path):
    require(not path.exists(), "preserve earlier receipts")
    seed = read(SEED)
    require(sha(SEED.read_bytes()) == SEED_SHA, "immutable completed experiment")
    toolchain = toolchain_identity()
    protocol.verify_toolchain(toolchain, json.loads(LOCK.read_text()))
    require(shutil.disk_usage(ROOT).free > 2 * 1024 ** 3, "available owned output space")
    before, refs = capture_inputs(), references()
    require(refs == {"teaql-rs": protocol.RUNTIME, "teaql_code_gen": protocol.GENERATOR}, "exact locked source revisions")
    record = dict(complete=False, whole_goal_complete=False, production_installed=False,
                  schema=protocol.SCHEMA, toolchain=toolchain, lock_sha256=sha(LOCK.read_bytes()),
                  input_hashes_before=before, references_before=refs, steps=[], cases=[],
                  protocol_source=(ROOT / "scripts/rust_configuration_protocol.py").read_text(),
                  driver_source=Path(__file__).read_text(), seed_sha256=SEED_SHA)

    def run(label, command, *, env=None, cwd=None):
        actual = list(map(str, command))
        outcome = run_owned(actual, cwd=cwd, env=env, text=True, capture_output=True, timeout=600)
        record["steps"].append(dict(label=label, command=actual, cwd=str(cwd) if cwd else None,
                                    exit_code=outcome.returncode, stdout=outcome.stdout, stderr=outcome.stderr))
        write(path, record)
        print(label, "exit", outcome.returncode, flush=True)
        return outcome

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_rust_configuration_protocol_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                launcher = owned / "FrozenPrivacy.java"
                launcher.write_text(LAUNCHER)
                old = seed["owned_directory"]
                java = next(step["command"] for step in seed["steps"] if step["label"] == "original_fresh_java")
                java = [item.replace(old, directory) for item in java]
                require(run("locked_original_javac", java).returncode == 0, "fresh original generator compilation")
                generate = next(step["command"] for step in seed["steps"] if step["label"] == "original_generate")
                classpath = generate[2].replace(old, directory)
                env = dict(os.environ, CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                           CARGO_TARGET_DIR=str(owned / "target"), RUSTC=toolchain["rustc"],
                           CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=os.environ["CC"])
                for key in ("RUSTFLAGS", "CARGO_ENCODED_RUSTFLAGS", "RUSTC_WRAPPER",
                            "RUSTC_WORKSPACE_WRAPPER", "RUSTC_BOOTSTRAP", "CARGO_BUILD_RUSTC"):
                    require(not env.get(key), "unqualified compiler override " + key)
                    env.pop(key, None)
                record["rust_environment"] = {key: env[key] for key in (
                    "CARGO_INCREMENTAL", "CARGO_PROFILE_DEV_DEBUG", "CARGO_TARGET_DIR", "RUSTC",
                    "CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER")}
                for case in cases():
                    work = owned / case["name"]
                    work.mkdir()
                    model, generated = work / "model.xml", work / "original_generated"
                    source = model_source(case)
                    model.write_text(source)
                    row = dict(case=case, model=source, model_sha256=sha(source.encode()))
                    record["cases"].append(row)
                    require(run(case["name"] + "_generate", ["java", "-cp", classpath,
                                "FrozenPrivacy", model, generated]).returncode == 0, "complete original generation")
                    row["generated_hashes_before"] = hashes(generated)
                    row["java_metadata"] = json.loads((generated / "metadata.json").read_text())
                    require({item["entity"] for item in row["java_metadata"]} == set(ENTITIES.values()),
                            "complete generated entity inventory")
                    harness = work / "harness"
                    harness.mkdir()
                    manifest = ("[package]\nname=\"teaql_configuration_protocol_probe\"\nversion=\"0.0.0\"\n"
                                "edition=\"2024\"\n[workspace]\n[dependencies]\n"
                                'domain={package="cpp-inheritance-graph-core",path="' + str(generated / "lib") + '"}\n'
                                'teaql-core={path="' + str(REFERENCE / "teaql-core") + '"}\nserde_json="1"\n'
                                "[patch.crates-io]\n" + "".join(package + '={path="' + str(REFERENCE / package) + '"}\n'
                                for package in ("teaql-core", "teaql-macros", "teaql-runtime", "teaql-data-service",
                                                "teaql-sql", "teaql-provider-sqlite")))
                    (harness / "Cargo.toml").write_text(manifest)
                    (harness / "src").mkdir()
                    observer = ('use teaql_core::TeaqlEntity;\nuse serde_json::json;\nfn main() {\n'
                                'let values = [' + ",".join('domain::' + name + '::entity_descriptor()'
                                for name in ENTITIES.values()) + '];\nprintln!("{}", serde_json::Value::Array(values.into_iter()'
                                '.map(|d| json!({"owner":d.name,"limit":d.audit_value_max_len.map(|v|v.to_string()),'
                                '"relations":d.relations.into_iter().map(|r|json!({"name":r.name,"target":r.target_entity,'
                                '"many":r.many})).collect::<Vec<_>>()})).collect()));\n}\n')
                    (harness / "src/main.rs").write_text(observer)
                    (harness / "Cargo.lock").write_bytes((REFERENCE / "Cargo.lock").read_bytes())
                    cargo = [toolchain["cargo"]]
                    require(run(case["name"] + "_resolve", cargo + ["metadata", "--offline", "--format-version", "1",
                                "--manifest-path", harness / "Cargo.toml"], env=env).returncode == 0, "locked dependency resolution")
                    row.update(manifest=manifest, observer_source=observer,
                               cargo_lock=(harness / "Cargo.lock").read_text())
                    compilation = run(case["name"] + "_compile", cargo + ["rustc", "--offline", "--locked", "-v",
                        "--manifest-path", harness / "Cargo.toml", "--target", toolchain["target"],
                        "-p", "cpp-inheritance-graph-core", "--lib", "--", "--emit=mir"], env=env)
                    require(compilation.returncode == case["compiler_exit"], "original compiler outcome")
                    row["outcome"] = "resolved" if compilation.returncode == 0 else "compiler_refusal"
                    if compilation.returncode == 0:
                        invocations = [shlex.split(match[1]) for match in re.finditer(r"Running `([^\n]+)`", compilation.stderr)
                                       if "--crate-name cpp_inheritance_graph_core " in match[1]]
                        require(len(invocations) == 1, "one actual original-domain compiler invocation")
                        invocation = invocations[0]
                        require(invocation[0] == toolchain["rustc"] and "--emit=mir" in invocation,
                                "locked compiler emitted the original domain MIR")
                        suffixes = [item.removeprefix("extra-filename=") for item in invocation if item.startswith("extra-filename=")]
                        require(len(suffixes) == 1, "exact original compiler output identity")
                        mir_path = Path(invocation[invocation.index("--out-dir") + 1]) / ("cpp_inheritance_graph_core" + suffixes[0] + ".mir")
                        mir = mir_path.read_text()
                        sources = {module + "::entity": dict(owner=name, path=str(generated / "lib/src" / module / "entity.rs"),
                                   getter_methods=[item[1] for item in BASE if item[0] == name and item[2] == "option"])
                                   for module, name in ENTITIES.items()}
                        row.update(mir=mir, mir_sha256=sha(mir.encode()), entity_sources=sources,
                                   compiler_invocation=invocation)
                        result = protocol.projection_facts(mir, sources)
                        protocol.verify_membership(result, case["expected"], case["absent"])
                        observation = run(case["name"] + "_descriptors", cargo + ["run", "--offline", "--locked", "--quiet",
                            "--manifest-path", harness / "Cargo.toml", "--target", toolchain["target"]], env=env)
                        require(observation.returncode == 0, "actual original descriptor execution")
                        row.update(protocol=result, descriptors=json.loads(observation.stdout))
                    else:
                        require("error[E0425]: cannot find type `DoesNotExist` in the crate root" in compilation.stderr,
                                "reference refused the actually used missing Rust type")
                    row["generated_hashes_after"] = hashes(generated)
                    require(row["generated_hashes_before"] == row["generated_hashes_after"],
                            "compile original generated sources unchanged")
                    write(path, record)
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
        record["owned_directory_removed"] = not Path(record["owned_directory"]).exists()
        record["input_hashes_after"], record["references_after"] = capture_inputs(), references()
        require(record["input_hashes_before"] == record["input_hashes_after"]
                and record["references_before"] == record["references_after"], "stable source inputs")
        record["complete"] = True
        validate(record)
    except BaseException as error:
        record["complete"] = False
        record["error"] = repr(error)
        raise
    finally:
        if "owned_directory" in record:
            record["owned_directory_removed"] = not Path(record["owned_directory"]).exists()
        write(path, record)


def validate(record):
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["production_installed"] is False, "bounded completed prototype")
    require(record["protocol_source"] == (ROOT / "scripts/rust_configuration_protocol.py").read_text()
            and record["driver_source"] == Path(__file__).read_text(), "exact protocol and execution sources")
    require(record["lock_sha256"] == sha(LOCK.read_bytes()) and record["seed_sha256"] == SEED_SHA,
            "pinned toolchain lock and original experiment")
    protocol.verify_toolchain(record["toolchain"], json.loads(LOCK.read_text()))
    require(record["owned_directory_removed"] is True and record["input_hashes_before"] == record["input_hashes_after"]
            and record["references_before"] == record["references_after"], "source stability and owned cleanup")
    require(record["references_after"] == {"teaql-rs": protocol.RUNTIME, "teaql_code_gen": protocol.GENERATOR},
            "exact locked reference revisions")
    require(len(record["cases"]) == 4
            and json.loads(json.dumps([row["case"] for row in record["cases"]])) == json.loads(json.dumps(cases())),
            "all independent fixture contracts retained")
    steps = {step["label"]: step for step in record["steps"]}
    require(len(steps) == len(record["steps"]) == 16, "all actual generation, compilation, and execution processes")
    for label, step in steps.items():
        require(type(step["exit_code"]) is int and step["exit_code"] == (101 if label == "compiler_refusal_compile" else 0),
                "actual compiler outcomes, without boolean coercion")
    for row in record["cases"]:
        case = row["case"]
        require(row["model"] == model_source(case) and row["model_sha256"] == sha(row["model"].encode()),
                "exact original model")
        require(row["generated_hashes_before"] == row["generated_hashes_after"]
                and row["generated_hashes_before"], "unchanged generated original domain")
        if case["compiler_exit"] != 0:
            require(row["outcome"] == "compiler_refusal" and "protocol" not in row and "mir" not in row,
                    "compiler refusal cannot yield stale projection facts")
            require("error[E0425]: cannot find type `DoesNotExist` in the crate root"
                    in steps[case["name"] + "_compile"]["stderr"],
                    "actual missing-type diagnostic, rather than a toolchain failure")
            continue
        require(row["outcome"] == "resolved" and row["mir_sha256"] == sha(row["mir"].encode()), "actual completed MIR")
        result = protocol.projection_facts(row["mir"], row["entity_sources"])
        require(result == row["protocol"], "complete compiler facts and operand provenance")
        protocol.verify_membership(result, case["expected"], case["absent"])
        require(len(row["descriptors"]) == 6 and {d["owner"] for d in row["descriptors"]} == set(ENTITIES.values()),
                "six actual compiled entity descriptors")
        require(row["descriptors"] == json.loads(steps[case["name"] + "_descriptors"]["stdout"]),
                "descriptor observations came from the actual original executable")
        school = next(item for item in row["descriptors"] if item["owner"] == "School")
        require(school["limit"] == case["school_limit"], "actual complete/partial/ignored metadata interpretation")
        names = {item["name"] for item in school["relations"]}
        require(("protocol_omitted" in names) == (case["name"] != "ignored")
                and "protocol_invalid" not in names and ("protocol_after" in names) == (case["name"] == "complete"),
                "omitted projection retains metadata, ignored tail does not")


def controls(record):
    tests = ("format_header", "unknown_method", "call_syntax", "missing_projection", "wrong_type",
             "wrong_cardinality", "wrong_owner", "missing_relation_constant", "clobbered_relation_constant",
             "cross_block_constant", "duplicate_projection", "source_path", "unknown_entity_module",
             "unknown_getter", "missing_getter", "duplicate_getter", "getter_cardinality", "wrong_descriptor",
             "stale_compiler_refusal", "boolean_exit", "compiler_identity", "target_identity", "cleanup")
    rejected = []
    for name in tests:
        value = copy.deepcopy(record)
        row = value["cases"][0]
        selected = next(f for f in row["protocol"]["projections"] if f["relation"] == "protocol_pointer_many")
        call = selected["call"]
        if name == "format_header": row["mir"] = row["mir"].replace("WARNING", "NEW_FORMAT", 1)
        elif name == "unknown_method": row["mir"] = row["mir"].replace("::borrowed_json::{closure#0}", "::unknown::{closure#0}")
        elif name == "call_syntax": row["mir"] = row["mir"].replace(call, call.replace("resolve_relation_list", "resolve_relation_unknown"), 1)
        elif name == "missing_projection": row["mir"] = row["mir"].replace(call, "", 1)
        elif name == "wrong_type": row["mir"] = row["mir"].replace(call, call.replace("enrollment::entity::Enrollment", "school::entity::School"), 1)
        elif name == "wrong_cardinality": row["mir"] = row["mir"].replace(call, call.replace("resolve_relation_list", "resolve_relation_option"), 1)
        elif name == "wrong_owner": row["mir"] = row["mir"].replace('= const "School";', '= const "Academy";')
        elif name == "missing_relation_constant": row["mir"] = re.sub(r'(_\d+) = const "protocol_pointer_many";', r'\1 = const ();', row["mir"])
        elif name in ("clobbered_relation_constant", "cross_block_constant"):
            local = re.search(r'(?:move|copy) (_\d+)\) ->', call)[1]
            prefix = local + " = const ();\n" if name == "clobbered_relation_constant" else "bb999999: {\n"
            row["mir"] = row["mir"].replace(call, prefix + call, 1)
        elif name == "duplicate_projection": row["mir"] = row["mir"].replace(call, call + "\n" + call, 1)
        elif name == "source_path": row["entity_sources"]["school::entity"]["path"] += ".wrong"
        elif name == "unknown_entity_module": row["entity_sources"].pop("school::entity")
        elif name == "unknown_getter":
            getter = row["protocol"]["other_relation_calls"][0]
            row["mir"] = row["mir"].replace(getter["function"], getter["function"].replace(
                "::" + getter["getter_method"] + "::{closure#0}", "::unknown::{closure#0}"), 1)
        elif name in ("missing_getter", "duplicate_getter", "getter_cardinality"):
            getter_call = row["protocol"]["other_relation_calls"][0]["call"]
            replacement = ("" if name == "missing_getter" else getter_call + "\n" + getter_call
                           if name == "duplicate_getter" else getter_call.replace("resolve_relation_option", "resolve_relation_list"))
            row["mir"] = row["mir"].replace(getter_call, replacement, 1)
        elif name == "wrong_descriptor": row["descriptors"][0]["limit"] = "unobserved"
        elif name == "stale_compiler_refusal": value["cases"][-1]["protocol"] = row["protocol"]
        elif name == "boolean_exit": value["steps"][0]["exit_code"] = False
        elif name == "compiler_identity": value["toolchain"]["rustc_version"] += "-unqualified"
        elif name == "target_identity": value["toolchain"]["target"] = "mips-unknown-linux-musl"
        elif name == "cleanup": value["owned_directory_removed"] = False
        if "mir" in row:
            row["mir_sha256"] = sha(row["mir"].encode())
        if name in ("missing_projection", "wrong_type", "wrong_cardinality"):
            row["protocol"] = protocol.projection_facts(row["mir"], row["entity_sources"])
        try:
            validate(value)
        except protocol.ProtocolError as error:
            rejected.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise protocol.ProtocolError("Accepted protocol corruption: " + name)
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--execute", action="store_true")
    mode.add_argument("--check", action="store_true")
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    if args.execute:
        def stop(signum, frame):
            raise KeyboardInterrupt("Reap only this prototype's active child")
        signal.signal(signal.SIGTERM, stop)
        execute(args.record.resolve())
    else:
        record = read(args.record)
        validate(record)
        rejected = controls(record)
        if args.controls:
            require(not args.controls.exists(), "preserve prior controls")
            write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize,
                                     execution_sha256=sha(args.record.read_bytes()), controls=rejected))
        print("Rust configuration protocol:", len(rejected), "corruptions rejected; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
