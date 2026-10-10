"""Record unchanged public transaction scope calls against Rust v5.0.5."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe
from generate_audited_save import inputs as save_inputs
from generate_audited_auto_save import case as base_case

def case(**kwargs):
    result = base_case()
    result.update(flow="save", body_fail=False)
    result.update(kwargs)
    return result

def inputs():
    cases = []
    for original in save_inputs():
        for flow in ["save", "save_twice"]:
            cases.append(case(**original, flow=flow))
    for flow, body_fail, begin_fail, commit_fail, rollback_fail, registered in itertools.product(
            ["ready", "query", "mutate", "service_query", "prepared_mutate", "missing_service", "mixed", "save_query"],
            [False, True], [False, True], [False, True], [False, True], [False, True]):
        cases.append(case(flow=flow, body_fail=body_fail, begin_fail=begin_fail,
                          commit_fail=commit_fail, rollback_fail=rollback_fail, registered=registered))
    for flow, error, read_error, event_error, body_fail, commit_fail, rollback_fail in itertools.product(
            ["query", "mutate", "query_save", "save_query", "mixed"], [-1,0], [-1,0], [-1,0],
            [False,True], [False,True], [False,True]):
        cases.append(case(flow=flow, fail=error, read_fail_at=read_error, event_fail=event_error,
                          body_fail=body_fail, commit_fail=commit_fail, rollback_fail=rollback_fail))
    return cases

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify-upstream.py")], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    lock = json.loads((root / "upstream.lock.json").read_text())
    corpus = dict(baseline=lock["commit"],
        note="Direct unmodified public execute_in_transaction and TransactionScope calls, including query, mutate, entity service, repeated audited saves, and combined rollback failure. Only compiler-specific missing-resource type names and dynamic audit timestamps are normalized.",
        cases=[dict(input=i, expected=o) for i,o in zip(cases,outputs)])
    Path(__file__).with_name("transaction_scope_cases.json").write_text(json.dumps(corpus, indent=2) + "\n")
    print(f"Recorded {len(cases)} TransactionScope observations")

if __name__ == "__main__":
    main()
