#!/usr/bin/env python3
"""Run C++ against committed Rust observations; --rust verifies them live too."""
import argparse
import json
import math
import gzip
import subprocess
import shutil
from pathlib import Path


def same(expected, actual):
    # JSON equality here preserves number kind, including floating negative zero.
    if type(expected) is not type(actual):
        return False
    if isinstance(expected, dict):
        return expected.keys() == actual.keys() and all(same(expected[k], actual[k]) for k in expected)
    if isinstance(expected, list):
        return len(expected) == len(actual) and all(same(a, b) for a, b in zip(expected, actual))
    if isinstance(expected, float):
        return expected == actual and (expected != 0 or math.copysign(1, expected) == math.copysign(1, actual))
    return expected == actual


def observe(executable, inputs, emulator=(), arguments=()):
    if not Path(executable).is_file() and not shutil.which(executable):
        raise FileNotFoundError(f'Probe executable is missing; build its target first: {executable}')
    result = subprocess.run([*emulator, executable, *arguments], input="".join(json.dumps(v, ensure_ascii=True) + "\n" for v in inputs),
                            text=True, capture_output=True, check=True)
    # JSON Lines uses LF framing. Python splitlines() also splits literal U+0085,
    # U+2028 and U+2029 inside valid JSON strings emitted by the Rust serializer.
    lines = result.stdout.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    outputs = [json.loads(line) for line in lines]
    if len(outputs) != len(inputs):
        raise RuntimeError(f"{executable}: incomplete output: {len(outputs)} / {len(inputs)}")
    return outputs


def main(corpus_name="value_5_0_6_cases.json.gz", label="Value"):
    parser = argparse.ArgumentParser()
    parser.add_argument("--cpp", required=True)
    parser.add_argument("--rust")
    parser.add_argument("--cpp-emulator", default="",
                        help="CMake emulator command and arguments, separated by semicolons")
    args = parser.parse_args()
    corpus_path = Path(__file__).with_name(corpus_name)
    if corpus_path.suffix == ".gz":
        with gzip.open(corpus_path, "rt", encoding="utf-8") as source:
            corpus = json.load(source)
    else:
        corpus = json.loads(corpus_path.read_text())
    root = Path(__file__).resolve().parents[2]
    lock = json.loads((root / "upstream.lock.json").read_text())
    if corpus["baseline"] != lock["commit"]:
        raise RuntimeError(f"{label} observations belong to a different upstream baseline")
    cases = corpus["cases"]
    inputs = [c["input"] for c in cases]
    expected = [c["expected"] for c in cases]
    if args.rust:
        subprocess.run(["python3", str(root / "scripts/verify_upstream.py")], cwd=root, check=True)
        live = observe(args.rust, inputs)
        if not same(expected, live):
            raise RuntimeError("Committed observations differ from live Rust oracle")
    emulator = args.cpp_emulator.split(";") if args.cpp_emulator else ()
    actual = observe(args.cpp, inputs, emulator)
    failures = [(i, inputs[i], a, b) for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    for i, input_value, a, b in failures[:20]:
        print(f"Case {i}: {input_value}\n Rust: {a}\n C++:  {b}")
    print(f"{label} parity: {len(cases) - len(failures)}/{len(cases)} observations match")
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
