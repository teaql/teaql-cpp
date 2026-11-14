#!/usr/bin/env python3
"""Reject optimized-Python empty and substituted generated sample comparisons."""

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
PREEDIT = ROOT / "docs/parity/generated_sample_optimized_preedit_5_0_6.json.gz"
EXPECTED = {
    "types": ("verify_sample_types_5_0_6", "teaql_sample_type_probe", 360, 13),
    "custom": ("verify_custom_sample_5_0_6", "teaql_custom_sample_probe", 140, 21),
    "catalog": ("verify_catalog_sample_5_0_6", "teaql_catalog_sample_probe", 240, 18),
    "business": ("verify_business_sample_5_0_6", "teaql_business_sample_probe", 128, 14),
}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_transition(before, after, count):
    lines = before.splitlines(keepends=True)
    assertions = sorted((node for node in ast.walk(ast.parse(before))
                         if isinstance(node, ast.Assert)),
                        key=lambda node: node.lineno, reverse=True)
    require(len(assertions) == count and all(node.lineno == node.end_lineno
                                             for node in assertions),
            "Original source assertion inventory")
    for node in assertions:
        line = lines[node.lineno - 1]
        indentation = line[:len(line) - len(line.lstrip())]
        condition = ast.get_source_segment(before, node.test)
        message = ast.get_source_segment(before, node.msg) if node.msg else None
        require(line.lstrip().startswith("assert ") and line.endswith("\n"),
                "Exact original assertion line")
        lines[node.lineno - 1] = (
            indentation + "if not (" + condition + "):\n"
            + indentation + "    raise AssertionError"
            + ("(" + message + ")" if message else "") + "\n")
    require(after == "".join(lines), "Only source checks changed")
    require(not any(isinstance(node, ast.Assert) for node in ast.walk(ast.parse(after))),
            "No optimized-away assertion in the active sample comparator")


def run(command, environment, label):
    process = subprocess.run(command, cwd=ROOT, env=environment,
                             capture_output=True, text=True, timeout=120)
    require(process.returncode != 0 and "AssertionError" in process.stderr
            and "Verified 0 " not in process.stdout,
            "Wrong generated sample accepted: " + label)
    return {"case": label, "exit_code": process.returncode}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    if args.record is not None:
        require(not args.record.exists(), "Fresh control receipt")
    before = json.loads(gzip.decompress(PREEDIT.read_bytes()))
    require(before["complete"] is True and before["whole_goal_complete"] is False
            and len(before["entries"]) == len(EXPECTED)
            and before["program_sha256"] == hashlib.sha256(
                before["program"].encode()).hexdigest(),
            "Complete four-comparator false-pass witness")
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE="1", TEAQL_LOG_ENDPOINT="off")
    environment.pop("PYTHONOPTIMIZE", None)
    results = []
    source_after = {}

    with tempfile.TemporaryDirectory(prefix="teaql_generated_sample_optimized_controls_") as owned:
        fake = Path(owned) / "empty_observer"
        fake.write_text("#!/usr/bin/python3\nimport sys\nsys.stdin.read()\n")
        fake.chmod(0o700)
        environment["TMPDIR"] = owned
        for row in before["entries"]:
            label = row["label"]
            require(label in EXPECTED, "Known generated sample comparison")
            module, binary_name, case_count, assertion_count = EXPECTED[label]
            source = ROOT / "scripts" / (module + ".py")
            binary = ROOT / "build_native_current" / binary_name
            require(row["source_path"] == str(source.relative_to(ROOT))
                    and row["binary_name"] == binary_name
                    and row["expected_count"] == case_count
                    and row["source_sha256"] == hashlib.sha256(
                        row["source_before"].encode()).hexdigest()
                    and row["results"][0]["optimized"] is False
                    and row["results"][0]["exit_code"] != 0
                    and row["results"][1]["optimized"] is True
                    and row["results"][1]["exit_code"] == 0
                    and "Verified 0 " in row["results"][1]["stdout"],
                    "Reproduced zero-observation false pass: " + label)
            source_transition(row["source_before"], source.read_text(), assertion_count)
            require(binary.is_file(), "Registered native sample executable: " + label)
            source_after[label] = digest(source)
            for optimized in (False, True):
                command = [sys.executable] + (["-O"] if optimized else []) + [
                    "-c", before["program"], str(ROOT / "scripts"), module, str(binary)]
                result = run(command, environment, label + "/empty_input/"
                             + ("optimized" if optimized else "normal"))
                result["optimized"] = optimized
                results.append(result)

            wrong_source = (
                "import importlib,sys\n"
                "sys.path.insert(0,sys.argv[1])\n"
                "verifier=importlib.import_module(sys.argv[2])\n"
                "verifier.provenance=lambda: {'wrong': True}\n"
                "sys.argv=[sys.argv[2],'--cpp',sys.argv[3]]\n"
                "verifier.main()\n"
            )
            results.append(run([sys.executable, "-O", "-c", wrong_source,
                                str(ROOT / "scripts"), module, str(binary)],
                               environment, label + "/wrong_source"))
            results.append(run([sys.executable, "-O", str(source), "--cpp", str(fake)],
                               environment, label + "/empty_observer"))
        removed_bytes = sum(path.stat().st_size for path in Path(owned).rglob("*")
                            if path.is_file())

    require(not Path(owned).exists() and len(results) == 16
            and all(digest(ROOT / "scripts" / (EXPECTED[label][0] + ".py")) == value
                    for label, value in source_after.items()),
            "Sixteen actual controls, unchanged sources and owned cleanup")
    if args.record is not None:
        record = {"complete": True, "whole_goal_complete": False,
                  "preedit_sha256": digest(PREEDIT), "source_after_sha256": source_after,
                  "results": results, "owned_directory_removed": True,
                  "owned_bytes_removed": removed_bytes}
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(),
                                              mtime=0))
    print("66/66 unconditional source checks and 16/16 negative processes")


if __name__ == "__main__":
    main()
