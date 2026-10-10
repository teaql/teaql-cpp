"""Record original public Context defaults without capturing nondeterministic IDs."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify-upstream.py")], check=True)
    cases = [dict(identifier=identifier, timezone=timezone, trace=trace)
             for identifier, timezone, trace in itertools.product(
                 [None, "", "alice", "用户🧊", "i" * 512],
                 ["", "UTC", "Asia/Shanghai", " arbitrary text ", "z" * 512],
                 ["", "request-42", "🧊\ntrace"])]
    outputs = observe(args.rust, cases)
    if not same(outputs, observe(args.rust, cases)):
        raise RuntimeError("Repeated original Context default observations disagree")
    if not all(value["trace_clock_is_microseconds"] for value in outputs):
        raise RuntimeError("Original default trace counters left the independent clock bracket")
    corpus = dict(baseline=json.loads((root / "upstream.lock.json").read_text())["commit"],
                  note="Actual original public defaults and setters. Trace counter is independently checked against an epoch microsecond bracket, not normalized to a captured timestamp. Native PID is checked in-process; thread IDs retain the documented C++ native-ID adaptation.",
                  cases=[dict(input=value, expected=output) for value, output in zip(cases, outputs)])
    Path(__file__).with_name("context_defaults_cases.json").write_text(json.dumps(corpus, indent=2) + "\n")
    print(f"Recorded {len(cases)} twice-agreeing original Context default observations")


if __name__ == "__main__":
    main()
