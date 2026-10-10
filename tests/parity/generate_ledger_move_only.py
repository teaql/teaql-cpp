"""Record the complete audited-save matrix with an original non-Clone entity."""
import argparse
import gzip
import json
from pathlib import Path
import subprocess
from compare_values import observe, same
from generate_audited_save import inputs


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify-upstream.py")], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    if not same(outputs, observe(args.rust, cases)):
        raise RuntimeError("Repeated original non-Clone ledger saves disagree")
    corpus = dict(baseline=json.loads((root / "upstream.lock.json").read_text())["commit"],
                  note="The full audited-with-executor matrix invokes actual original save with a model that has no Clone implementation. The existing fixture projects dynamic audit timestamps as in the separately reviewed audited-save protocol; this does not establish full raw event diagnostics or privacy acceptance.",
                  cases=[dict(input=data, expected=out) for data, out in zip(cases, outputs)])
    path = Path(__file__).with_name("ledger_move_only_cases.json.gz")
    with gzip.GzipFile(filename=str(path), mode="wb", mtime=0) as stream:
        stream.write(json.dumps(corpus, separators=(",", ":"), ensure_ascii=True).encode())
    print(f"Recorded {len(cases)} twice-agreeing original non-Clone ledger saves")


if __name__ == "__main__":
    main()
