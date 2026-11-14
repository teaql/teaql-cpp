#!/usr/bin/env python3
"""Reject optimized-Python false startup parity for actual generated execution."""

import argparse
import ast
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "scripts/verify_business_environment_startup_5_0_6.py"
PREEDIT = ROOT / "docs/parity/business_environment_optimized_preedit_5_0_6.json.gz"
BINARY = ROOT / "build_native_current/teaql_generated_business_environment_probe"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_transition(before, after):
    lines = before.splitlines(keepends=True)
    assertions = sorted((node for node in ast.walk(ast.parse(before))
                         if isinstance(node, ast.Assert)),
                        key=lambda node: node.lineno, reverse=True)
    require(len(assertions) == 25 and all(node.lineno == node.end_lineno
                                          for node in assertions),
            "Twenty-five original one-line checks")
    for node in assertions:
        line = lines[node.lineno - 1]
        indentation = line[:len(line) - len(line.lstrip())]
        condition = ast.get_source_segment(before, node.test)
        message = ast.get_source_segment(before, node.msg) if node.msg else None
        require(line.lstrip().startswith("assert ") and line.endswith("\n"),
                "Original assertion line")
        lines[node.lineno - 1] = (
            indentation + "if not (" + condition + "):\n"
            + indentation + "    raise AssertionError"
            + ("(" + message + ")" if message else "") + "\n")
    require(after == "".join(lines), "Only the exact twenty-five checks changed")
    require(not any(isinstance(node, ast.Assert) for node in ast.walk(ast.parse(after))),
            "No optimized-away assertion remains in the startup comparator")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    if args.record is not None:
        require(not args.record.exists(), "Fresh control record")
    before = json.loads(gzip.decompress(PREEDIT.read_bytes()))
    require(before["complete"] is True and before["source_path"] == str(SOURCE.relative_to(ROOT))
            and before["source_sha256"] == hashlib.sha256(
                before["source_before"].encode()).hexdigest()
            and before["program_sha256"] == hashlib.sha256(
                before["program"].encode()).hexdigest(),
            "Exact reproduced predecessor")
    require(before["results"][0]["optimized"] is False
            and before["results"][0]["exit_code"] != 0
            and before["results"][0]["report_complete"] is False
            and before["results"][1]["optimized"] is True
            and before["results"][1]["exit_code"] == 0
            and before["results"][1]["report_complete"] is True
            and before["results"][1]["cases"] == 68
            and before["results"][1]["first_actual"] == {"wrong": True}
            and before["owned_directory_removed"] is True,
            "Actual optimized-Python false acceptance")
    source_transition(before["source_before"], SOURCE.read_text())
    require(BINARY.is_file(), "Built native generated startup executable")
    source_hash = digest(SOURCE)
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE="1")
    environment.pop("PYTHONOPTIMIZE", None)
    results = []

    with tempfile.TemporaryDirectory(prefix="teaql_business_environment_optimized_controls_") as owned:
        for optimized in (False, True):
            record = Path(owned) / ("optimized.json.gz" if optimized else "normal.json.gz")
            command = [sys.executable] + (["-O"] if optimized else []) + [
                "-c", before["program"], str(ROOT / "scripts"), str(BINARY), str(record)]
            process = subprocess.run(command, cwd=ROOT, env=environment,
                                     capture_output=True, text=True, timeout=120)
            report = json.loads(gzip.decompress(record.read_bytes())) if record.exists() else {}
            require(process.returncode != 0 and "AssertionError" in process.stderr
                    and report.get("complete") is False and len(report.get("cases", [])) == 1
                    and report["cases"][0]["cpp"]["actual"] == {"wrong": True},
                    "Wrong generated startup output accepted under "
                    + ("optimized" if optimized else "normal") + " Python")
            results.append({"case": "wrong_generated_output", "optimized": optimized,
                            "exit_code": process.returncode, "report_cases": 1})

        source_program = (
            "import sys\n"
            "sys.path.insert(0, sys.argv[1])\n"
            "import verify_business_environment_startup_5_0_6 as verifier\n"
            "verifier.source_inputs = lambda: {'wrong': True}\n"
            "sys.argv = ['startup', '--cpp', sys.argv[2]]\n"
            "verifier.main()\n"
        )
        process = subprocess.run(
            [sys.executable, "-O", "-c", source_program, str(ROOT / "scripts"), str(BINARY)],
            cwd=ROOT, env=environment, capture_output=True, text=True, timeout=120)
        require(process.returncode != 0 and "AssertionError" in process.stderr,
                "Changed source inputs accepted under optimized Python")
        results.append({"case": "wrong_source_inputs", "optimized": True,
                        "exit_code": process.returncode})
        removed_bytes = sum(path.stat().st_size for path in Path(owned).rglob("*")
                            if path.is_file())

    require(not Path(owned).exists() and digest(SOURCE) == source_hash,
            "Owned controls cleaned and comparator unchanged")
    require(len(results) == 3, "Three actual negative controls")
    if args.record is not None:
        record = {"complete": True, "whole_goal_complete": False,
                  "source_before_sha256": before["source_sha256"],
                  "source_after_sha256": source_hash, "preedit_sha256": digest(PREEDIT),
                  "binary_sha256": digest(BINARY), "results": results,
                  "owned_directory_removed": True, "owned_bytes_removed": removed_bytes}
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(),
                                              mtime=0))
    print("25/25 unconditional checks, 3/3 negative controls under normal/optimized Python")


if __name__ == "__main__":
    main()
