#!/usr/bin/env python3
"""Exercise registered custom comparators with real, then incomplete probe output.

The unmodified comparator must first accept its real native observations through
the recording wrapper. Replays then remove or duplicate one complete record, or
remove one JSON member while retaining record cardinality. This qualifies only
the exercised result boundary, not oracle provenance or all normalization rules.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / "tests/parity"
MODES = ("drop_record", "duplicate_record", "drop_member", "bool_to_integer", "integer_to_float")


def digest(data):
    return hashlib.sha256(data).hexdigest()


def mutate(output, mode, prefix):
    rows = output.split(b"\n")
    trailing = rows[-1] == b""
    if trailing:
        rows.pop()
    eligible = [i for i, row in enumerate(rows)
                if row.startswith(prefix.encode())] if prefix else list(range(len(rows)))
    # Progress markers and termination stderr have their own invariants. They
    # are not JSON result records and need not reject duplicate progress text.
    try:
        values = {index: json.loads(rows[index][len(prefix):]) for index in eligible}
    except (ValueError, UnicodeDecodeError):
        return output, None
    effect = None
    if mode == "drop_record" and len(eligible) > 1:
        removed = rows.pop(eligible[-1])
        effect = dict(kind=mode, removed_sha256=digest(removed))
    elif mode == "duplicate_record" and eligible:
        rows.insert(eligible[-1] + 1, rows[eligible[0]])
        effect = dict(kind=mode, duplicated_sha256=digest(rows[eligible[0]]))
    elif mode == "drop_member":
        for index in eligible:
            try:
                value = json.loads(rows[index][len(prefix):])
            except (ValueError, UnicodeDecodeError):
                continue
            if isinstance(value, dict) and value:
                key = next(iter(value))
                del value[key]
                effect = dict(kind=mode, removed_key=key, record=index)
            elif isinstance(value, list) and value:
                value.pop()
                effect = dict(kind="drop_array_element", record=index)
            else:
                continue
            rows[index] = prefix.encode() + json.dumps(value, ensure_ascii=True).encode()
            break
    elif mode in ("bool_to_integer", "integer_to_float"):
        changed = 0

        def alter(value):
            nonlocal changed
            if isinstance(value, bool) and mode == "bool_to_integer":
                changed += 1
                return int(value)
            if type(value) is int and mode == "integer_to_float" and abs(value) <= 2 ** 53:
                changed += 1
                return float(value)
            if isinstance(value, list):
                return [alter(item) for item in value]
            if isinstance(value, dict):
                return {key: alter(item) for key, item in value.items()}
            return value

        for index, value in values.items():
            rows[index] = prefix.encode() + json.dumps(alter(value), ensure_ascii=True).encode()
        if changed:
            effect = dict(kind=mode, changed_values=changed)
    if effect is None:
        return output, None
    return b"\n".join(rows) + (b"\n" if trailing else b""), effect


def probe(config_path, arguments):
    config = json.loads(config_path.read_text())
    data = sys.stdin.buffer.read()
    environment = dict(os.environ)
    environment.pop("TEAQL_PARITY_DIAGNOSTIC_DIR", None)
    mode = json.loads(Path(config["mode_file"]).read_text())
    command = [config["native"], *arguments]
    key = digest(json.dumps(dict(command=command, environment=environment, cwd=os.getcwd(),
                                 input_sha256=digest(data)), sort_keys=True).encode())
    cache = Path(config["cache"]) / key
    cached = cache.with_suffix(".json").exists()
    if cached:
        result = json.loads(cache.with_suffix(".json").read_text())
        output = cache.with_suffix(".stdout").read_bytes()
        stderr = cache.with_suffix(".stderr").read_bytes()
        exit_code = result["exit_code"]
    else:
        result = subprocess.run(command, input=data, capture_output=True, env=environment)
        output, stderr, exit_code = result.stdout, result.stderr, result.returncode
        cache.with_suffix(".stdout").write_bytes(output)
        cache.with_suffix(".stderr").write_bytes(stderr)
        cache.with_suffix(".json").write_text(json.dumps(dict(exit_code=exit_code)))
    original = output
    effect = None
    # Formatter width/count admission are prerequisites, not formatter results.
    prerequisite = "--pointer-bits" in arguments or "--validate-counts" in arguments
    if exit_code == 0 and not prerequisite:
        output, effect = mutate(output, mode, config.get("prefix", ""))
    call = dict(native=config["native"], native_sha256=config["native_sha256"],
                arguments=arguments, input_sha256=digest(data), input_bytes=len(data),
                cache_key=key, cached=cached, native_exit_code=exit_code,
                original_sha256=digest(original), emitted_sha256=digest(output),
                original_records=(len(original.split(b"\n")) - int(original.endswith(b"\n")))
                                 if original else 0,
                emitted_records=(len(output.split(b"\n")) - int(output.endswith(b"\n")))
                                if output else 0,
                original_bytes=len(original), emitted_bytes=len(output),
                prerequisite=prerequisite, effect=effect)
    with Path(config["log"]).open("a") as stream:
        stream.write(json.dumps(call) + "\n")
    sys.stdout.buffer.write(output)
    sys.stderr.buffer.write(stderr)
    raise SystemExit(exit_code)


def run(command, directory, environment, timeout):
    start = time.monotonic()
    process = subprocess.Popen(command, cwd=directory, env=environment, text=True,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               start_new_session=True)
    timed_out = False
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        timed_out = True
        # The session contains only this trial and its descendants.
        os.killpg(process.pid, signal.SIGKILL)
        stdout, stderr = process.communicate()
    return dict(command=command, exit_code=process.returncode, stdout=stdout,
                stderr=stderr, timed_out=timed_out,
                elapsed_seconds=time.monotonic() - start)


def variants(test):
    command = test["command"]
    if "--cpp" in command:
        index = command.index("--cpp") + 1
        path = Path(command[index])
        if path.is_file():
            return [("probe", index, [(path.name, path, "")])]
        if test["name"] != "log_formatter_rust_parity":
            raise RuntimeError(f"Unreviewed --cpp directory: {test['name']}")
        return [(name, index, [(name, path / name, "")]) for name in
                ("teaql_audit_event_probe", "teaql_runtime_log_probe",
                 "teaql_log_formatter_probe")]
    if "--cpp-dir" in command:
        index = command.index("--cpp-dir") + 1
        model = command[command.index("--model") + 1]
        name = "teaql_inherited_checker_helpers_" + model + "_probe"
        return [(model, index, [(name, Path(command[index]) / name, "")])]
    if "--schema-crud" in command:
        return [(flag[2:], command.index(flag) + 1,
                 [(Path(command[command.index(flag) + 1]).name,
                   Path(command[command.index(flag) + 1]), prefix)])
                for flag, prefix in (("--schema-crud", "schema+crud example rows: "),
                                     ("--relations-graph", "relation+graph example rows: "))]
    if test["name"] in {"parity_comparator_result_contracts", "parity_corpus_inventory_contracts",
                         "parity_corpus_source_controls", "parity_resolved_corpus_source_controls",
                         "parity_composite_corpus_source_controls", "parity_remaining_corpus_source_controls",
                         "parity_declared_core_driver_controls", "parity_runtime_foundation_driver_controls",
                         "parity_sql_executor_driver_controls", "parity_entity_runtime_driver_controls", "parity_registry_driver_controls", "parity_query_store_driver_controls", "parity_private_runtime_driver_controls", "parity_entity_api_driver_controls", "parity_sql_mutation_driver_controls"}:
        return []
    raise RuntimeError(f"Unreviewed native argument shape: {test['name']}")


def references():
    paths = {"runtime": ROOT / ".reference/teaql-rs",
             "generator": ROOT / ".reference/teaql_code_gen",
             "producer": ROOT.parent / "teaql_code_gen_cpp"}
    return {name: subprocess.check_output(["git", "-C", str(path), "rev-parse", "HEAD"],
                                          text=True).strip() for name, path in paths.items()}


def input_hashes():
    frozen = json.loads(gzip.decompress((ROOT / "docs/parity/"
        "audit_expression_full_native_inputs_5_0_6.json.gz").read_bytes()))
    roots = {"runtime": ROOT, "producer": ROOT.parent / "teaql_code_gen_cpp"}
    paths = {name: roots[prefix] / relative for name in frozen["hashes"]
             for prefix, relative in [name.split(":", 1)]}
    paths["runtime:" + str(Path(__file__).relative_to(ROOT))] = Path(__file__)
    return {name: digest(path.read_bytes()) for name, path in paths.items()}


def main():
    from inventory_parity_baselines import direct_corpus

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--test", action="append", help="Exact registration; repeat to select a cohort")
    parser.add_argument("--modes", nargs="+", choices=MODES, default=list(MODES))
    args = parser.parse_args()
    if args.record.exists() or not args.record.name.endswith(".json.gz"):
        parser.error("Choose a fresh .json.gz receipt; historical runs are immutable")
    registry = json.loads(subprocess.check_output(
        ["ctest", "--preset", "native", "--show-only=json-v1"], cwd=ROOT))
    selected = []
    for test in registry["tests"]:
        scripts = [Path(part) for part in test.get("command", [])
                   if part.endswith(".py") and Path(part).parent == PARITY]
        if len(scripts) == 1 and direct_corpus(scripts[0]) is None:
            if not args.test or test["name"] in args.test:
                selected.append(test)
    if args.test and set(args.test) != {test["name"] for test in selected}:
        parser.error("A selected registration is absent or is not a custom comparator")
    record = dict(scope=__doc__, complete=False, full_goal_complete=False,
                  runtime_commit=json.loads((ROOT / "upstream.lock.json").read_text())["commit"],
                  references_before=references(), input_hashes_before=input_hashes(),
                  registry=registry, selected=[test["name"] for test in selected],
                  cases=[], free_bytes_before=shutil.disk_usage(ROOT).free)
    settings = ROOT / "build_native_current/DartConfiguration.tcl"
    record["default_timeout_source"] = str(settings)
    record["default_timeout_source_sha256"] = digest(settings.read_bytes())
    default_timeout = next(float(line.split(":", 1)[1]) for line in settings.read_text().splitlines()
                           if line.startswith("TimeOut:"))
    record["default_timeout"] = default_timeout

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record, ensure_ascii=True).encode(), mtime=0))

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_dispatcher_result_controls_506_") as temporary:
            owned = Path(temporary)
            record["owned_directory"] = str(owned)
            for number, test in enumerate(selected):
                properties = {entry["name"]: entry["value"] for entry in test["properties"]}
                child_temporary = owned / "children"
                child_temporary.mkdir(exist_ok=True)
                environment = dict(os.environ, PYTHONDONTWRITEBYTECODE="1",
                                   TMPDIR=str(child_temporary))
                for entry in properties.get("ENVIRONMENT", []):
                    name, value = entry.split("=", 1)
                    environment[name] = value
                configurations = variants(test)
                if not configurations:
                    result = run(test["command"], properties.get("WORKING_DIRECTORY", str(ROOT)),
                                 environment, properties.get("TIMEOUT", default_timeout))
                    result.update(mode="existing_invariant_controls", calls=[], changed=False,
                                  accepted_changed_output=False)
                    record["cases"].append(dict(test=test["name"], invariant=True,
                                                original_command=test["command"],
                                                properties=properties, trials=[result]))
                    if result["exit_code"] != 0:
                        raise RuntimeError("Existing comparator invariant controls failed")
                    print(f"{number + 1}/{len(selected)} {test['name']}: existing controls pass",
                          flush=True)
                for variant, argument, binaries in configurations:
                    directory = owned / str(number) / variant
                    directory.mkdir(parents=True)
                    cache = directory / "cache"
                    cache.mkdir()
                    log = directory / "calls.jsonl"
                    mode_file = directory / "mode.json"
                    command = list(test["command"])
                    original_argument = Path(command[argument])
                    if original_argument.is_dir():
                        # Keep all unrelated probes real; replace only the selected branch.
                        for path in original_argument.iterdir():
                            (directory / path.name).symlink_to(path)
                    for name, native, prefix in binaries:
                        wrapper = directory / name
                        if wrapper.is_symlink():
                            wrapper.unlink()
                        config_path = directory / (name + ".control.json")
                        config_path.write_text(json.dumps(dict(native=str(native),
                            native_sha256=digest(native.read_bytes()), cache=str(cache),
                            log=str(log), prefix=prefix, mode_file=str(mode_file))))
                        wrapper.write_text("#!" + sys.executable + "\nimport runpy,sys\n"
                            + "sys.argv = " + repr([str(Path(__file__)), "--probe", str(config_path)])
                            + " + sys.argv[1:]\nrunpy.run_path(" + repr(str(Path(__file__)))
                            + ", run_name='__main__')\n")
                        wrapper.chmod(0o700)
                    command[argument] = str(directory if original_argument.is_dir() else wrapper)
                    case = dict(test=test["name"], variant=variant,
                                original_command=test["command"], properties=properties, trials=[])
                    record["cases"].append(case)
                    for mode in ("valid", "replay", *args.modes):
                        log.unlink(missing_ok=True)
                        mode_file.write_text(json.dumps(mode))
                        diagnostics = directory / "diagnostics" / mode
                        result = run(command, properties.get("WORKING_DIRECTORY", str(ROOT)),
                                     dict(environment, TEAQL_PARITY_DIAGNOSTIC_DIR=str(diagnostics)),
                                     properties.get("TIMEOUT", default_timeout))
                        result["mode"] = mode
                        result["calls"] = [json.loads(line) for line in log.read_text().splitlines()] \
                            if log.exists() else []
                        changed = any(call["effect"] for call in result["calls"])
                        result["changed"] = changed
                        result["accepted_changed_output"] = changed and result["exit_code"] == 0
                        case["trials"].append(result)
                        if result["timed_out"]:
                            raise RuntimeError(f"Native/control execution failed: {test['name']}/{mode}")
                        if mode in ("valid", "replay") and (result["exit_code"] != 0 or not result["calls"]):
                            raise RuntimeError(f"Real-output wrapper did not pass: {test['name']}")
                    print(f"{number + 1}/{len(selected)} {test['name']}/{variant}: "
                          + ", ".join(f"{trial['mode']}={trial['exit_code']}"
                                      + ("(unchanged)" if not trial["changed"] else "")
                                      for trial in case["trials"]), flush=True)
                    save()
                record["owned_bytes_before_cleanup"] = sum(
                    path.stat().st_size for path in owned.rglob("*")
                    if path.is_file() and not path.is_symlink())
    finally:
        record["owned_directory_removed"] = not Path(record.get("owned_directory", "/")).exists()
        record["references_after"] = references()
        record["input_hashes_after"] = input_hashes()
        record["inputs_unchanged"] = record["input_hashes_before"] == record["input_hashes_after"]
        trials = [trial for case in record["cases"] for trial in case["trials"]]
        record["accepted_mutations"] = sum(trial["accepted_changed_output"] for trial in trials)
        record["rejected_mutations"] = sum(trial["changed"] and trial["exit_code"] != 0
                                            for trial in trials)
        record["unchanged_controls"] = sum(not trial["changed"] and trial["mode"] in MODES
                                           for trial in trials)
        save()
    assert record["inputs_unchanged"] and record["references_before"] == record["references_after"]
    assert record["owned_directory_removed"]
    record["complete"] = True
    save()
    print(f"Result controls: {record['rejected_mutations']} rejected; "
          f"{record['accepted_mutations']} accepted changed outputs")
    return bool(record["accepted_mutations"])


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--probe":
        probe(Path(sys.argv[2]), sys.argv[3:])
    else:
        raise SystemExit(main())
