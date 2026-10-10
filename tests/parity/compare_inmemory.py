"""Compare the pinned public in-memory engine and expression evaluator."""
import argparse
import copy
import json
from pathlib import Path
import subprocess
from compare_values import observe, same


def target_input(case, bits):
    value = copy.deepcopy(case)
    query = value.get("query", {})
    if bits == 32 and "slice" in query:
        offset, limit = query["slice"]
        query["slice"] = [offset & 0xffffffff,
                          None if limit is None else limit & 0xffffffff]
    return value


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cpp", required=True)
    parser.add_argument("--rust")
    parser.add_argument("--cpp-emulator", default="",
                        help="Semicolon-separated CMake emulator command")
    args = parser.parse_args()
    emulator = [part for part in args.cpp_emulator.split(";") if part]
    root = Path(__file__).resolve().parents[2]
    corpus = json.loads(Path(__file__).with_name("inmemory_cases.json").read_text())
    lock = json.loads((root / "upstream.lock.json").read_text())
    if corpus["baseline"] != lock["commit"]:
        raise RuntimeError("In-memory observations belong to another baseline")
    bits = int(subprocess.run([*emulator, args.cpp, "--pointer-bits"], check=True, text=True,
                              capture_output=True).stdout)
    if bits not in [32, 64]:
        raise RuntimeError("Unsupported probe pointer width")
    cases = corpus["cases"]
    inputs = [c["input"] for c in cases]
    expected = [c.get("expected_32", c["expected"]) if bits == 32 else c["expected"] for c in cases]
    if args.rust:
        subprocess.run(["python3", str(root / "scripts/verify-upstream.py")], check=True)
        # On a 64-bit host, normalize only the source's explicit `as usize`
        # pagination casts before calling the unchanged public Rust engine.
        live = observe(args.rust, [target_input(i, bits) for i in inputs])
        if not same(expected, live):
            raise RuntimeError("Recorded observations differ from live Rust")
    actual = observe(args.cpp, inputs, emulator)
    failures = [(i, inputs[i], a, b) for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    for i, value, a, b in failures[:20]:
        print(f"Case {i}: {value}\n Rust: {a}\n C++: {b}")
    print(f"In-memory query/evaluator ({bits}-bit): {len(cases)-len(failures)}/{len(cases)} observations match")
    return bool(failures)

if __name__ == "__main__":
    raise SystemExit(main())
