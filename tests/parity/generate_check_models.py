"""Record original checker model diagnostics and complete wire serialization."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same


def inputs():
    cases = [dict(rule=rule, profile=profile, path=path)
             for rule, profile, path in itertools.product(range(8), ["", "camelCase", "snake_case", "PascalCase"],
                 [[], ["order_items", 17, "a~/ß_field"], ["", 2147483647], ["用户_🧊", "_leading__tail_"]])]
    values = [dict(kind="null"), dict(kind="typed_null"), dict(kind="bool", value=True),
              dict(kind="i64", value="-9223372036854775808"), dict(kind="u64", value="18446744073709551615"),
              dict(kind="f64", value="NaN"), dict(kind="f64", value="-0"), dict(kind="decimal", value="1.00"),
              dict(kind="text", value="用户🧊\0"), dict(kind="json", value={"z": 1, "a": [None, "text"]}),
              dict(kind="object", value={"child": dict(kind="text", value="nested")})]
    cases += [dict(rule=rule, profile=profile, path=["some_field", 1], input=value,
                   system=dict(kind="u64", value="17"), message=message, entity="Item", source="/prior~0/path")
              for rule, profile, value, message in itertools.product([0, 3], ["camelCase", "PascalCase"], values, ["", "message🧊"])]
    cases += [dict(rule=0, profile=profile, path=[]) for profile in ["CamelCase", " snake_case", "unknown", "🧊"]]
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
        raise RuntimeError("Repeated original checker model outputs disagree")
    corpus = dict(baseline=json.loads((root / "upstream.lock.json").read_text())["commit"],
                  note="Actual original CheckResult/ObjectLocation/model methods, source derived diagnostics in nine modes, Clone/PartialEq and complete serde WireCheckResult. Native-width path indices stay in the common host/MIPS range; input/system values include full-width numbers and non-finite floats. No diagnostic timestamp normalization.",
                  cases=[dict(input=data, expected=out) for data, out in zip(cases, outputs)])
    with gzip.GzipFile(filename=str(Path(__file__).with_name("check_model_cases.json.gz")), mode="wb", mtime=0) as stream:
        stream.write(json.dumps(corpus, separators=(",", ":"), ensure_ascii=True).encode())
    print(f"Recorded {len(cases)} twice-agreeing original checker model cases")


if __name__ == "__main__":
    main()
