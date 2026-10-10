#!/usr/bin/env python3
"""Install, relocate and exercise a TEAQL SDK using independent consumers."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
PACKAGES = ("teaql_memory", "teaql_async", "teaql_core", "teaql_data_service",
            "teaql_sql", "teaql_runtime", "teaql_provider_sqlite")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True,
                        help="Completed producer build; this tool does not build its archives")
    parser.add_argument("--output-dir", type=Path, required=True,
                        help="New directory for the relocated SDK, consumers and logs")
    parser.add_argument("--headers-only", action="store_true")
    parser.add_argument("--toolchain", type=Path,
                        default=ROOT / "scripts/toolchains/clang_native.cmake")
    parser.add_argument("--jobs", type=int, default=8)
    args = parser.parse_args()
    output = args.output_dir.resolve()
    if output.is_relative_to(ROOT):
        raise SystemExit("Use an output directory outside the producer repository")
    if output.exists():
        raise SystemExit(f"Use a new output directory to preserve existing evidence: {output}")
    output.mkdir(parents=True)
    steps = []

    def run(name, command):
        print(f"Running {name}", flush=True)
        start = time.monotonic()
        with (output / f"{name}.txt").open("w") as log:
            completed = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
        steps.append({"name": name, "command": command, "exit_code": completed.returncode,
                      "seconds": round(time.monotonic() - start, 3)})
        (output / "steps.json").write_text(json.dumps(steps, indent=2) + "\n")
        if completed.returncode:
            raise SystemExit(f"{name} failed; inspect {output / (name + '.txt')}")

    stage, sdk = output / "original_prefix", output / "relocated_sdk"
    run("install", ["cmake", "--install", str(args.build_dir.resolve()),
                    "--prefix", str(stage)])
    stage.rename(sdk)
    cmake_files = list(sdk.rglob("*.cmake"))
    forbidden = (str(ROOT), str(args.build_dir.resolve()), str(stage))
    for path in cmake_files:
        if any(spelling in path.read_text() for spelling in forbidden):
            raise SystemExit(f"Installed metadata retains a source/build/original prefix: {path}")
    if any(path.suffix in {".pcm", ".bmi", ".gcm"} for path in sdk.rglob("*")):
        raise SystemExit("The installed SDK must not distribute prebuilt BMIs")
    source = output / "consumer_source"
    shutil.copytree(ROOT / "modules/tests/external_consumer", source)
    modes = ["headers"] if args.headers_only else ["modules", "headers"]
    for mode in modes:
        build = output / f"consumer_{mode}"
        run(f"{mode}_configure", ["cmake", "-S", str(source), "-B", str(build), "-G", "Ninja",
            f"-DCMAKE_TOOLCHAIN_FILE={args.toolchain.resolve()}", f"-DCMAKE_PREFIX_PATH={sdk}",
            "-DCMAKE_BUILD_TYPE=Debug", "-DCMAKE_CXX_FLAGS_DEBUG=-g0",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            f"-DTEAQL_CONSUME_MODULES={'ON' if mode == 'modules' else 'OFF'}"])
        run(f"{mode}_build", ["cmake", "--build", str(build), "-j", str(args.jobs)])
        run(f"{mode}_ctest", ["ctest", "--test-dir", str(build), "--output-on-failure"])
        run(f"{mode}_no_work", ["cmake", "--build", str(build)])
        if "no work to do" not in (output / f"{mode}_no_work.txt").read_text():
            raise SystemExit(f"Installed {mode} consumer did not reach a no-work build")
        commands = json.loads((build / "compile_commands.json").read_text())
        for command in commands:
            if str(ROOT / "teaql_") in command["command"] or str(ROOT / "build_") in command["command"]:
                raise SystemExit(f"Consumer uses producer declarations or build outputs: {command}")
        if mode == "modules":
            ninja = (build / "build.ninja").read_text()
            for package in PACKAGES:
                interface = sdk / "share/teaql_cpp/modules" / package / "lib.cppm"
                if not interface.is_file() or str(interface) not in ninja:
                    raise SystemExit(f"Missing consumer-built installed module interface: {package}")
    cache = (args.build_dir / "CMakeCache.txt").read_text().splitlines()
    producer_flags = next((line.split("=", 1)[1] for line in cache
                           if line.startswith("CMAKE_CXX_FLAGS_DEBUG:STRING=")), None)
    evidence = {
        "status": "Installed consumer contracts passed",
        "modes": modes,
        "relocated_prefix": str(sdk),
        "original_prefix_exists": stage.exists(),
        "distributed_bmis": 0,
        "consumer_debug_flags": "-g0",
        "producer_debug_flags": producer_flags,
        "toolchain_lock_sha256": hashlib.sha256((ROOT / "toolchains.lock.json").read_bytes()).hexdigest(),
        "fixture_sha256": {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                           for path in sorted(source.iterdir()) if path.is_file()},
        "steps": steps,
    }
    (output / "results.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print(f"Verified installed {', '.join(modes)} consumers after moving the SDK prefix", flush=True)


if __name__ == "__main__":
    main()
