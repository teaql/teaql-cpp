#!/usr/bin/env python3
"""Carry the qualified primitive/tuple types through governed SQLite execution."""

import argparse
import ast
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import types
from xml.sax.saxutils import escape

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests/parity"))
from compare_values import same
from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references

BASE = ROOT / "scripts/verify_generated_audit_sql_sinks_5_0_6.py"
BASE_SHA256 = "bdfc09b352dcf7e8b5e066d64753a43959b8670fc2b085218907636e0a3bbf93"
SOURCE = ROOT / "docs/parity/ap02_primitive_const_path_candidate_sql_5_0_6.json.gz"
SOURCE_SHA256 = "7de30cf5c4cee81af79cbfce416a44b330b1b73c20874c762a2219b172356b70"
INSTALLED = ROOT / "docs/parity/ap02_primitive_const_path_installation_milestone_5_0_6.json.gz"
INSTALLED_SHA256 = "bbc93de761d80e648a37f7063e6fb0e6957e0a6960f70b227d03fbc481a8cf8c"
NAMES = (
    "integer_u128", "float_f32", "char_utf8", "unicode_destructure",
    "tuple_assignment_order", "tuple_compound_order_primitive", "tuple_alias_copy",
    "modeled_child",
)
MODES = (
    "success", "write_failure", "sink_failure", "policy_denied", "stale_version",
    "policy_approved", "graph_success", "graph_policy_denied", "graph_policy_approved",
    "graph_write_failure", "graph_stale_child", "graph_stale_root",
)
EXPECTED = {
    "success": (True, True, 4, 1), "write_failure": (False, False, 3, 0),
    "sink_failure": (False, True, 4, 1), "policy_denied": (False, False, 3, 0),
    "stale_version": (False, False, 4, 0), "policy_approved": (True, True, 4, 1),
    "graph_success": (True, True, 4, 2), "graph_policy_denied": (False, False, 3, 0),
    "graph_policy_approved": (True, True, 4, 2),
    "graph_write_failure": (False, False, 3, 0),
    "graph_stale_child": (False, False, 3, 0),
    "graph_stale_root": (False, False, 4, 0),
}


