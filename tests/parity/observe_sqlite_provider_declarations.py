#!/usr/bin/env python3
"""Record original provider declarations without replacing earlier observations."""
import argparse
import json
import subprocess
from pathlib import Path

from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify-upstream.py")], check=True)
    target = Path(__file__).with_name("sqlite_transport_cases.json")
    corpus = json.loads(target.read_text())
    baseline = json.loads((root / "upstream.lock.json").read_text())["commit"]
    if corpus["baseline"] != baseline:
        raise RuntimeError("Transport observations have a different baseline")
    cases = corpus["cases"]
    live = observe(args.rust, [case["input"] for case in cases])
    if not same(live, [case["expected"] for case in cases]):
        raise RuntimeError("Existing original transport observations changed")
    changed = False
    for mode in ["capabilities", "units"]:
        value = {"mode": mode, "count": 0, "sql": "SELECT 1", "params": []}
        expected = observe(args.rust, [value])[0]
        existing = [case for case in cases if case["input"] == value]
        if existing:
            if len(existing) != 1 or not same(existing[0]["expected"], expected):
                raise RuntimeError("Provider declaration observation changed or is duplicated")
        else:
            cases.append({"input": value, "expected": expected})
            changed = True
    if changed:
        target.write_text(json.dumps(corpus, ensure_ascii=False, indent=2) + "\n")
    print(f"Verified {len(cases)} original transport/declaration observations")


if __name__ == "__main__":
    main()
