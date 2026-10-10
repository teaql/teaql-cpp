"""Record public store calls without copying the original implementation."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe


def step(op, key="key", user=0, direct=False, value=None, timeout=None):
    return dict(op=op, key=key, user=user, direct=direct, value=value, timeout=timeout)


def inputs():
    values = [dict(kind=k, value=v) for k, v in [
        ("null", None), ("typed_null", None), ("text", ""), ("text", "shared"),
        ("text", "unicode \u4e2d\0tail"), ("bool", True), ("i64", "-9223372036854775808"),
        ("u64", "18446744073709551615"), ("f64", "-0.0"), ("decimal", "1.2300"),
        ("date", "2026-10-07"), ("timestamp", "-1"), ("json", {"nested": [None, 2]}),
        ("list", [dict(kind="text", value="nested"), dict(kind="typed_null", value=None)]),
        ("object", {"field": dict(kind="list", value=[dict(kind="text", value="x" * 256)])}),
    ]]
    cases = []
    for key, v, direct, installed in itertools.product(
            ["", "key", "long / key", "unicode \u4e2d\0key"], values, [False, True], [False, True]):
        steps = [step("install", user=1)]
        if installed:
            steps.append(step("install"))
        steps += [step("get", key, direct=direct), step("put", key, direct=direct, value=v),
                  step("get", key, direct=direct), step("get", key, user=1),
                  step("new"), step("install"), step("get", key),
                  step("remove", key, direct=direct), step("get", key, user=1),
                  step("remove", key, user=1)]
        cases.append(dict(steps=steps))
    for timeout, direct in itertools.product([0, 60, 2**63 - 1, 2**64 - 1], [False, True]):
        cases.append(dict(steps=[step("install"),
            step("put", direct=direct, value=values[3], timeout=timeout), step("sleep"),
            step("get", direct=direct), step("put", value=values[6]), step("get"),
            step("remove"), step("get")]))
    for timeout in [None, 0, 60]:
        cases.append(dict(steps=[step("install"), step("put", value=values[3]),
            step("put", value=values[7], timeout=timeout), step("sleep"), step("get"),
            step("put", value=values[0]), step("get")]))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify-upstream.py")], check=True)
    scripts = inputs()
    outputs = observe(args.rust, scripts)
    lock = json.loads((root / "upstream.lock.json").read_text())
    corpus = dict(baseline=lock["commit"], note="Direct unchanged original InMemoryDataStore and UserContext public store calls. Shared fixture forwards the original SPI; no copied store algorithm. Failure marks original Instant overflow panic and subsequent lock poisoning. Sleep advances zero TTL before observation.",
                  cases=[dict(input=i, expected=o) for i, o in zip(scripts, outputs)])
    Path(__file__).with_name("data_store_cases.json").write_text(json.dumps(corpus, indent=2) + "\n")
    print(f"Recorded {len(scripts)} public DataStore scripts")


if __name__ == "__main__":
    main()
