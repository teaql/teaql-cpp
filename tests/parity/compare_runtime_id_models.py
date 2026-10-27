"""Compare runtime ID models with the corresponding original build profile."""
import argparse
import gzip
import json
from pathlib import Path
import subprocess
from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cpp", required=True)
    parser.add_argument("--cpp-emulator", default="")
    parser.add_argument("--profile", choices=["debug", "release"], default="debug")
    parser.add_argument("--rust")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    with gzip.open(Path(__file__).with_name("runtime_id_model_baseline_5_0_6_cases.json.gz"), "rt") as stream:
        corpus = json.load(stream)
    if corpus["baseline"] != json.loads((root / "upstream.lock.json").read_text())["commit"]:
        raise RuntimeError("Runtime ID observations belong to a different baseline")
    cases = corpus["cases"]
    if type(cases) is not list or not cases:
        raise RuntimeError("Comparison corpus cases must be a nonempty list")
    inputs = [case["input"] for case in cases]
    key = "expected_release" if args.profile == "release" else "expected"
    expected = [case[key] for case in cases]
    if args.rust:
        subprocess.run(["python3", str(root / "scripts/verify_upstream.py")], check=True)
        if not same(expected, observe(args.rust, inputs)):
            raise RuntimeError("Recorded observations differ from the original build profile")
    emulator = args.cpp_emulator.split(";") if args.cpp_emulator else ()
    actual = observe(args.cpp, inputs, emulator)
    failures = [(i, case["input"], a, b)
                for i, (case, a, b) in enumerate(zip(cases, expected, actual)) if not same(a, b)]
    for i, data, original, native in failures[:10]:
        print(f"Case {i}: {data}\nRust: {original}\nC++: {native}")
    print(f"Runtime ID model parity ({args.profile}): {len(cases) - len(failures)}/{len(cases)}")
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
