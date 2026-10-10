"""Record actual original cache and pagination model diagnostics/derives."""
import argparse
import copy
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same


def inputs():
    values = [dict(kind="null"), dict(kind="typed_null"),
              dict(kind="text", value="用户\0🧊"), dict(kind="u64", value=str((1 << 64) - 1)),
              dict(kind="f64", value="NaN"), dict(kind="list", value=[dict(kind="bool", value=True)]),
              dict(kind="json", value={"name": "item", "count": 3})]
    cases = []
    for direction, boundary, timestamp in itertools.product(
            ["Asc", "Desc"], values, [(0, 0), (-1, 999999999), (253402300800, 123456789)]):
        cursor = dict(kind="cursor", cursor="cursor🧊", key="query\0key", entity="Item",
                      direction=direction, boundary=boundary, page_size=str((1 << 64) - 1),
                      offset="17", seconds=str(timestamp[0]), nanos=timestamp[1])
        cases.append(dict(cursor, other=copy.deepcopy(cursor)))
    base = cases[0]
    for field, changed in [("cursor", "other"), ("key", "other"), ("entity", "other"),
                           ("direction", "Desc"), ("boundary", dict(kind="i64", value="0")),
                           ("page_size", "0"), ("offset", "0"), ("seconds", "1"), ("nanos", 1)]:
        altered = copy.deepcopy(base)
        altered["other"][field] = changed
        cases.append(altered)
    cases += [dict(kind="ids", key=key, ids=ids, seconds=str(seconds), nanos=nanos)
              for key, ids, (seconds, nanos) in itertools.product(
                  ["", "query🧊\0", "k" * 512], [[], ["0", "1", str((1 << 64) - 1)]],
                  [(0, 0), (-1, 999999999), (253402300800, 123456789)])]
    cases += [dict(kind="cache", namespace=namespace)
              for namespace in ["", "teaql", "用户🧊\0", "n" * 512]]
    cases.append(dict(kind="cache_live"))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify-upstream.py")], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    if not same(outputs, observe(args.rust, cases)):
        raise RuntimeError("Original query-store observations disagree across repeats")
    corpus = dict(baseline=json.loads((root / "upstream.lock.json").read_text())["commit"],
                  note="Actual public originals, full cursor fields/Clone/equality, retained IDs/Clone, cache defaults and empty-map Debug. Live Instant payloads are not normalized into fixed observations; the live-cache case checks real put-clock brackets and unchanged stored timestamps, plus complete row diagnostics, without replacing production clocks.",
                  cases=[dict(input=case, expected=out) for case, out in zip(cases, outputs)])
    path = Path(__file__).with_name("query_store_model_cases.json.gz")
    with gzip.GzipFile(filename=str(path), mode="wb", mtime=0) as stream:
        stream.write(json.dumps(corpus, ensure_ascii=True, separators=(",", ":")).encode())
    print(f"Recorded {len(cases)} twice-agreeing original query-store model observations")


if __name__ == "__main__":
    main()
