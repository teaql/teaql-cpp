#!/usr/bin/env python3
"""Qualify deferred-resource decisions and embedded replay checks under -O."""

import argparse
import ast
import copy
import gzip
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from verify_generated_driver_hardened_sources_5_0_6 import (
    ReplaceAssert, check_row, digest, require,
)


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "docs/parity/deferred_resource_hardened_transition_5_0_6.json.gz"
MANIFEST_SHA256 = "e3596b613b54426c2471e06026ba7be6eef6e149a58c26ef67808c23720ca105"
WITNESS = ROOT / "docs/parity/deferred_resource_optimized_preedit_5_0_6.json.gz"
EMBEDDED_WITNESS = ROOT / "docs/parity/deferred_resource_embedded_preedit_5_0_6.json.gz"
DRIVER = ROOT / "tests/parity/verify_deferred_resource_replacement_hardened_5_0_6.py"
CONTROLLER = ROOT / "tests/parity/verify_comparator_result_contracts_hardened_5_0_6.py"
SAVED = ROOT / "docs/parity/deferred_resource_replacement_final_5_0_6.json.gz"


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def load_module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def sources():
    require(digest(MANIFEST.read_bytes()) == MANIFEST_SHA256, "Deferred source transition changed")
    manifest, witness, embedded = read(MANIFEST), read(WITNESS), read(EMBEDDED_WITNESS)
    require(manifest["complete"] is True and manifest["whole_goal_complete"] is False
            and manifest["runtime_commit"] == "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3"
            and manifest["generator_commit"] == "45e1572c4956ca963410d8ea436ead3c8857087a"
            and manifest["witness_sha256"] == digest(WITNESS.read_bytes())
            and manifest["embedded_witness_sha256"] == digest(EMBEDDED_WITNESS.read_bytes())
            and manifest["assertions"] == 9 and manifest["embedded_assertions"] == 5,
            "Locked bounded deferred-resource repair")
    originals = {
        "tests/parity/verify_deferred_resource_replacement_5_0_6.py": witness,
        "tests/parity/verify_comparator_result_contracts_5_0_6.py": embedded,
    }
    require(len(manifest["rows"]) == 2
            and {row["historical_path"] for row in manifest["rows"]} == set(originals)
            and sum(row["assertions"] for row in manifest["rows"]) == 9
            and sum(len(row["adaptations"]) for row in manifest["rows"]) == 4,
            "Complete source pairs and declared controller adaptations")
    for row in manifest["rows"]:
        original = originals[row["historical_path"]]
        check_row(row, {"source": original["source"], "sha256": original["source_sha256"]})
    before, after = manifest["embedded"]["before_source"], manifest["embedded"]["after_source"]
    controller = load_module(CONTROLLER, "deferred_replay_controller")
    require(before == embedded["probe_source"] and after.strip() == controller.PROBE.strip()
            and sum(isinstance(n, ast.Assert) for n in ast.walk(ast.parse(before))) == 5
            and not any(isinstance(n, ast.Assert) for n in ast.walk(ast.parse(after)))
            and ast.dump(ReplaceAssert().visit(ast.parse(before)), include_attributes=False)
            == ast.dump(ast.parse(after), include_attributes=False),
            "Exact embedded-script assertion transition")
    for path, wanted in manifest["dependencies"].items():
        require(digest((ROOT / path).read_bytes()) == wanted, "Deferred comparison dependency changed")
    require(witness["complete"] is True and witness["whole_goal_complete"] is False
            and witness["cases"] == 143 and witness["positive_payloads"] == 163
            and witness["owned_directory_removed"] is True
            and len(witness["results"]) == 10
            and [(r["case"], r["optimized"], r["exit_code"] == 0) for r in witness["results"]]
            == [(name, optimized, name == "valid" or optimized)
                for name in ("valid", "empty", "missing", "wrong_pending_flag", "wrong_business_result")
                for optimized in (False, True)], "Four actual pre-edit false acceptances")
    require(embedded["complete"] is True and embedded["whole_goal_complete"] is False
            and embedded["embedded_assertions"] == 5 and embedded["invalid_input"] == "{}\n"
            and [(r["optimized"], r["exit_code"] == 0) for r in embedded["results"]]
            == [(False, False), (True, True)] and embedded["owned_directory_removed"] is True,
            "Actual embedded request-validation false acceptance")
    return manifest, controller


