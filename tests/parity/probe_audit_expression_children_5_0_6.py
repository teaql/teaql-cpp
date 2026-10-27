#!/usr/bin/env python3
"""Compare selected syn expression child rules with the C++ producer adapter."""

import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from compare_values import json_lines, same


ROOT = Path(__file__).resolve().parents[2]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
REFERENCE_PROJECT = ROOT / "tests/parity/rust_container_metadata_reference_5_0_6"
JAVA_PACKAGE = "com/skynet/codegenerator/cpp"


def cases():
    expressions = (
        "become f()", "become (f())", "become if flag { a } else { b }",
        "become match value { Some(x) => x, None => 0 }",
        "become a + b", "become return a", "become { a }",
        "break", "break 'outer", "break 'outer value", "break (1 + 2)",
        "break 'outer: loop { break; }", "break ('outer: loop { break; })",
        "return", "return a + b", "return if flag { a } else { b }",
        "yield", "yield a + b", "yield (if flag { a } else { b })",
        "continue", "continue 'outer",
        "'outer: loop { break 'outer 1; }",
        "'outer: while ready { break 'outer; }",
        "'outer: for item in items { break 'outer; }",
        "'outer: { break 'outer 1; }",
        "static |x| x", "static move |x| x", "const |x| x",
        "const move |x| x", "async |x| x", "async move |x| x",
        "for<'a> |x: &'a str| x", "for<'a> move |x: &'a str| x",
        "|x| -> i32 { x + 1 }", "move |x| -> i32 { x + 1 }",
        "|x: &str| x.len()", "|x| async move { x }",
        "async { value }", "async move { value }",
        "try { value? }", "unsafe { value }", "const { 1 + 2 }",
        "a as *const T", "a as *mut T", "a as &T",
        "a as &'a T", "a as dyn Trait", "a as impl Trait",
        "a as fn(i32) -> i32", "a as [i32; 3]",
        "(a as T).field", "(a as T)?", "(a as T)(x)",
        "a..", "..a", "a..b", "..=a", "a..=b",
        "(a..b).start", "(a..b)?", "(a..b)[0]",
        "if let Some(x) = value && x > 0 { x } else { 0 }",
        "while let Some(x) = source.next() { consume(x); }",
        "for item in (0..10).filter(f) { consume(item); }",
        "match value { Some(x) if x > 0 => x, _ => 0 }",
        "match value { Some(x) => { x }, None => 0 }",
    )
    tails = ("", " + 1", ".field", "?", " as T")
    return [
        {
            "name": f"child_{index:03d}_{tail_index}",
            "input": "17, audit_value_max_len=" + expression + tail
            + ', entity="After", table="after_table", indexed_layout, audit_value_max_len=21',
        }
        for index, expression in enumerate(expressions)
        for tail_index, tail in enumerate(tails)
    ]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, *, cwd=ROOT, env=None, input_text=None):
    result = subprocess.run(command, cwd=cwd, env=env, input=input_text,
                            text=True, capture_output=True, timeout=900)
    if result.returncode:
        raise RuntimeError(f"{command[0]} exited {result.returncode}: {result.stderr[-3000:]}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists() or args.record.suffix != ".gz":
        parser.error("Use a fresh .gz evidence path")
    runtime = json.loads((ROOT / "upstream.lock.json").read_text())
    generator = json.loads((PRODUCER / "cpp_generator.lock.json").read_text())
    assert shutil.disk_usage(ROOT).free > 1024 ** 3
    selected = cases()
    assert len({item["input"] for item in selected}) == len(selected)
    sources = sorted((PRODUCER / "generator/src/main/java" / JAVA_PACKAGE).glob("*.java"))
    tracked = [*sources, REFERENCE_PROJECT / "main.rs",
               ROOT / "tests/generator/ContainerMetadataProbe.java",
               ROOT / ".reference/teaql-rs/teaql-macros/src/attr.rs", Path(__file__)]
    before = {str(path): digest(path) for path in tracked}
    encoded = "".join(json.dumps(item["input"], ensure_ascii=True) + "\n" for item in selected)
    with tempfile.TemporaryDirectory(prefix="teaql_audit_expression_children_506_") as owned_name:
        owned = Path(owned_name)
        env = dict(os.environ, CARGO_TARGET_DIR=str(owned / "target"),
                   CARGO_INCREMENTAL="0", CARGO_PROFILE_DEV_DEBUG="0",
                   CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER="/usr/bin/cc")
        run(["cargo", "build", "--offline", "--locked", "--manifest-path",
             str(REFERENCE_PROJECT / "Cargo.toml"), "-j", "2"], env=env)
        original = json_lines(run([
            str(owned / "target/debug/teaql_container_metadata_reference_506")],
            input_text=encoded))
        classpath_file = owned / "classpath.txt"
        run(["mvn", "-q", "-o", "-pl", "generator", "dependency:build-classpath",
             f"-Dmdep.outputFile={classpath_file}"], cwd=PRODUCER)
        cp = ":".join(str(PRODUCER / path) for path in
                      ("generator/target/classes", "model/target/classes"))
        cp += ":" + classpath_file.read_text().strip()
        classes = owned / "classes"
        classes.mkdir()
        run(["javac", "-cp", cp, "-d", str(classes), *map(str, sources),
             str(ROOT / "tests/generator/ContainerMetadataProbe.java")])
        cp = ":".join(map(str, [classes, PRODUCER / "generator/src/test/resources",
                               PRODUCER / "generator/src/main/resources"])) + ":" + cp
        current = json_lines(run(["java", "-cp", cp,
                                  "com.skynet.codegenerator.cpp.ContainerMetadataProbe"],
                                 input_text=encoded))
        assert len(original) == len(current) == len(selected)
        observations = [dict(case, original=expected, cpp=actual,
                             match=same(expected, actual) if expected.get("parsed") is True
                             else expected.get("parsed") is False
                             and actual.get("parsed") is False)
                        for case, expected, actual in zip(selected, original, current)]
        owned_bytes = sum(path.stat().st_size for path in owned.rglob("*") if path.is_file())
    after = {str(path): digest(path) for path in tracked}
    assert before == after
    record = {
        "scope": "Selected syn expression child rules and continuation effects; finite probe only",
        "runtime_commit": runtime["commit"],
        "generator_commit": generator["generator_baseline_commit"],
        "source_hashes": before,
        "cases": len(observations),
        "differences": sum(not item["match"] for item in observations),
        "observations": observations,
        "owned_directory": owned_name,
        "owned_bytes_removed": owned_bytes,
        "owned_directory_removed": not Path(owned_name).exists(),
    }
    args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + "\n").encode(), mtime=0))
    print("cases", record["cases"], "differences", record["differences"],
          "owned bytes removed", owned_bytes)


if __name__ == "__main__":
    main()
