"""Owned CLI fixtures for retained generated matrices and SQLite raw Debug."""

import copy
import gzip
import importlib
import json
import os
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
FAMILIES = {
    "expression_diagnostics": ("compare_generated_expression_diagnostics.py",
                               "generated_expression_diagnostics_5_0_6_cases.json", 196),
    "expression_values": ("compare_generated_expression_values.py",
                          "generated_expression_values_5_0_6_cases.json", 108),
    "runtime_assembly": ("compare_generated_runtime_assembly.py",
                        "generated_runtime_assembly_5_0_6_cases.json.gz", 72),
    "sample_execution": ("compare_generated_sample_execution.py",
                        "generated_sample_execution_5_0_6_cases.json.gz", 18),
    "sample_failures": ("compare_generated_sample_failures.py",
                       "generated_sample_failures_5_0_6_cases.json.gz", 28),
    "sample_state": ("compare_generated_sample_state.py",
                    "generated_sample_state_5_0_6_cases.json", 144),
    "sqlite_raw_debug": ("compare_sqlite_from_sql_raw_debug.py",
                         "sqlite_from_sql_raw_debug_5_0_6_cases.json.gz", 36),
}
MODES = ("valid", "foreign_source", "empty", "partial", "extra", "row_metadata",
         "wrong_output", "missing_output", "extra_output", "blank_output")
OBSERVER = '''import json, os, sys
from pathlib import Path
path = Path(__file__)
path.with_suffix('.called').write_text('called')
outputs = json.loads(path.with_suffix('.json').read_text())
rows = [outputs[json.dumps(json.loads(line), sort_keys=True, ensure_ascii=True,
                          separators=(',', ':'))] for line in sys.stdin]
mode = os.environ['TEAQL_GENERATED_CLI_CONTROL']
if mode == 'wrong_output':
    rows[0] = {'unexpected_control_output': True}
elif mode == 'missing_output':
    rows.pop()
elif mode == 'extra_output':
    rows.append(rows[-1])
elif mode == 'blank_output':
    print()
for row in rows:
    print(json.dumps(row, ensure_ascii=False))
'''


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == ".gz" else data)


def write(path, value):
    data = json.dumps(value, ensure_ascii=False).encode()
    path.write_bytes(gzip.compress(data, mtime=0) if path.suffix == ".gz" else data)


def prepare(directory, scripts):
    repository = Path(directory) / "repository"
    parity = repository / "tests/parity"
    parity.mkdir(parents=True)
    for relative in {"tests/parity/compare_values.py", "tests/parity/expression_cli_cases.py",
                     *scripts.values()}:
        target = repository / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes((ROOT / relative).read_bytes())
    (repository / "upstream.lock.json").write_bytes((ROOT / "upstream.lock.json").read_bytes())
    for relative in (".reference", "scripts", "tests/generator"):
        (repository / relative).symlink_to(ROOT / relative, target_is_directory=True)
    adaptation = importlib.import_module("compare_generated_expression_diagnostics").adapt
    from compare_values import same
    probes = {}
    for name, (_, corpus_name, count) in FAMILIES.items():
        corpus = read(ROOT / "tests/parity" / corpus_name)
        require(len(corpus["cases"]) == count, "Complete retained fixture scope")
        outputs = {}
        for case in corpus["cases"]:
            key = json.dumps(case["input"], ensure_ascii=True, sort_keys=True, separators=(",", ":"))
            expected = adaptation(case["expected"]) if name == "expression_diagnostics" else case["expected"]
            require(key not in outputs or same(outputs[key], expected), "Conflicting duplicate input")
            outputs[key] = expected
        probe = Path(directory) / (name + ".py")
        probe.write_text("#!" + sys.executable + "\n" + OBSERVER)
        probe.chmod(0o700)
        probe.with_suffix(".json").write_text(json.dumps(outputs, ensure_ascii=True))
        probes[name] = probe
    return repository, probes


def run_case(repository, probes, scripts, family, mode, optimized):
    _, corpus_name, _ = FAMILIES[family]
    corpus = read(ROOT / "tests/parity" / corpus_name)
    if mode == "foreign_source":
        if family == "expression_diagnostics":
            corpus["source_sha256"] = "0" * 64
        elif family == "sqlite_raw_debug":
            corpus["baseline"] = "0" * 40
        else:
            producer = corpus["producers"]["checkers"] if family == "runtime_assembly" else corpus["producer"]
            producer["model_sha256"] = "0" * 64
    elif mode == "empty":
        corpus["cases"] = []
    elif mode == "partial":
        corpus["cases"] = corpus["cases"][:-1]
    elif mode == "extra":
        corpus["cases"].append(copy.deepcopy(corpus["cases"][-1]))
    elif mode == "row_metadata":
        corpus["cases"][0]["invented_control_field"] = True
    require(mode in MODES, "Known generated CLI fixture mode")
    write(repository / "tests/parity" / corpus_name, corpus)
    probe = probes[family]
    marker = probe.with_suffix(".called")
    if marker.exists():
        marker.unlink()
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE="1", TEAQL_LOG_ENDPOINT="off",
                       TEAQL_GENERATED_CLI_CONTROL=mode)
    environment.pop("PYTHONOPTIMIZE", None)
    environment.pop("PYTHONPATH", None)
    command = [sys.executable, "-B", *(["-O"] if optimized else []),
               str(repository / scripts[family]), "--cpp", str(probe)]
    result = subprocess.run(command, cwd=repository, env=environment,
                            capture_output=True, text=True, timeout=120)
    return {"family": family, "mode": mode, "optimized": optimized,
            "exit_code": result.returncode, "invoked": marker.exists(),
            "stdout": result.stdout, "stderr": result.stderr}


def owned_bytes(directory):
    return sum(path.stat().st_size for path in Path(directory).rglob("*")
               if not path.is_symlink() and path.is_file())
