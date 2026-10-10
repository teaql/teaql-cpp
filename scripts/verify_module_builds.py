#!/usr/bin/env python3
"""Measure matching header/module builds and verify real incremental edit propagation."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
PACKAGES = ("teaql_memory", "teaql_async", "teaql_core", "teaql_data_service",
            "teaql_sql", "teaql_runtime", "teaql_provider_sqlite")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True,
                        help="New directory outside the repository for the isolated snapshot")
    parser.add_argument("--jobs", type=int, default=8)
    args = parser.parse_args()
    output = args.output_dir.resolve()
    if output.exists() or output.is_relative_to(ROOT):
        raise SystemExit("Use a new output directory outside the producer repository")
    source = output / "source"
    source.mkdir(parents=True)
    for directory in (*PACKAGES, "modules", "cmake"):
        shutil.copytree(ROOT / directory, source / directory)
    shutil.copy2(ROOT / "CMakeLists.txt", source / "CMakeLists.txt")
    fixture = source / "modules/tests/incremental_consumer"
    header_includes = []
    for package in PACKAGES:
        interface = (source / package / "src/lib.cppm").read_text().split("export module", 1)[0]
        for spelling in re.findall(r"^#include (<teaql/[^>]+>)", interface, re.M):
            if spelling not in header_includes:
                header_includes.append(spelling)
    (fixture / "header_scope.hpp").write_text(
        "#pragma once\n" + "".join(f"#include {spelling}\n" for spelling in header_includes))
    with (source / "CMakeLists.txt").open("a") as root_cmake:
        root_cmake.write("\nadd_subdirectory(modules/tests/incremental_consumer)\n")
    original_hashes = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                       for package in PACKAGES for path in (ROOT / package).rglob("*")
                       if path.is_file()}
    records = []
    builds = {mode: output / f"build_{mode}" for mode in ("headers", "modules")}

    def run(name, command, expected=None, no_work=False, timed=False):
        print(f"Running {name}", flush=True)
        logfile = output / f"{name}.txt"
        resource_file = output / f"{name}_resources.txt"
        executed = command
        if timed:
            executed = ["/usr/bin/time", "-v", "-o", str(resource_file), *command]
        start = time.monotonic()
        with logfile.open("w") as log:
            completed = subprocess.run(executed, stdout=log, stderr=subprocess.STDOUT,
                                       env={**os.environ, "LC_ALL": "C"})
        row = {"name": name, "command": command, "exit_code": completed.returncode,
               "wall_seconds": round(time.monotonic() - start, 3)}
        if resource_file.exists():
            resources = resource_file.read_text()
            match = re.search(r"Maximum resident set size \(kbytes\):\s*(\d+)", resources)
            if not match:
                raise SystemExit(f"Missing maximum child RSS measurement in {resource_file}")
            row["max_child_rss_kib"] = int(match.group(1))
        records.append(row)
        (output / "steps.json").write_text(json.dumps(records, indent=2) + "\n")
        if completed.returncode:
            raise SystemExit(f"{name} failed; inspect {logfile}")
        text = logfile.read_text()
        if expected is not None and text.strip() != expected:
            raise SystemExit(f"{name} observed stale/incorrect output: {text!r}, expected {expected!r}")
        if no_work and "no work to do" not in text:
            raise SystemExit(f"{name} rebuilt work unexpectedly")
        return text

    def build(mode, phase, expected, require=None, forbid=None):
        directory = builds[mode]
        text = run(f"{mode}_{phase}_build", ["cmake", "--build", str(directory),
                   "--target", "teaql_build_consumer", "-j", str(args.jobs)], timed=True)
        if require and require not in text:
            raise SystemExit(f"Missing expected incremental work {require!r} in {mode}/{phase}")
        if forbid and forbid in text:
            raise SystemExit(f"Unexpected incremental work {forbid!r} in {mode}/{phase}")
        executable = directory / "modules/tests/incremental_consumer/teaql_build_consumer"
        run(f"{mode}_{phase}_run", [str(executable)], expected=expected)
        run(f"{mode}_{phase}_no_work", ["cmake", "--build", str(directory),
            "--target", "teaql_build_consumer"], no_work=True, timed=True)

    baseline = "19782 0 0 2024"
    for mode, directory in builds.items():
        run(f"{mode}_configure", ["cmake", "-S", str(source), "-B", str(directory), "-G", "Ninja",
            f"-DCMAKE_TOOLCHAIN_FILE={ROOT / 'scripts/toolchains/clang_native.cmake'}",
            "-DCMAKE_BUILD_TYPE=Debug", "-DCMAKE_CXX_FLAGS_DEBUG=-g1",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DBUILD_TESTING=OFF", "-DTEAQL_BUILD_EXAMPLES=OFF",
            f"-DTEAQL_ENABLE_MODULES={'ON' if mode == 'modules' else 'OFF'}"])
        build(mode, "clean", baseline, require="Building CXX object")
    # A consumer-only edit leaves all library/interface inputs unchanged.
    consumer = fixture / "main.cpp"
    consumer.write_text(consumer.read_text() + "\n// Controlled consumer-only rebuild.\n")
    for mode in builds:
        build(mode, "consumer_edit", baseline, require="main.cpp.o", forbid="src/lib.cppm.o")

    def edit(path, before, after, expected, phase, require, forbid=None):
        original = path.read_text()
        if original.count(before) != 1:
            raise SystemExit(f"Controlled edit is not unique in {path}: {before!r}")
        try:
            path.write_text(original.replace(before, after))
            for mode in builds:
                build(mode, phase, expected, require=require, forbid=forbid)
        finally:
            path.write_text(original)
        for mode in builds:
            build(mode, phase + "_restore", baseline, require=require, forbid=forbid)

    time_cpp = source / "teaql_core/src/time.cpp"
    edit(time_cpp, "day_ - 1 - 719468;", "day_ - 1 - 719467;", "19783 0 0 2024",
         "ordinary_body", "src/time.cpp.o", forbid="src/lib.cppm.o")
    time_header = source / "teaql_core/include/teaql/core/time.hpp"
    edit(time_header, "std::int64_t millis = 0;", "std::int64_t millis = 9;", "19782 9 0 2024",
         "declaration", "main.cpp.o")
    reference = source / "teaql_memory/include/teaql/memory/reference.hpp"
    edit(reference, "explicit Ref(std::uint64_t id) noexcept : id_(id) {}\n\n"
         "    std::uint64_t id() const noexcept {\n        return id_;\n    }",
         "explicit Ref(std::uint64_t id) noexcept : id_(id) {}\n\n"
         "    std::uint64_t id() const noexcept {\n        return id_ + 1;\n    }",
         "19782 0 1 2024", "template_body", "main.cpp.o")
    # Introduce a real project-include edge, then edit only its dependency.
    dependency = time_header.parent / "detail/incremental_dependency.hpp"
    dependency.write_text("#pragma once\nnamespace teaql::detail {\n"
                          "inline constexpr int incremental_year_offset = 0;\n}\n")
    original = time_header.read_text()
    try:
        time_header.write_text(original.replace("#pragma once", "#pragma once\n"
            "#include <teaql/core/detail/incremental_dependency.hpp>", 1).replace(
            "return year_;", "return year_ + detail::incremental_year_offset;", 1))
        for mode in builds:
            build(mode, "dependency_added", baseline, require="main.cpp.o")
        dependency.write_text(dependency.read_text().replace("offset = 0", "offset = 1"))
        for mode in builds:
            build(mode, "dependency_edit", "19782 0 0 2025", require="main.cpp.o")
    finally:
        time_header.write_text(original)
        dependency.unlink()
    for mode in builds:
        build(mode, "dependency_restore", baseline, require="main.cpp.o")

    for name, expected_hash in original_hashes.items():
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != expected_hash:
            raise SystemExit(f"Producer file changed during the isolated experiment: {name}")
    bmis = {str(path.relative_to(builds["modules"])): path.stat().st_size
            for path in builds["modules"].rglob("*.pcm")}
    result = {
        "status": "Clean, no-work and controlled incremental builds verified",
        "compiler": "Clang23.1.3/libstdc++16.2", "profile": "Debug -g1", "jobs": args.jobs,
        "scope": "All seven production packages plus one identical consumer; textual coverage matches global fragments",
        "measurements": "One run per scenario; warm filesystem caches; max_child_rss is maximum child process RSS, not concurrent tree RSS",
        "header_includes": header_includes,
        "source_snapshot_sha256": original_hashes,
        "toolchain_lock_sha256": hashlib.sha256((ROOT / "toolchains.lock.json").read_bytes()).hexdigest(),
        "bmi_bytes": bmis, "bmi_total_bytes": sum(bmis.values()), "steps": records,
    }
    (output / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    print("Verified all controlled edits and restoration without changing producer files", flush=True)


if __name__ == "__main__":
    main()
