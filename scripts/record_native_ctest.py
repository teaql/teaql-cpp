#!/usr/bin/env python3
"""Record a terminal complete native JUnit run without replacing earlier evidence."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]


def failure_family(name, text):
    if re.search(r"different (?:upstream |source )?baseline|different upstream commit|"
                 r"baseline mismatch|wrong (?:upstream )?baseline", text, re.IGNORECASE):
        return "historical_baseline_requires_replay"
    if "timeout" in text.lower() or "timed out" in text.lower():
        return "timeout_requires_diagnosis"
    if "compiler" in name or "type_error" in name:
        return "compiler_contract_requires_diagnosis"
    if "rust_parity" in name:
        return "source_comparison_requires_diagnosis"
    return "native_contract_requires_diagnosis"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--junit", type=Path, required=True)
    parser.add_argument("--registry", type=Path, required=True)
    parser.add_argument("--exit-code", type=int, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    registry = json.loads(args.registry.read_text())
    registered = {test["name"]: test for test in registry["tests"]}
    if len(registered) != len(registry["tests"]):
        raise RuntimeError("Duplicate test names in registry")
    root = ET.parse(args.junit).getroot()
    cases = root.findall(".//testcase")
    names = [case.attrib["name"] for case in cases]
    if len(names) != len(set(names)) or set(names) != set(registered):
        raise RuntimeError("JUnit is not a complete run of the recorded native registry")
    tests = []
    for case in cases:
        name = case.attrib["name"]
        failure = case.find("failure")
        error = case.find("error")
        skipped = case.find("skipped")
        status = "failed" if failure is not None or error is not None else "passed"
        if skipped is not None or case.attrib.get("status") in ("notrun", "disabled"):
            status = "skipped"
        output = "\n".join(node.text or "" for node in case.iter()
                           if node.tag in ("failure", "error", "system-out", "system-err"))
        messages = "\n".join(node.attrib.get("message", "") for node in case.iter()
                             if node.tag in ("failure", "error", "skipped"))
        row = dict(name=name, status=status, seconds=float(case.attrib.get("time", "0")),
                   output_sha256=hashlib.sha256(output.encode()).hexdigest())
        if status != "passed":
            row.update(family=failure_family(name, messages + "\n" + output),
                       message=messages, output_excerpt=output[:4000])
        tests.append(row)
    counts = {status: sum(row["status"] == status for row in tests)
              for status in ("passed", "failed", "skipped")}
    if args.exit_code == 0 and counts["failed"]:
        raise RuntimeError("Successful terminal exit contradicts JUnit failures")
    if args.exit_code != 0 and not counts["failed"] and not counts["skipped"]:
        raise RuntimeError("Failed terminal exit has no matching JUnit disposition")
    families = {}
    for row in tests:
        if "family" in row:
            families[row["family"]] = families.get(row["family"], 0) + 1
    record = dict(
        schema=1, runtime_commit=lock["commit"], profile="native",
        scope="Complete post-readonly native run; all registered test names accounted for.",
        total=len(tests), **counts, exit_code=args.exit_code,
        junit_sha256=hashlib.sha256(args.junit.read_bytes()).hexdigest(),
        registry_sha256=hashlib.sha256(args.registry.read_bytes()).hexdigest(),
        notice="Terminal diagnostic evidence, not complete capability qualification. Failure "
               "families are triage hints requiring inspection; output excerpts may be truncated. "
               "Earlier complete-run evidence remains unchanged.",
        failure_families=families, tests=tests,
    )
    if args.report.exists():
        raise FileExistsError(f"Preserve prior evidence; choose a new report: {args.report}")
    args.report.write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps(dict(total=len(tests), **counts, failure_families=families), sort_keys=True))


if __name__ == "__main__":
    main()