def require(condition, message):
    if not condition:
        raise RuntimeError("Primitive governed execution: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


def cases(source):
    require(source["complete"] is True and source["differences"] == []
            and tuple(row["name"] for row in source["cases"]) == NAMES,
            "all eight actual generated source configurations")
    relations = []
    bindings = []
    for row in source["cases"]:
        text = row["input"]
        require(text.startswith("17, reverse_relation(")
                and text.endswith(", audit_value_max_len=21"), "exact original input envelope")
        text = text[len("17, "):-len(", audit_value_max_len=21")]
        old_type = "crate::child::Child" if row["name"] == "modeled_child" else "Self"
        edits = {
            'name="links"': 'name="primitive_' + row["name"] + '"',
            'target="Child"': 'target="Enrollment"',
            'foreign_key="owner"': 'foreign_key="school",many',
            old_type: "crate::enrollment::Enrollment",
        }
        for before, after in edits.items():
            require(text.count(before) == 1, "one declared type/identity graft")
            text = text.replace(before, after, 1)
        relations.append(text)
        bindings.append({"source_name": row["name"], "source_input": row["input"],
                         "derived_relation": text,
                         "edits": [[before, after] for before, after in edits.items()]})
    prefix = "13, " + ", ".join(relations)
    suffix = ', audit_value_max_len=17, audit_mask_fields="name"'
    invalid = ', reverse_relation(name="discarded",target="Enrollment",json_type="Vec<")'
    return {
        "complete": {"name": "complete", "input": prefix + suffix,
                     "limit": "17", "masked": "PR*****...*****RY", "bindings": bindings},
        "partial": {"name": "partial", "input": prefix + invalid + suffix,
                    "limit": "13", "masked": "PR***...***RY", "bindings": bindings},
    }


class Harden(ast.NodeTransformer):
    def __init__(self):
        self.count = 0

    def visit_Assert(self, node):
        self.count += 1
        failure = ast.Raise(ast.Call(ast.Name("AssertionError", ast.Load()),
                                    [node.msg or ast.Constant("Required source check")], []), None)
        return ast.copy_location(ast.If(ast.UnaryOp(ast.Not(), node.test), [failure], []), node)


def hardened_source():
    require(sha(BASE.read_bytes()) == BASE_SHA256, "unchanged historical driver")
    transform = Harden()
    tree = ast.fix_missing_locations(transform.visit(ast.parse(BASE.read_text())))
    require(transform.count == 77 and not any(isinstance(node, ast.Assert)
                                            for node in ast.walk(tree)),
            "all historical assertions become unconditional checks")
    return ast.unparse(tree) + "\n"


def derived_model(selected):
    baseline = (ROOT / "tests/generator/inheritance_graph.xml").read_text()
    old = 'advisor="specialist(optional=true)" _audit_mask_fields="name" />'
    require(baseline.count(old) == 1, "one School declaration")
    replacement = ('advisor="specialist(optional=true)" _audit_mask_fields="name"\n'
                   '            _audit_value_max_len="'
                   + escape(selected["input"], {'"': '&quot;'}) + '" />')
    return baseline.replace(old, replacement, 1)


def run_owned(command, *, cwd=None, env=None, input=None, text=False,
              capture_output=False, timeout=None, check=False):
    require(text is True and capture_output is True, "captured text process contract")
    child = subprocess.Popen(command, cwd=cwd, env=env, text=True, stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
    try:
        out, err = child.communicate(input=input, timeout=timeout)
    except BaseException:
        try:
            os.killpg(child.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            child.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(child.pid, signal.SIGKILL)
            child.communicate()
        raise
    result = subprocess.CompletedProcess(command, child.returncode, out, err)
    if check:
        result.check_returncode()
    return result


def metadata_sources(module, directory):
    rust = module.RUST_METADATA_PROBE.read_text()
    marker = '        "declared": descriptor.audit_mask_fields_declared,'
    addition = '''
        "primitive_relations": descriptor.relations.iter()
            .filter(|r| r.name.starts_with("primitive_"))
            .map(|r| json!({"name":r.name,"target":r.target_entity,
                "local":r.local_key,"foreign":r.foreign_key,"many":r.many,
                "attach":r.attach,"delete_missing":r.delete_missing}))
            .collect::<Vec<_>>(),'''
    require(rust.count(marker) == 1, "one original descriptor observation")
    rust = rust.replace(marker, marker + addition, 1)
    cpp = module.CPP_METADATA_PROBE.read_text()
    marker = '    output.emplace("declared", Json(resource, descriptor.audit_mask_fields_declared));'
    addition = '''
    Json::Array relations(resource);
    for (const auto& relation : descriptor.relations) {
        if (relation.name.starts_with("primitive_")) {
            Json::Object row(resource);
            row.emplace("name", Json(resource, relation.name));
            row.emplace("target", Json(resource, relation.target_entity));
            row.emplace("local", Json(resource, relation.local_key));
            row.emplace("foreign", Json(resource, relation.foreign_key));
            row.emplace("many", Json(resource, relation.many));
            row.emplace("attach", Json(resource, relation.attach));
            row.emplace("delete_missing", Json(resource, relation.delete_missing));
            relations.emplace_back(Json(resource, std::move(row)));
        }
    }
    output.emplace("primitive_relations", Json(resource, std::move(relations)));'''
    require(cpp.count(marker) == 1, "one native descriptor observation")
    cpp = cpp.replace(marker, marker + addition, 1)
    rust_path, cpp_path = directory / "metadata.rs", directory / "metadata.cpp"
    rust_path.write_text(rust)
    cpp_path.write_text(cpp)
    subprocess.run([str(ROOT / ".tools/clang-format/bin/clang-format"),
                    "--style=file:" + str(ROOT / ".clang-format"), "--Werror", "-i",
                    str(cpp_path)], check=True)
    return rust_path, cpp_path


def prerequisites():
    require(sha(SOURCE.read_bytes()) == SOURCE_SHA256
            and sha(INSTALLED.read_bytes()) == INSTALLED_SHA256,
            "immutable actual compiler/runtime and production witnesses")
    installed = read(INSTALLED)
    require(installed["complete"] is True and installed["whole_goal_complete"] is False,
            "bounded qualified installation")
    selected = cases(read(SOURCE))
    hardened_source()
    return selected


def execute(case_name, record_path, sink_path):
    selected = prerequisites()[case_name]
    require(not record_path.exists() and not sink_path.exists(), "preserve execution receipts")
    before, refs = capture_inputs(), references()
    source_text = hardened_source()
    module = types.ModuleType("primitive_governed_audit_driver")
    module.__file__ = str(BASE)
    exec(compile(source_text, str(BASE), "exec"), module.__dict__)
    # Change only process ownership, input inventory and the explicitly derived model/observers.
    module.subprocess = types.SimpleNamespace(
        run=run_owned, check_output=subprocess.check_output)
    source_case = {"name": case_name, "input": selected["input"],
                   "expected": {"parsed": True, "limit": selected["limit"]}}
    corpus = {"runtime_commit": read(SOURCE)["runtime_commit"],
              "generator_commit": read(SOURCE)["generator_commit"]}
    model = derived_model(selected)

    def checked_model(name, declaration, source):
        require(name == case_name and declaration == "named" and source == SOURCE,
                "exact composed model selection")
        return corpus, source_case, model

    module.checked_model = checked_model
    module.SOURCE_CASE_EXPECTATIONS = {
        case_name: (selected["limit"], selected["masked"], True)}
    headers = sorted(path for package in ROOT.glob("teaql_*")
                     for path in (package / "include").rglob("*")
                     if path.is_file() and path.suffix in (".hpp", ".h"))
    compiler = read(ROOT / "docs/parity/ap02_primitive_const_path_installed_5_0_6.json.gz")[
        "native_compiler"]
    inventory = [Path(__file__).resolve(), SOURCE, INSTALLED, module.RUST_METADATA_PROBE,
                 module.CPP_METADATA_PROBE, ROOT / "upstream.lock.json",
                 ROOT / "toolchains.lock.json", ROOT / "build_native_current/compile_commands.json",
                 ROOT / ".reference/teaql-rs/Cargo.lock", Path(compiler["path"]),
                 ROOT / ".tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a", *headers]
    inventory_before = {str(path): sha(path.read_bytes()) for path in inventory}
    error = None
    with tempfile.TemporaryDirectory(prefix="teaql_primitive_governed_driver_") as temporary:
        directory = Path(temporary)
        module.RUST_METADATA_PROBE, module.CPP_METADATA_PROBE = metadata_sources(module, directory)
        observers = {"rust": module.RUST_METADATA_PROBE.read_text(),
                     "cpp": module.CPP_METADATA_PROBE.read_text()}
        prior_argv = sys.argv
        sys.argv = [str(BASE), "--source-case", case_name, "--source-corpus", str(SOURCE),
                    "--record", str(record_path), "--sink-record", str(sink_path)]
        try:
            require(module.main() == 0, "complete actual governed execution")
        except BaseException as caught:
            error = caught
        finally:
            sys.argv = prior_argv
            if record_path.exists():
                record = read(record_path)
                record.update(
                    whole_goal_complete=False, case=selected, model_source=model,
                    recipe_source=Path(__file__).read_text(), recipe_sha256=sha(Path(__file__).read_bytes()),
                    historical_driver_sha256=BASE_SHA256, hardened_driver_source=source_text,
                    hardened_driver_sha256=sha(source_text.encode()), assertions_hardened=77,
                    source_execution_sha256=SOURCE_SHA256, installed_milestone_sha256=INSTALLED_SHA256,
                    observer_sources=observers,
                    wrapper_directory=str(directory), native_headers=[str(path) for path in headers],
                    native_compiler=compiler, extra_inputs_before=inventory_before,
                    extra_inputs_after={str(path): sha(path.read_bytes()) for path in inventory},
                    global_inputs_before=before, global_inputs_after=capture_inputs(),
                    references_before=refs, references_after=references())
                record["complete"] = (record.get("complete") is True and error is None
                                      and record["extra_inputs_before"] == record["extra_inputs_after"]
                                      and record["global_inputs_before"] == record["global_inputs_after"]
                                      and record["references_before"] == record["references_after"])
                write(record_path, record)
        wrapper_bytes = sum(path.stat().st_size for path in directory.rglob("*") if path.is_file())
    if record_path.exists():
        record = read(record_path)
        record["wrapper_directory_removed"] = not directory.exists()
        record["wrapper_bytes_removed"] = wrapper_bytes
        record["complete"] = record["complete"] and record["wrapper_directory_removed"]
        write(record_path, record)
    if error:
        raise error
    validate(read(record_path), read(sink_path))
    print("Qualified composed primitive configuration:", case_name, flush=True)


def validate(record, sinks):
    selected = prerequisites()[record["case"]["name"]]
    current_recipe = Path(__file__).read_text()
    historical_recipe = record["recipe_source"]
    historical_sha = sha(historical_recipe.encode())
    require(record["recipe_sha256"] == historical_sha, "retained actual execution recipe")
    if historical_recipe != current_recipe:
        require(historical_sha == "a5176fcb89fc0834b9b78e4bf801631bedffb477d1ea4de412bb70c6b0bec769",
                "only the exact completed v2 recipe may reuse execution")
        trees = [ast.parse(text) for text in (historical_recipe, current_recipe)]
        for tree in trees:
            functions = [node for node in tree.body if isinstance(node, ast.FunctionDef)
                         and node.name == "validate"]
            require(len(functions) == 1, "one complete admission function")
            functions[0].body = [ast.Pass()]
        require(ast.dump(trees[0], include_attributes=False)
                == ast.dump(trees[1], include_attributes=False),
                "all execution/model/observer/control bodies remain identical")

    def current_input(name, expected):
        path = Path(name)
        if path == Path(__file__).resolve() and expected == historical_sha:
            return True
        return path.is_file() and sha(path.read_bytes()) == expected

    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["case"] == selected and record["assertions_hardened"] == 77
            and record["historical_driver_sha256"] == BASE_SHA256
            and record["source_execution_sha256"] == SOURCE_SHA256
            and record["installed_milestone_sha256"] == INSTALLED_SHA256,
            "complete exact derived production configuration")
    require(record["hardened_driver_source"] == hardened_source()
            and record["hardened_driver_sha256"] == sha(hardened_source().encode()),
            "exact unconditional execution recipe")
    require(record["inputs_unchanged"] is True and record["classpath_unchanged"] is True
            and record["input_hashes_before"] == record["input_hashes_after"]
            and record["extra_inputs_before"] == record["extra_inputs_after"]
            and record["global_inputs_before"] == record["global_inputs_after"]
            and record["references_before"] == record["references_after"],
            "complete stable compilation and global source epochs")
    for name, expected in record["input_hashes_before"].items():
        path = Path(name)
        if path.parent == Path(record["wrapper_directory"]):
            language = {"metadata.rs": "rust", "metadata.cpp": "cpp"}.get(path.name)
            require(language is not None and sha(record["observer_sources"][language].encode()) == expected,
                    "retained compiled observer bytes")
        else:
            require(current_input(name, expected),
                    "actual unchanged compilation input: " + name)
    for inventory in (record["extra_inputs_before"], record["classpath_hashes"]):
        require(all(current_input(name, expected)
                    for name, expected in inventory.items()), "actual compiler/header/library/dependency bytes")
    headers = [str(path) for path in sorted(
        path for package in ROOT.glob("teaql_*") for path in (package / "include").rglob("*")
        if path.is_file() and path.suffix in (".hpp", ".h"))]
    require(record["native_headers"] == headers
            and all(name in record["extra_inputs_before"] for name in headers)
            and sha(Path(record["native_compiler"]["path"]).read_bytes())
                == record["native_compiler"]["sha256"], "complete actual native header/compiler identity")
    require(record["owned_directory_removed"] is True
            and record["wrapper_directory_removed"] is True
            and not Path(record["owned_directory"]).exists()
            and not Path(record["wrapper_directory"]).exists(), "all owned outputs removed")
    require(record["model_source"] == derived_model(selected)
            and record["model_sha256"] == sha(record["model_source"].encode())
            and record["school_model_metadata"]["audit_limit"] == selected["input"],
            "actual generated model and complete attribute")
    steps = {row["label"]: row for row in record["steps"]}
    required = {"upstream", "generator_ledger", "original_classpath", "original_compile",
                "original_generate", "cpp_classpath", "cpp_compile", "cpp_generate",
                "rust_build", "cpp_build", "cpp_metadata_build", "cpp_save_build",
                "rust_metadata", "cpp_metadata", "rust_query", "cpp_query", "sink_matrix"}
    required.update(language + "_save_" + mode
                    for language in ("rust", "cpp") for mode in MODES)
    require(len(steps) == len(record["steps"]) == 41
            and set(steps) == required
            and all(type(row["exit_code"]) is int and row["exit_code"] == 0
                    for row in record["steps"]), "all actual process results")
    metadata = record["original_metadata"]
    require(record["metadata_match"] is True and same(metadata, record["cpp_metadata"])
            and len(metadata["descriptors"]) == 3,
            "actual complete generated descriptor equality")
    expected_relations = [{"name": "primitive_" + name, "target": "Enrollment",
                           "local": "id", "foreign": "school", "many": True,
                           "attach": True, "delete_missing": True} for name in NAMES]
    school = metadata["descriptors"][0]
    require(school["entity"] == "School" and school["limit"] == selected["limit"]
            and school["masks"] == ["name"] and school["declared"] is True
            and school["table"] == "school_data" and school["service"] == "sqlite"
            and school["name_sql_policy"] == "Masked"
            and same(school["primitive_relations"], expected_relations),
            "all eight actual typed relations and protected metadata")
    safe = {"name": "name", "value": selected["masked"], "masked": True,
            "truncated": True, "raw_length": 21, "output_length": int(selected["limit"]),
            "mask_reason": "_audit_mask_fields", "truncate_reason": "_audit_value_max_len"}
    require(metadata["safe_audit"] == [safe], "actual masked and truncated audit field")
    require(record["save_match"] is True and record["save_cases"] == 12
            and set(record["original_save"]) == set(record["cpp_save"]) == set(MODES)
            and same(record["original_save"], record["cpp_save"]), "all actual governed Save pairs")
    for mode, (ok, committed, version, events) in EXPECTED.items():
        row = record["original_save"][mode]
        require(type(row["save_ok"]) is bool and row["save_ok"] is ok
                and type(row["version"]) is int and type(row["event_count"]) is int
                and row["db_name"] == ("PRIVATE-SCHOOL-CANARY" if committed else "school")
                and row["version"] == version and row["event_count"] == events
                and same(row["safe_field"], safe if events else None),
                "authoritative storage, commit and safe event: " + mode)
        require((row["enrollment_note"], row["enrollment_version"]) == (
            ("PRIVATE-NOTE-CANARY", 4) if mode in ("graph_success", "graph_policy_approved")
            else ("student", 4) if mode == "graph_stale_child" else ("student", 3)),
            "actual child write or rollback: " + mode)
        require(row["policy_operation_counts"] == (
            [2] if mode in ("graph_policy_denied", "graph_policy_approved")
            else [1] if mode in ("policy_denied", "policy_approved") else []),
            "whole immutable policy plan: " + mode)
        for language, field in (("rust", "original_save"), ("cpp", "cpp_save")):
            require(same(json.loads(steps[language + "_save_" + mode]["stdout"]),
                         record[field][mode]), "actual Save stdout")
    require(record["query_match"] is True and same(record["original_query"], record["cpp_query"])
            and record["original_query"]["rows"] == [[7, 3, None]]
            and record["original_query"]["error"] is None
            and "PRIVATE-SCHOOL-CANARY" not in json.dumps(record["original_query"]["sql"])
            and "MASKED; NOT REPLAYABLE" in json.dumps(record["original_query"]["sql"]),
            "actual SQLite query and safe SQL diagnostics")
    for language, field in (("rust", "original_metadata"), ("cpp", "cpp_metadata"),
                            ("rust", "original_query"), ("cpp", "cpp_query")):
        kind = "metadata" if field.endswith("metadata") else "query"
        require(same(json.loads(steps[language + "_" + kind]["stdout"]), record[field]),
                "actual descriptor/query stdout")
    require(record["sink_cases"] == 12 and sinks["owned_directory_removed"] is True
            and len(sinks["observations"]) == 12,
            "complete actual file publication selection")
    sink_path = Path(record["sink_receipt"]["path"])
    require(sink_path.is_file() and sha(sink_path.read_bytes()) == record["sink_receipt"]["sha256"],
            "immutable actual file publication receipt")
    expected = {(language, name) for language in ("rust", "cpp") for name in (
        "unset", "invalid", "enabled", "no_debug_sink", "endpoint_off", "logging_disabled")}
    require({(row["language"], row["case"]) for row in sinks["observations"]} == expected,
            "exact distinct file publication modes")
    for row in sinks["observations"]:
        require(type(row["exit_code"]) is int and row["exit_code"] == 0
                and "PRIVATE-SCHOOL-CANARY" not in row["ordinary"]
                and ("PRIVATE-SCHOOL-CANARY" in row["debug"]) is (row["case"] == "enabled")
                and row["authorized_debug"] is (row["case"] == "enabled"),
                "exact ordinary/debug plaintext gate")


def controls(record, sinks):
    tests = (
        ("incomplete", lambda r, s: r.update(complete=False)),
        ("wrong_limit", lambda r, s: r["original_metadata"]["descriptors"][0].update(limit="21")),
        ("missing_type", lambda r, s: r["original_metadata"]["descriptors"][0]["primitive_relations"].pop()),
        ("wrong_source", lambda r, s: r["case"]["bindings"].pop()),
        ("changed_program", lambda r, s: r.update(hardened_driver_source="pass\n")),
        ("boolean_exit", lambda r, s: r["steps"][0].update(exit_code=False)),
        ("fabricated_row", lambda r, s: r["original_save"]["success"].update(version=3)),
        ("rolled_back_child", lambda r, s: r["original_save"]["graph_success"].update(enrollment_note="student")),
        ("missing_policy", lambda r, s: r["original_save"]["graph_policy_approved"].update(policy_operation_counts=[1])),
        ("missing_sink", lambda r, s: s["observations"].pop()),
        ("unsafe_sink", lambda r, s: s["observations"][0].update(ordinary="PRIVATE-SCHOOL-CANARY")),
        ("invalid_permission", lambda r, s: s["observations"][0].update(debug="PRIVATE-SCHOOL-CANARY")),
        ("changed_header", lambda r, s: (
            r["extra_inputs_before"].update({r["native_headers"][0]: "0" * 64}),
            r["extra_inputs_after"].update({r["native_headers"][0]: "0" * 64}))),
        ("missing_header", lambda r, s: r["native_headers"].pop()),
        ("changed_model", lambda r, s: r.update(model_source=r["model_source"] + "\n")),
        ("missing_process", lambda r, s: r["steps"][0].update(label="invented")),
    )
    result = []
    for name, mutate in tests:
        a, b = copy.deepcopy(record), copy.deepcopy(sinks)
        mutate(a, b)
        try:
            validate(a, b)
        except RuntimeError as error:
            result.append({"case": name, "rejected": True, "error": str(error)})
        else:
            raise RuntimeError("Accepted governed execution corruption: " + name)
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
        execute(args.case, args.record.resolve(), args.sink_record.resolve())
    else:
        record, sinks = read(args.record), read(args.sink_record)
        require(record["case"]["name"] == args.case, "exact selected receipt")
        validate(record, sinks)
        rejected = controls(record, sinks)
        if args.controls:
            require(not args.controls.exists(), "preserve rejection controls")
            write(args.controls, {"complete": True, "whole_goal_complete": False,
                                  "optimized": sys.flags.optimize, "controls": rejected,
                                  "execution_sha256": sha(args.record.read_bytes()),
                                  "sinks_sha256": sha(args.sink_record.read_bytes()),
                                  "recipe_sha256": sha(Path(__file__).read_bytes())})
        print("Verified governed execution and", len(rejected),
              "corruptions; optimized=", sys.flags.optimize)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
