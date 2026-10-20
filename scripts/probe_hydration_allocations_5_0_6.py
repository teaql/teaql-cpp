#!/usr/bin/env python3
"""Exploratory hydration measurement; does not qualify allocation budgets."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "tests/parity/hydration_allocation_probe.cpp"
RECORD = ROOT / "docs/parity/hydration_allocation_exploration_5_0_6.json.gz"
RECORD_PATH = None
RUN_CONTEXT = {"steps": []}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, cwd):
    result = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
    RUN_CONTEXT["steps"].append({"command": list(map(str, command)), "cwd": str(cwd),
                                 "exit_code": result.returncode, "stdout": result.stdout, "stderr": result.stderr})
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


def main():
    global RECORD_PATH
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", choices=("build_native_current", "build_sanitize_current"), default="build_native_current")
    parser.add_argument("--record", type=Path, default=RECORD)
    args = parser.parse_args()
    build = ROOT / args.build_dir
    if args.record.exists():
        raise FileExistsError("Preserve existing evidence; choose a new --record path")
    RECORD_PATH = args.record
    RUN_CONTEXT.update(build_profile=args.build_dir, source=SOURCE.read_text(), source_sha256=digest(SOURCE))
    commands = json.loads((build / "compile_commands.json").read_text())
    entry = next(item for item in commands if item["file"].endswith("/dynamic_relation_tests.cpp"))
    source_hash = digest(SOURCE)
    runtime_paths = ["teaql_runtime/src/entity_runtime.cpp", "teaql_runtime/include/teaql/runtime/entity_runtime.hpp", "teaql_memory/include/teaql/memory/scoped.hpp", "teaql_memory/include/teaql/memory/arena.hpp", "teaql_memory/src/arena.cpp"]
    runtime_inputs = {path: digest(ROOT / path) for path in runtime_paths}
    generated = SOURCE.parent.parent / "generator/parity/hydration_allocation_contracts/cpp"
    generated_hashes = {str(p.relative_to(ROOT)): digest(p) for p in generated.rglob("*") if p.is_file()}
    RUN_CONTEXT.update(runtime_inputs_sha256=runtime_inputs, generated_sha256=generated_hashes)
    template = run(["ninja", "-C", str(build), "-t", "commands", "teaql_dynamic_relation_tests"], ROOT).splitlines()[-1]
    link = shlex.split(template.removeprefix(": && ").removesuffix(" && :"))
    libraries = {str((build / arg).resolve()): digest((build / arg).resolve()) for arg in link if arg.endswith(".a")}
    with tempfile.TemporaryDirectory(prefix="teaql_hydration_probe_") as temporary:
        obj = str(Path(temporary) / "probe.o")
        executable = str(Path(temporary) / "probe")
        compile_command = shlex.split(entry["command"])
        compile_command[compile_command.index("-o") + 1] = obj
        compile_command[-1] = str(SOURCE)
        run(compile_command, entry["directory"])
        link = [obj if arg.endswith("dynamic_relation_tests.cpp.o") else arg for arg in link]
        link[link.index("-o") + 1] = executable
        link.append("-Wl,--wrap=_ZN5teaql5Arena11do_allocateEmm")
        run(link, build)
        output = run([executable], ROOT)
        binary_hash = digest(Path(executable))
    rows = []
    for line in output.splitlines():
        parts = line.split(",")
        if parts[0] in {"typed_hydration", "context_hydration", "identity_batch", "plain_hydration"}:
            rows.append([*parts[:2], *map(int, parts[2:])])
    expected = {(path, projection, size) for path in ("typed_hydration", "context_hydration", "identity_batch", "plain_hydration") for projection in ("sparse", "full") for size in (1, 100, 10000)}
    assert len(rows) == 24 and {tuple(row[:3]) for row in rows} == expected
    assert "HYDRATION_SAMPLES=24;POSITIVE_CONTROLS=passed;BUDGET_CEILINGS=passed;SOURCE_ACCEPTED=false" in output
    assert "HYDRATION_MOVE_CONTRACT=passed;ORIGINAL_ROW_TRANSFER=passed" in output
    assert all(digest(ROOT / path) == value for path, value in runtime_inputs.items())
    assert source_hash == digest(SOURCE)
    assert all(digest(ROOT / path) == value for path, value in generated_hashes.items())
    assert all(digest(Path(path)) == value for path, value in libraries.items())
    record = {
        "steps": RUN_CONTEXT["steps"],
        "complete": True, "source_promoted": False, "allocation_budget_accepted": False, "budget_ceilings_passed": True,
        "scope": "Exploration only; no original or module qualification",
        "build_profile": args.build_dir,
        "runtime_inputs_sha256": runtime_inputs,
        "source_sha256": source_hash, "source": SOURCE.read_text(),
        "generated_sha256": generated_hashes, "libraries_sha256": libraries,
        "compile_command": compile_command, "link_command": link,
        "binary_sha256": binary_hash, "output": output, "rows": rows,
        "columns": ["path", "projection", "rows", "arena_requests", "arena_bytes", "heap_calls", "heap_bytes", "elapsed_ns"],
    }
    args.record.write_bytes(gzip.compress(json.dumps(record, indent=2).encode(), mtime=0))
    print(output, end="")
    print(args.record)


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        if RECORD_PATH is not None and not RECORD_PATH.exists():
            failure = dict(RUN_CONTEXT, complete=False, source_promoted=False,
                           allocation_budget_accepted=False, error=str(error))
            RECORD_PATH.write_bytes(gzip.compress(json.dumps(failure, indent=2).encode(), mtime=0))
        raise
