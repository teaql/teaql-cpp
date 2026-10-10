#!/usr/bin/env python3
"""Measure verified header/module snapshots after other compiler jobs finish."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--experiment-dir", type=Path, required=True,
                        help="Completed verify_module_builds.py experiment")
    parser.add_argument("--output-dir", type=Path, required=True, help="New measurement directory")
    parser.add_argument("--jobs", type=int, default=8)
    args = parser.parse_args()
    experiment = args.experiment_dir.resolve()
    verification = json.loads((experiment / "results.json").read_text())
    if verification["status"] != "Clean, no-work and controlled incremental builds verified":
        raise SystemExit("Incremental verification must finish before isolated measurements")
    if args.jobs != verification["jobs"]:
        raise SystemExit("Keep the verified parallelism for the performance comparison")
    for name, digest in verification["source_snapshot_sha256"].items():
        if hashlib.sha256((experiment / "source" / name).read_bytes()).hexdigest() != digest:
            raise SystemExit(f"The isolated production snapshot was not fully restored: {name}")
    configuration = {}
    for mode in ("headers", "modules"):
        cache = (experiment / f"build_{mode}/CMakeCache.txt").read_text().splitlines()
        configuration[mode] = {line.split(":", 1)[0]: line.split("=", 1)[1]
            for line in cache if any(line.startswith(key + ":") for key in (
                "CMAKE_CXX_COMPILER", "CMAKE_BUILD_TYPE", "CMAKE_CXX_FLAGS", "CMAKE_CXX_FLAGS_DEBUG"))}
        compiler_files = list((experiment / f"build_{mode}/CMakeFiles").glob(
            "*/CMakeCXXCompiler.cmake"))
        if len(compiler_files) != 1:
            raise SystemExit(f"Missing or ambiguous configured compiler identity: {mode}")
        compiler_info = compiler_files[0].read_text()
        for key in ("CMAKE_CXX_COMPILER", "CMAKE_CXX_COMPILER_ID", "CMAKE_CXX_COMPILER_VERSION",
                    "CMAKE_CXX_PLATFORM_ID", "CMAKE_CXX_SIZEOF_DATA_PTR"):
            match = re.search(rf'set\({key} "([^\"]*)"\)', compiler_info)
            if not match:
                raise SystemExit(f"Missing configured compiler identity {key}: {mode}")
            configuration[mode][key] = match.group(1)
    if configuration["headers"] != configuration["modules"]:
        raise SystemExit("Header and module compiler/profile flags differ")
    output = args.output_dir.resolve()
    if output.exists():
        raise SystemExit("Use a new output directory to preserve prior measurements")
    # Reject known concurrent compilers. This does not promise an otherwise idle
    # host; load, affinity and quota are recorded so timings retain that limit.
    for process in Path("/proc").iterdir():
        if not process.name.isdigit():
            continue
        try:
            arguments = (process / "cmdline").read_bytes().split(b"\0")
            executable = Path(os.fsdecode(arguments[0])).name if arguments else ""
        except (FileNotFoundError, PermissionError, ProcessLookupError):
            continue
        if executable in {"clang++", "clang", "cc1plus", "g++", "clang-scan-deps"}:
            raise SystemExit(f"Wait for concurrent compiler PID {process.name} to finish")
    output.mkdir(parents=True)
    rows = []
    consumer = experiment / "source/modules/tests/incremental_consumer/main.cpp"
    if not consumer.exists():
        # The initial historical experiment used this fixture path before its
        # name was normalized to avoid the workspace build-directory ignore rule.
        consumer = experiment / "source/modules/tests/build_consumer/main.cpp"
    original = consumer.read_bytes()

    def run(name, command, expected=None):
        print(f"Measuring {name}", flush=True)
        metrics = output / f"{name}_resources.txt"
        load_before = os.getloadavg()
        start = time.monotonic()
        with (output / f"{name}.txt").open("w") as log:
            completed = subprocess.run(["/usr/bin/time", "-v", "-o", str(metrics), *command],
                stdout=log, stderr=subprocess.STDOUT, env={**os.environ, "LC_ALL": "C"})
        resources = metrics.read_text()
        match = re.search(r"Maximum resident set size \(kbytes\):\s*(\d+)", resources)
        if completed.returncode or not match:
            raise SystemExit(f"Failed measurement {name}: inspect {output}")
        text = (output / f"{name}.txt").read_text()
        if expected == "no_work" and "no work to do" not in text:
            raise SystemExit(f"Unexpected rebuild during {name}")
        if expected == "baseline" and text.strip() != "19782 0 0 2024":
            raise SystemExit(f"The verified source did not retain baseline behavior: {name}")
        rows.append({"name": name, "command": command,
            "wall_seconds": round(time.monotonic() - start, 3),
            "max_child_rss_kib": int(match.group(1)), "exit_code": completed.returncode,
            "load_before": load_before, "load_after": os.getloadavg()})
        (output / "steps.json").write_text(json.dumps(rows, indent=2) + "\n")

    try:
        # Each producer/consumer graph is rebuilt sequentially with identical
        # flags and job count. No compiler-upgrade comparison is involved.
        for mode in ("headers", "modules"):
            directory = experiment / f"build_{mode}"
            command = ["cmake", "--build", str(directory), "--target", "teaql_build_consumer"]
            run(f"{mode}_clean", ["cmake", "--build", str(directory), "--target", "clean"])
            run(f"{mode}_full_build", [*command, "-j", str(args.jobs)])
            executable = next(row["command"][0] for row in verification["steps"]
                              if row["name"] == f"{mode}_clean_run")
            run(f"{mode}_baseline", [executable], expected="baseline")
            run(f"{mode}_no_work", command, expected="no_work")
            consumer.write_bytes(original + b"\n// Isolated consumer timing edit.\n")
            run(f"{mode}_consumer_rebuild", [*command, "-j", str(args.jobs)])
            run(f"{mode}_consumer_result", [executable], expected="baseline")
            consumer.write_bytes(original)
            run(f"{mode}_consumer_restore", [*command, "-j", str(args.jobs)])
            run(f"{mode}_final_no_work", command, expected="no_work")
        # The two modes share the restored source snapshot. Editing it during
        # the second mode advances its mtime for the first mode as well.
        # Reconcile both graphs before claiming the final no-work state.
        for mode in ("headers", "modules"):
            directory = experiment / f"build_{mode}"
            command = ["cmake", "--build", str(directory), "--target", "teaql_build_consumer"]
            run(f"{mode}_reconciled_build", [*command, "-j", str(args.jobs)])
            run(f"{mode}_reconciled_no_work", command, expected="no_work")
    finally:
        consumer.write_bytes(original)
    bmis = {str(path.relative_to(experiment / "build_modules")): path.stat().st_size
            for path in (experiment / "build_modules").rglob("*.pcm")}
    quota_path = Path("/sys/fs/cgroup/cpu.max")
    cpu_quota = quota_path.read_text().strip() if quota_path.exists() else None
    if cpu_quota is None:
        # Older hosts use cgroup v1; an absent v2 file is not a measurement failure.
        for directory in Path("/sys/fs/cgroup").glob("cpu*"):
            quota = directory / "cpu.cfs_quota_us"
            period = directory / "cpu.cfs_period_us"
            if quota.is_file() and period.is_file():
                cpu_quota = {"version": 1, "quota_us": quota.read_text().strip(),
                             "period_us": period.read_text().strip()}
                break
    result = {
        "status": "Sequential same-compiler build measurements completed",
        "compiler": verification["compiler"], "profile": verification["profile"],
        "configuration": configuration,
        "jobs": args.jobs, "cpu_affinity_count": len(os.sched_getaffinity(0)),
        "cpu_quota": cpu_quota,
        "scope": verification["scope"],
        "limits": "One run per scenario; warm filesystem caches; maximum child RSS is not concurrent tree memory; host load recorded",
        "bmi_bytes": bmis, "bmi_total_bytes": sum(bmis.values()), "steps": rows,
    }
    (output / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    print("Recorded sequential header/module build and consumer timings", flush=True)


if __name__ == "__main__":
    main()