def check_bindings(manifest, build, profile):
    tests = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(build), "--show-only=json-v1"], text=True))["tests"]
    registered = {row["name"]: row for row in tests}
    bindings = [row for row in manifest["bindings"] if row["profile"] == profile]
    require(len(bindings) == (2 if profile == "native" else 1), "Exact deferred profile binding scope")
    for binding in bindings:
        require(binding["test"] in registered, "Missing deferred resource registration")
        command = registered[binding["test"]]["command"]
        normalized = ["${PYTHON}", *[
            value.replace(str(build.resolve()), "${BINARY_DIR}")
            .replace(str(ROOT), "${SOURCE_DIR}") for value in command[1:]]]
        require(Path(command[0]).resolve() == Path(sys.executable).resolve()
                and normalized == binding["command"], "Deferred active registration changed")
    return bindings


def invoke(script, optimized, *arguments, input_text=None):
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE="1", TEAQL_LOG_ENDPOINT="off")
    environment.pop("PYTHONOPTIMIZE", None)
    environment.pop("PYTHONPATH", None)
    command = [sys.executable, "-B", *(["-O"] if optimized else []), str(script), *map(str, arguments)]
    return subprocess.run(command, cwd=ROOT, env=environment, input=input_text,
                          capture_output=True, text=True, timeout=180)


