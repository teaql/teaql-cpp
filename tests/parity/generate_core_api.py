#!/usr/bin/env python3
"""Record unchanged original public calls; seed 505, signed boundary fixtures."""
import argparse
import json
import random
import subprocess
from pathlib import Path
from compare_values import observe


def inputs():
    bounds = [-(1 << 63), -(1 << 63) + 1, -(1 << 53), -(1 << 32), -(1 << 31),
              -1001, -1000, -1, 0, 1, 1000, 1001, (1 << 31) - 1,
              (1 << 32) - 1, (1 << 53), (1 << 63) - 2, (1 << 63) - 1]
    cases = [dict(kind="timestamp", values=[str(x) for x in bounds]),
             dict(kind="timestamp", values=[]),
             dict(kind="timestamp", values=["0", "-1", "0", "-1", "1"])]
    rng = random.Random(505)
    for _ in range(64):
        values = rng.choices(bounds, k=8) + [rng.randrange(-(1 << 63), 1 << 63) for _ in range(8)]
        rng.shuffle(values)
        cases.append(dict(kind="timestamp", values=[str(x) for x in values]))
    lists = [[], [""], ["name"], ["name", "name"], ["password", "name", "status"],
             ["status", "name", "status", ""], ["雪", "😀", "e\u0301", "a\0b"],
             ["a" * 4096, "b" * 4096, "a" * 4096]]
    for name in ["Customer", "URL", "雪Customer", ""]:
        for initial in [None, [], ["previous"]]:
            for fields in lists:
                for alias in [False, True]:
                    cases.append(dict(kind="metadata", name=name, initial=initial, fields=fields, alias=alias))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify_upstream.py")], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    lock = json.loads((root / "upstream.lock.json").read_text())
    corpus = dict(baseline=lock["commit"],
                  note="Original Timestamp Ord/Hash scalar writes and dynamic audit-mask builder; seed 505.",
                  cases=[dict(input=x, expected=y) for x, y in zip(cases, outputs)])
    Path(__file__).with_name("core_api_cases.json").write_text(json.dumps(corpus, indent=2) + "\n")
    print(f"Recorded {len(cases)} original Core API observations")


if __name__ == "__main__":
    main()
