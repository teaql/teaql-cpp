"""Record actual original raw/safe event derives and safe projection."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same


def case(**updates):
    values = {"id": dict(kind="u64", value="1001"), "name": dict(kind="text", value="用户🧊 name"),
              "secret": dict(kind="text", value="business-secret"), "password": dict(kind="text", value="credential-secret")}
    out = dict(kind="created", entity="Order", values=values, old=None, new=values,
               fields=["name", "secret", "password", "absent", "name"], id=dict(kind="u64", value="1001"),
               version="-3", count=7, trace=True, bootstrap=False, governance=False,
               mask=["secret"], maximum=None, plaintext=False)
    out.update(updates)
    return out


def inputs():
    cases = [case(kind=kind, bootstrap=bootstrap, governance=governance)
             for kind, bootstrap, governance in itertools.product(
                 ["created", "updated", "updated_old", "deleted", "deleted_old", "recovered", "recovered_old",
                  "schema_created", "schema_verified", "field_added", "data_seeded"], [False, True], [False, True])]
    cases += [case(kind="updated_old", old=old, maximum=maximum, plaintext=plaintext)
              for old, maximum, plaintext in itertools.product(
                  [None, {}, {"name": dict(kind="text", value="old🧊"), "secret": dict(kind="text", value="previous-secret")}],
                  [0, 3, 4, 8, None], [False, True])]
    values = [dict(kind="null"), dict(kind="typed_null"), dict(kind="bool", value=True),
              dict(kind="i64", value="-9223372036854775808"), dict(kind="u64", value="18446744073709551615"),
              dict(kind="f64", value="NaN"), dict(kind="decimal", value="1.00"),
              dict(kind="object", value={"api_key": dict(kind="text", value="nested-secret")}),
              dict(kind="json", value={"token": "json-secret"}),
              dict(kind="list", value=[dict(kind="text", value="payload🧊")])]
    cases += [case(values={"value": value, "_hidden": dict(kind="text", value="internal")}, new={"value": value},
                   kind=kind, mask=[], governance=True)
              for kind, value in itertools.product(["created", "updated", "updated_old"], values)]
    cases += [case(values={}, new={}, old={}, trace=trace, count=count, kind=kind)
              for kind, trace, count in itertools.product(["schema_created", "data_seeded"], [False, True], [0, 2147483647])]
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
        raise RuntimeError("Original event model outputs differ across repeats")
    corpus = dict(baseline=json.loads((root / "upstream.lock.json").read_text())["commit"],
                  note="Actual original public event constructors, source Clone/PartialEq and full derived Debug in nine modes, with real build_safe_event results. Exact source sensitive-plaintext acknowledgement is toggled per input; credentials remain governed by original privacy code. Native-width schema counts stay within the shared host/MIPS range; existing separate width tests cover their platform profiles. No diagnostic timestamp normalization.",
                  cases=[dict(input=data, expected=out) for data, out in zip(cases, outputs)])
    with gzip.GzipFile(filename=str(Path(__file__).with_name("event_model_cases.json.gz")), mode="wb", mtime=0) as stream:
        stream.write(json.dumps(corpus, separators=(",", ":"), ensure_ascii=True).encode())
    print(f"Recorded {len(cases)} twice-agreeing original event model cases")


if __name__ == "__main__":
    main()
