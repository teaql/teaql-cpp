"""Owned subprocess fixtures for execution, termination and diagnostic checks."""

import json
import os
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
FAMILIES = {
    "service_errors": {
        "script": "scripts/verify_runtime_service_errors_5_0_6.py",
        "native": "runtime_service_errors_rust_parity",
        "module": "module_runtime_service_errors_rust_parity",
    },
    "callback_close": {
        "script": "tests/parity/check_sample_log_callback_close.py",
        "native": "sample_data_logging_callback_close",
        "module": "module_sample_data_logging_callback_close",
    },
    "optimistic_output": {
        "script": "tests/parity/teaql_runtime/optimistic_conflict_output.py",
        "native": "original_runtime_optimistic_conflict_output_contracts",
        "module": "module_original_runtime_optimistic_conflict_output_contracts",
    },
    "warning_privacy": {
        "script": "tests/parity/check_search_warning_canary.py",
        "native": "original_core_dynamic_search_warning_privacy",
        "module": "module_original_core_dynamic_search_warning_privacy",
    },
}
SERVICE_RECORD = ROOT / "docs/parity/runtime_service_errors_original_observations_5_0_6.json"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def build_cases(directory):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    cases = []

    def executable(label, stdout, stderr="", code=0):
        marker = directory / (label + ".called")
        target = directory / (label + ".py")
        target.write_text("#!" + sys.executable + "\n"
                          "from pathlib import Path\nimport sys\n"
                          "Path(" + repr(str(marker)) + ").write_text('called')\n"
                          "sys.stdout.write(" + repr(stdout) + ")\n"
                          "sys.stderr.write(" + repr(stderr) + ")\n"
                          "raise SystemExit(" + str(code) + ")\n")
        target.chmod(0o700)
        return target, marker

    def add(family, name, valid, stdout, stderr="", code=0, rust=None, missing=False):
        cpp, marker = executable(name, stdout, stderr, code)
        if missing:
            cpp.unlink()
        arguments = ["--cpp", str(cpp)]
        markers = {"cpp": marker}
        invoked = {"cpp": not missing}
        if rust is not None:
            binary, marker = executable(name + "_rust", rust)
            arguments.extend(["--rust", str(binary)])
            markers["rust"] = marker
            invoked["rust"] = True
            if not valid:
                invoked["cpp"] = False
        cases.append({"family": family, "name": name, "valid": valid,
                      "arguments": arguments, "markers": markers,
                      "expected_invocations": invoked,
                      "error": None if valid else "FileNotFoundError" if missing else "AssertionError"})

    service = json.dumps(json.loads(SERVICE_RECORD.read_text())["cases"]) + "\n"
    add("service_errors", "valid_service_cpp", True, service)
    add("service_errors", "valid_service_both", True, service, rust=service)
    add("service_errors", "different_cpp_errors", False, "[]\n")
    add("service_errors", "different_rust_errors", False, service, rust="[]\n")
    add("service_errors", "missing_cpp_program", False, service, missing=True)
    add("callback_close", "protected_callback_exit", True, "", code=86)
    add("callback_close", "callback_returned_normally", False, "")
    add("callback_close", "unrelated_callback_failure", False, "", code=1)
    listing = "running 1 test\n"
    add("optimistic_output", "valid_optimistic_output", True, listing)
    add("optimistic_output", "failed_optimistic_child", False, listing, code=1)
    add("optimistic_output", "missing_optimistic_listing", False, "")
    for stream in ("stdout", "stderr"):
        message = "OptimisticLockConflict in memory.rs\n"
        add("optimistic_output", "optimistic_leak_" + stream, False,
            listing + message if stream == "stdout" else listing,
            message if stream == "stderr" else "")
    add("warning_privacy", "valid_omitted_warning", True, listing, "<omitted>\n")
    add("warning_privacy", "missing_warning_listing", False, "", "<omitted>\n")
    add("warning_privacy", "missing_warning_omission", False, listing)
    for label, canary in (("field", "CLIENT_SECRET_FIELD_PATH_91"), ("value", "SECRET_VALUE_99")):
        for stream in ("stdout", "stderr"):
            add("warning_privacy", "warning_" + label + "_" + stream, False,
                listing + canary + "\n" if stream == "stdout" else listing,
                "<omitted>\n" + (canary + "\n" if stream == "stderr" else ""))
    require(len(cases) == 20 and sum(row["valid"] for row in cases) == 5,
            "Exact valid and invalid subprocess fixture scope")
    return cases


def run_case(script, case, optimized):
    for marker in case["markers"].values():
        if marker.exists():
            marker.unlink()
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE="1", TEAQL_LOG_ENDPOINT="off")
    environment.pop("PYTHONOPTIMIZE", None)
    environment.pop("PYTHONPATH", None)
    command = [sys.executable, "-B"] + (["-O"] if optimized else [])
    command.extend([str(script), *case["arguments"]])
    result = subprocess.run(command, cwd=ROOT, env=environment,
                            capture_output=True, text=True, timeout=120)
    return {"name": case["name"], "family": case["family"], "valid": case["valid"],
            "optimized": optimized, "exit_code": result.returncode,
            "stdout": result.stdout, "stderr": result.stderr,
            "invoked": {name: path.exists() for name, path in case["markers"].items()}}
