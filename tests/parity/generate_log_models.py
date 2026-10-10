"""Record actual runtime log model derives and defaults without projection."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same


def inputs():
    cases = [dict(kind="options", operation=operation, mode=mode)
             for operation, mode in itertools.product(range(5), range(4))]
    cases += [dict(kind="level", fallback=fallback, text=text)
              for fallback, text in itertools.product(range(4),
                  ["_silent", "_summary", "_full", "_full_with_payload", "", " _full", "unknown"])]
    params = [[], [dict(kind="u64", value="18446744073709551615"), dict(kind="text", value="用户🧊")],
              [dict(kind="f64", value="NaN"), dict(kind="typed_null")],
              [dict(kind="object", value={"nested": dict(kind="text", value="payload")})]]
    for operation, optional, values, info in itertools.product(range(5), [None, "", "note🧊"], params, [False, True]):
        cases.append(dict(kind="entries", operation=operation, optional=optional, params=values, info=info,
                          outcome=operation % 3, start=dict(seconds="-1", nanos=999999999),
                          end=dict(seconds="253402300800", nanos=123456789),
                          duration_seconds="17", duration_nanos=123456789))
    for seconds, nanos in [("0", 0), ("0", 1), ("0", 999999999), ("18446744073709551615", 999999999)]:
        cases.append(dict(kind="entries", operation=0, optional=None, params=[], info=False, outcome=0,
                          start=dict(seconds="0", nanos=0), end=dict(seconds="0", nanos=0),
                          duration_seconds=seconds, duration_nanos=nanos))
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
        raise RuntimeError("Original log model observations disagree across repeats")
    corpus = dict(baseline=json.loads((root / "upstream.lock.json").read_text())["commit"],
                  note="Actual original public log structs/enums, full Clone/PartialEq/Debug in nine modes, option/default gates, log-level parse and parameter_count. Full SqlLogContext and independent SystemTime/Duration fields are observed without timestamp normalization. This raw model matrix is separate from privacy projection and sink publication.",
                  cases=[dict(input=data, expected=out) for data, out in zip(cases, outputs)])
    with gzip.GzipFile(filename=str(Path(__file__).with_name("log_model_cases.json.gz")), mode="wb", mtime=0) as stream:
        stream.write(json.dumps(corpus, separators=(",", ":"), ensure_ascii=True).encode())
    print(f"Recorded {len(cases)} twice-agreeing original runtime log model cases")


if __name__ == "__main__":
    main()
