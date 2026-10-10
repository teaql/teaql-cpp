"""Record original typed transaction completion with a non-Clone output."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same
from generate_transaction_scope import case


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify_upstream.py")], check=True)
    cases = [case(flow="move_only", body_fail=body, begin_fail=begin,
                  commit_fail=commit, rollback_fail=rollback, payload="owned🧊" * 80)
             for body, begin, commit, rollback in itertools.product([False, True], repeat=4)]
    outputs = observe(args.rust, cases)
    if not same(outputs, observe(args.rust, cases)):
        raise RuntimeError("Repeated original non-Clone transaction outputs disagree")
    corpus = dict(baseline=json.loads((root / "upstream.lock.json").read_text())["commit"],
                  note="Actual original execute_in_transaction with a non-Clone callback capture/result and Drop counter; begin/body/commit/rollback success and failure. Original provider io::Error itself is also non-Clone. Outcome and exact terminal ordering are observed after explicit result/Context teardown.",
                  cases=[dict(input=data, expected=out) for data, out in zip(cases, outputs)])
    Path(__file__).with_name("transaction_move_only_cases.json").write_text(json.dumps(corpus, indent=2) + "\n")
    print(f"Recorded {len(cases)} twice-agreeing original non-Clone transaction observations")


if __name__ == "__main__":
    main()