def controls(controller):
    driver, saved = load_module(DRIVER, "deferred_comparison_driver"), read(SAVED)
    from compare_values import same
    inputs, valid = driver.cases(), saved["cpp"]
    require(same(inputs, saved["inputs"]) and len(inputs) == len(valid) == 143
            and sum(len(driver.required_results(case)) for case in inputs) == 163,
            "Complete authentic saved resource recipe")
    variants = [("valid", valid, None), ("empty", [], "AssertionError"),
                ("missing", valid[:-1], "AssertionError"),
                ("extra", valid + [copy.deepcopy(valid[-1])], "AssertionError")]

    def changed(name, index, key, value, error="AssertionError"):
        rows = copy.deepcopy(valid)
        rows[index][key] = value
        require(not same(rows, valid), "Resource mutation must change an actual observation")
        variants.append((name, rows, error))

    changed("wrong_pending", 0, "pending", False)
    changed("numeric_pending", 0, "pending", 1)
    changed("wrong_closed", 0, "closed", False)
    changed("wrong_business_result", 0, "retry", {"invalid_business_result": True})
    expired = next(i for i, case in enumerate(inputs)
                   if case["action"] == "resume" and case["profile"].endswith("_dead"))
    changed("wrong_expired", expired, "expired_rejected", False)
    fresh = next(i for i, case in enumerate(inputs)
                 if case["action"] == "resume" and not case["profile"].endswith("_dead"))
    changed("unexpected_expired", fresh, "expired_rejected", True)
    changed("numeric_expiration", fresh, "expired_rejected", 0)
    policy = next(i for i, case in enumerate(inputs) if case["profile"] == "policy")
    changed("policy_allowed", policy, "policy_denied", False)
    cancelled = next(i for i, case in enumerate(inputs) if case["action"] == "cancel")
    changed("uncancelled", cancelled, "cancelled", False)
    changed("numeric_cancelled", cancelled, "cancelled", 1)
    missing = copy.deepcopy(valid)
    del missing[0]["retry"]
    variants.append(("missing_payload", missing, "RuntimeError"))
    closed = next(i for i, case in enumerate(inputs) if case["action"] == "close")
    changed("forbidden_closed_payload", closed, "retry", None, "RuntimeError")
    results, embedded_results = [], []
    with tempfile.TemporaryDirectory(prefix="teaql_deferred_resource_controls_506_") as owned:
        directory = Path(owned)
        probe = directory / "probe.py"
        probe.write_text("#!" + sys.executable + "\nimport json,sys\nfrom pathlib import Path\n"
                         "path=Path(__file__)\npath.with_suffix('.called').write_text('called')\n"
                         "data=json.loads(path.with_suffix('.json').read_text())\n"
                         "requests=[json.loads(line) for line in sys.stdin]\n"
                         "if requests!=data['inputs']: raise RuntimeError('Source recipe mismatch')\n"
                         "for row in data['outputs']: print(json.dumps(row))\n")
        probe.chmod(0o700)
        marker = probe.with_suffix(".called")
        embedded = directory / "embedded.py"
        embedded.write_text("from pathlib import Path\nROOT=Path(" + repr(str(ROOT)) + ")\n"
                            "TYPED=" + repr(controller.TYPED) + "\n" + controller.PROBE)
        lookup = [dict(ownership=True, mode=mode, cancel=cancel)
                  for mode in ("list", "rows", "first", "one", "count", "exists", "page", "stream")
                  for cancel in (False, True)]
        for optimized in (False, True):
            for name, rows, error in variants:
                probe.with_suffix(".json").write_text(json.dumps({"inputs": inputs, "outputs": rows}))
                if marker.exists():
                    marker.unlink()
                result = invoke(DRIVER, optimized, "--cpp", probe)
                require(marker.exists() and (result.returncode == 0) == (error is None)
                        and (error is None or error in result.stderr),
                        "Deferred resource control failed: " + name + " " + result.stderr[-1200:])
                results.append({"name": name, "optimized": optimized, "exit_code": result.returncode,
                                "invoked": True, "stdout": result.stdout, "stderr": result.stderr})
            probe.with_suffix(".json").write_text(json.dumps({"inputs": inputs, "outputs": valid}))
            protected = directory / "existing_evidence.json.gz"
            protected.write_bytes(b"PRESERVE EXISTING EVIDENCE")
            result = invoke(DRIVER, optimized, "--cpp", probe, "--record", protected)
            require(result.returncode != 0 and "AssertionError" in result.stderr
                    and protected.read_bytes() == b"PRESERVE EXISTING EVIDENCE",
                    "Deferred comparator overwrote existing evidence")
            results.append({"name": "existing_evidence", "optimized": optimized,
                            "exit_code": result.returncode, "stdout": result.stdout,
                            "stderr": result.stderr, "existing_evidence_preserved": True})
            for name, requests in (("valid", lookup), ("invalid", [{}])):
                environment = dict(os.environ, PYTHONDONTWRITEBYTECODE="1",
                                   TEAQL_COMPARATOR_KIND="lookup", TEAQL_COMPARATOR_CONTROL="saved")
                environment.pop("PYTHONOPTIMIZE", None)
                environment.pop("PYTHONPATH", None)
                result = subprocess.run(
                    [sys.executable, "-B", *(["-O"] if optimized else []), str(embedded)],
                    cwd=ROOT, env=environment, input="".join(json.dumps(row) + "\n" for row in requests),
                    capture_output=True, text=True, timeout=120)
                require((result.returncode == 0) == (name == "valid")
                        and (len(result.stdout.splitlines()) == 16 if name == "valid"
                             else "AssertionError" in result.stderr),
                        "Embedded lookup request validation failed")
                embedded_results.append({"name": name, "optimized": optimized,
                                         "exit_code": result.returncode,
                                         "stdout": result.stdout, "stderr": result.stderr})
        removed = sum(path.stat().st_size for path in directory.rglob("*") if path.is_file())
    require(not directory.exists(), "Owned deferred-resource fixtures remain")
    return {"results": results, "embedded_results": embedded_results,
            "owned_bytes_removed": removed, "owned_directory_removed": True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_native_current")
    parser.add_argument("--profile", choices=("native", "module"), default="native")
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), "Preserve existing evidence")
    manifest, controller = sources()
    bindings = check_bindings(manifest, args.build_dir, args.profile)
    record = controls(controller)
    record.update(complete=True, whole_goal_complete=False, profile=args.profile, bindings=bindings,
                  manifest_sha256=digest(MANIFEST.read_bytes()))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print("Deferred resource repair: nine direct and five embedded checks, 143 scenarios, "
          "163 payloads and normal/optimized acceptance controls pass (" + args.profile + ")")


if __name__ == "__main__":
    main()
