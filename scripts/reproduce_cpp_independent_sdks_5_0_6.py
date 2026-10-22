#!/usr/bin/env python3
"""Strictly reproduce five active SDKs; optionally refresh proven source provenance.

Generated implementation files must remain identical. Original Rust and
historical C++ directories are never modified by this verifier.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
FAMILIES = ("query_alias_contracts", "relation_identity_contracts", "expression_root_contracts",
            "hydration_allocation_contracts", "graph_serialization_contracts")
HELPERS = ("CppFieldLayoutMetadata", "CppRustLiteralMetadata", "CppRustXidMetadata")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def files(path):
    return {str(source.relative_to(path)): digest(source) for source in path.rglob("*")
            if source.is_file() and source.name != "PROVENANCE.json"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--refresh-provenance", action="store_true",
                        help="Record current producer hashes only after every SDK byte matches")
    parser.add_argument("--record", type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists() or args.record.suffix != ".gz":
        parser.error("Use a fresh compressed evidence filename")
    sources = sorted((PRODUCER / "generator/src/main/java/com/skynet/codegenerator/cpp").glob("*.java"))
    runtime = json.loads((ROOT / "upstream.lock.json").read_text())
    generator = json.loads((PRODUCER / "cpp_generator.lock.json").read_text())
    inputs = [Path(__file__).resolve(), ROOT / "tests/generator/TypeConfigurationProbe.java", *sources]
    before = {str(path): digest(path) for path in inputs}
    record = {"complete": False, "scope": "Five active full C++ SDKs reproduce unchanged "
              "implementation bytes; explicit provenance-only recording, not semantic acceptance",
              "refresh_provenance": args.refresh_provenance, "steps": [], "cases": [],
              "input_hashes_before": before, "free_bytes_before": shutil.disk_usage(ROOT).free}

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + "\n").encode(), mtime=0))

    def run(label, command, cwd=ROOT):
        print(label, "running", flush=True)
        result = subprocess.run(command, cwd=cwd, text=True, capture_output=True, timeout=600)
        record["steps"].append({"label": label, "command": list(map(str, command)),
                                "cwd": str(cwd), "exit_code": result.returncode,
                                "stdout": result.stdout, "stderr": result.stderr})
        save()
        print(label, "exit", result.returncode, flush=True)
        result.check_returncode()
        return result

    owned = None
    try:
        run("upstream", ["python3", "scripts/verify_upstream.py"])
        with tempfile.TemporaryDirectory(prefix="teaql_independent_sdk_reproduction_506_") as name:
            owned = Path(name)
            record["owned_directory"] = name
            classpath = owned / "classpath.txt"
            run("classpath", ["mvn", "-q", "-o", "-pl", "generator", "dependency:build-classpath",
                "-Dmdep.outputFile=" + str(classpath)], PRODUCER)
            cp = ":".join(str(PRODUCER / path) for path in
                          ("generator/target/classes", "model/target/classes"))
            cp += ":" + classpath.read_text().strip()
            classes = owned / "classes"
            classes.mkdir()
            run("producer_compile", ["javac", "--release", "17", "-cp", cp, "-d", str(classes),
                *map(str, sources), str(ROOT / "tests/generator/TypeConfigurationProbe.java")])
            cp = ":".join(map(str, [classes, PRODUCER / "generator/src/main/resources"])) + ":" + cp
            staged = []
            for family in FAMILIES:
                target = ROOT / "tests/generator/parity" / family / "cpp"
                manifest = target / "PROVENANCE.json"
                previous = json.loads(manifest.read_text())
                assert previous["runtime_commit"] == runtime["commit"]
                assert previous["generator_commit"] == generator["generator_baseline_commit"]
                assert previous["hand_edited"] is False
                assert files(target) == previous["files"], family
                model = ROOT / previous["model"]
                assert digest(model) == previous["model_sha256"]
                output = owned / family
                run(family + "_generate", ["java", "-cp", cp, "TypeConfigurationProbe", "cpp",
                    str(model), str(output)])
                assert json.loads((output / "OBSERVATION.json").read_text())["generated"]
                run(family + "_format", [str(ROOT / ".tools/clang-format/bin/clang-format"),
                    "--style=file:" + str(ROOT / ".clang-format"), "--Werror", "-i",
                    *map(str, output.rglob("*.hpp"))])
                actual = files(output)
                assert actual == previous["files"], (family, [path for path in set(actual) | set(previous["files"])
                    if actual.get(path) != previous["files"].get(path)])
                updated = json.loads(json.dumps(previous))
                for path in updated["generator_implementation_sha256"]:
                    updated["generator_implementation_sha256"][path] = digest(PRODUCER / path)
                for helper in HELPERS:
                    path = "generator/src/main/java/com/skynet/codegenerator/cpp/" + helper + ".java"
                    updated["generator_implementation_sha256"][path] = digest(PRODUCER / path)
                if args.refresh_provenance:
                    staged.append((manifest, json.dumps(updated, indent=2) + "\n"))
                else:
                    assert previous == updated, family + " producer provenance differs"
                record["cases"].append({"family": family, "files": len(actual),
                    "generated_bytes_identical": True, "previous_provenance": previous,
                    "current_provenance": updated})
                save()
            for path, text in staged:
                path.write_text(text)
            record["changed_provenance"] = [str(path.relative_to(ROOT)) for path, _ in staged]
            record["complete"] = True
            record["owned_bytes_removed"] = sum(path.stat().st_size for path in owned.rglob("*")
                                                if path.is_file())
    except Exception as error:
        record["controller_error"] = {"type": type(error).__name__, "message": str(error)}
        save()
        raise
    finally:
        record["owned_directory_removed"] = owned is not None and not owned.exists()
        record["input_hashes_after"] = {str(path): digest(path) for path in inputs}
        record["inputs_unchanged"] = record["input_hashes_after"] == before
        record["complete"] = record["complete"] and record["inputs_unchanged"]
        save()
    return 0 if record["complete"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
