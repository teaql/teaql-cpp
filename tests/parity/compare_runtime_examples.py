"""Compare runnable C++ examples with real pinned Rust SQLite observations."""
import argparse
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--schema-crud", required=True)
    parser.add_argument("--relations-graph", required=True)
    parser.add_argument("--rust")
    parser.add_argument("--cpp-emulator", default="",
                        help="CMake emulator command and arguments, separated by semicolons")
    args = parser.parse_args()
    corpus = json.loads(Path(__file__).with_name("runtime_examples_cases.json").read_text())
    root = Path(__file__).resolve().parents[2]
    lock = json.loads((root / "upstream.lock.json").read_text())
    if corpus["baseline"] != lock["commit"]:
        raise RuntimeError("Examples belong to a different baseline")
    expected = [case["expected"] for case in corpus["cases"]]
    if args.rust:
        subprocess.run(["python3", str(root / "scripts/verify_upstream.py")], check=True)
        if not same(expected, observe(args.rust, [case["input"] for case in corpus["cases"]])):
            raise RuntimeError("Recorded examples differ from live original Rust calls")
        provenance = json.loads(Path(__file__).with_name("runtime_examples_provenance.json").read_text())
        if provenance["baseline"] != lock["commit"]:
            raise RuntimeError("SQL provenance belongs to a different baseline")
        live = observe(args.rust, [dict(case["input"], logs=True) for case in corpus["cases"]])
        if not same([provenance[case["input"]["mode"]] for case in corpus["cases"]], live):
            raise RuntimeError("Recorded SQL provenance differs from original Rust calls")
    actual = []
    emulator = args.cpp_emulator.split(";") if args.cpp_emulator else ()
    for binary, prefix in [(args.schema_crud, "schema+crud example rows: "),
                           (args.relations_graph, "relation+graph example rows: ")]:
        result = subprocess.run([*emulator, binary], text=True, capture_output=True, check=True)
        rows = [line[len(prefix):] for line in result.stdout.split("\n") if line.startswith(prefix)]
        if len(rows) != 1:
            raise RuntimeError(f"{binary}: missing or duplicate example result")
        actual.append(json.loads(rows[0]))
    if not same(expected, actual):
        raise RuntimeError(f"Example mismatch: Rust={expected}, C++={actual}")
    print("Runtime examples parity: 2/2 original SQLite scenarios match")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
