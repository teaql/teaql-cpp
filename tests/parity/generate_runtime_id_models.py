"""Record original runtime clock/profile/key/allocator/internal-ID model behavior."""
import argparse
import itertools
import gzip
import json
from pathlib import Path
import subprocess
from compare_values import observe, same


def inputs():
    cases = [dict(kind="clock", seconds=str(seconds), nanos=nanos)
             for seconds, nanos in [(0, 0), (-1, 123000000), (59, 1000000000),
                                    (59, 1999999999), (3600, 123456789),
                                    (253402300800, 0), (-62167219200, 0)]]
    cases += [dict(kind="key", version=version, byte=byte, other_version=other_version,
                   other_byte=other_byte)
              for version, byte, other_version, other_byte in itertools.product(
                  [0, 1, (1 << 32) - 1], [0, 0xff], [1, (1 << 32) - 1], [0, 0xff])]
    steps = [[], [dict(op="generate")], [dict(op="floor", value="1020"), dict(op="generate")],
             [dict(op="floor", value="0"), dict(op="generate"), dict(op="generate")],
             [dict(op="floor", value=str((1 << 64) - 1)), dict(op="generate"), dict(op="generate")]]
    cases += [dict(kind="counter", default=default, start=str(start), steps=actions)
              for default, start, actions in itertools.product([False, True], [0, 1000, (1 << 64) - 1], steps)]
    cases += [dict(kind="snowflake", worker=str(worker), datacenter=str(datacenter))
              for worker, datacenter in itertools.product([0, 1, 31, 32, (1 << 64) - 1], [0, 31, 32])]
    # Source HashMap iteration has no stable multi-entry text order. One-scope
    # observations expose exact private Mutex/state diagnostics deterministically.
    plans = [[], [dict(initial="0", maximum="2")],
             [dict(initial="0", maximum="2")] * 4,
             [dict(initial="3", maximum="2")],
             [dict(initial=str((1 << 64) - 1), maximum=str((1 << 64) - 1))],
             [dict(initial="1", maximum="9"), dict(initial="5", maximum="9")]]
    cases += [dict(kind="allocator", steps=actions) for actions in plans]
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    parser.add_argument("--rust-release", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify-upstream.py")], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    if not same(outputs, observe(args.rust, cases)):
        raise RuntimeError("Repeated original runtime ID observations disagree")
    release_outputs = observe(args.rust_release, cases)
    if not same(release_outputs, observe(args.rust_release, cases)):
        raise RuntimeError("Repeated original release runtime ID observations disagree")
    corpus = dict(baseline=json.loads((root / "upstream.lock.json").read_text())["commit"],
                  note="Actual original clock/profile/key/allocator/internal-ID constructors, derives and public transitions. All diagnostics use nine modes. Snowflake constructor state is deterministic; existing separate live generator tests cover clock windows. HashMap textual order is not a portable wire contract; deterministic allocator cases use one scope. Debug and release counter overflow are recorded independently from actual original builds.",
                  cases=[dict(input=case, expected=output, expected_release=release)
                         for case, output, release in zip(cases, outputs, release_outputs)])
    path = Path(__file__).with_name("runtime_id_model_cases.json.gz")
    with gzip.GzipFile(filename=str(path), mode="wb", mtime=0) as stream:
        stream.write(json.dumps(corpus, ensure_ascii=True, separators=(",", ":")).encode())
    print(f"Recorded {len(cases)} twice-agreeing original runtime ID model observations")


if __name__ == "__main__":
    main()
