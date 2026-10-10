"""Record actual original reference model derives and key redaction."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same


def inputs():
    cases = []
    strings = ["", "simple", 'quoted "value"\n\\', "边缘设备🧊", "\u0000\u0085\u2028", "x" * 512]
    for text in strings:
        for identity, version in [(0, -(1 << 63)), (1, 0), ((1 << 64) - 1, (1 << 63) - 1)]:
            common = dict(id=str(identity), version=str(version))
            cases.append(dict(kind="principal", realm=text, subject=text, root=text, **common))
            cases.append(dict(kind="document", document=text, purpose=text, aggregate=text, **common))
            cases.append(dict(kind="identity", entity=text, **common))
            for raw in [False, True]:
                cases.append(dict(kind="external", raw=raw, token=text, **common))
    times = [["0", 0], ["-1", 123000000], ["59", 1000000000], ["59", 1999999999],
             ["3600", 123456000], ["3600", 123456789], ["253402300800", 0],
             ["-62167219200", 0], ["-62198755200", 0]]
    for index, (text, issued) in enumerate(itertools.product(strings, times)):
        expires = times[(index + 1) % len(times)]
        cases.append(dict(kind="claims", entity=text, id=str((1 << 64) - 1), version=str(-(1 << 63)),
                          issued=issued, expires=expires, purpose=text, key_version=(1 << 32) - 1))
        cases.append(dict(kind="resolved", entity=text, id="0", version="-1", raw=bool(index % 2),
                          key=None if index % 3 == 0 else text,
                          issued=None if index % 4 == 0 else issued,
                          expires=None if index % 5 == 0 else expires))
    cases += [dict(kind="profile", variant=i) for i in range(3)]
    cases += [dict(kind="mode", raw=raw) for raw in [False, True]]
    cases.append(dict(kind="wire"))
    cases += [dict(kind="token_error", variant=i) for i in range(2)]
    cases += [dict(kind="round_error", variant=i) for i in range(5)]
    for key, byte in itertools.product(["k1", "valid-key_2", "K" * 64], [0, 0x11, 0x7b, 0xff]):
        cases.append(dict(kind="key", key=key, byte=byte))
    for text in strings:
        cases.append(dict(kind="notice", level="ERROR", message=text, profile="development",
                          telemetry_key="teaql.reference.mode", telemetry_value="raw"))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify_upstream.py")], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    if not same(outputs, observe(args.rust, cases)):
        raise RuntimeError("Repeated original model observations disagree")
    corpus = dict(baseline=json.loads((root / "upstream.lock.json").read_text())["commit"],
                  note="Actual original Debug/Clone/PartialEq, both Error displays/empty sources, redacted key diagnostics and UTC fields. Public model fields bypass constructor validation exactly as in Rust; no protocol algorithm is copied.",
                  cases=[dict(input=value, expected=output) for value, output in zip(cases, outputs)])
    path = Path(__file__).with_name("reference_model_cases.json.gz")
    with gzip.GzipFile(filename=str(path), mode="wb", mtime=0) as stream:
        stream.write(json.dumps(corpus, ensure_ascii=True, separators=(",", ":")).encode())
    print(f"Recorded {len(cases)} twice-agreeing original reference model observations")


if __name__ == "__main__":
    main()
