"""Record complete original policy model derives, constructors and defaults."""
import argparse
import copy
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same


def identity(id="policy", version="1", fingerprint="sha256:x"):
    return dict(id=id, version=version, fingerprint=fingerprint)


def inputs():
    cases = []
    base = identity()
    for field, text in itertools.product(["id", "version", "fingerprint"],
                                         ["", " \t", "\u2003", " a ", "用户🧊\0", "x" * 512]):
        value = dict(base)
        value[field] = text
        cases.append(dict(value, type="identity", other=base))
    for changed in [base, identity(id="other"), identity(version="2"), identity(fingerprint="sha256:y")]:
        cases.append(dict(base, type="identity", other=changed))
    # Constructor failure precedence with multiple blank fields.
    cases += [dict(type="identity", other=base, **identity(id="", version="", fingerprint="")),
              dict(type="identity", other=base, **identity(version="", fingerprint=""))]
    cases += [dict(type="decision", deny=deny, code=code, message=message, fields=fields)
              for deny, code, message, fields in itertools.product(
                  [False, True], ["", "\u2003", " DENIED ", "code\0"], ["", "用户🧊"],
                  [[], ["name", "items[0].amount", "name"]])]
    cases += [dict(type="approval", policy=base, approver=approver, seconds=str(seconds), nanos=nanos)
              for approver, (seconds, nanos) in itertools.product(
                  ["", "\u2003", " approver ", "用户🧊"], [(0, 0), (-1, 999999999), (253402300800, 123456789)])]
    values = [dict(kind="null"), dict(kind="typed_null"), dict(kind="u64", value=str((1 << 64) - 1)),
              dict(kind="f64", value="NaN"), dict(kind="object", value={"nested": dict(kind="text", value="用户🧊")})]
    for kind, value, reason in itertools.product(["Create", "Update", "Delete", "Recover"], values, [None, "", "reason🧊"]):
        operation = dict(kind=kind, key=dict(entity="Order", id=dict(kind="u64", value="7")),
                         version="-3", values={"z": value, "a": dict(kind="i64", value="17")})
        cases.append(dict(type="models", execution="exec-17", request="Order.saveGraph", root="Order",
                          reason=reason, operations=[operation], customer=True, policy=base,
                          approval="Approved", warnings=[], warning="MUTATION-POLICY-002", first=True))
    cases += [dict(type="models", execution="", request="", root="", reason=None, operations=[],
                   customer=customer, policy=policy, approval=approval, warnings=warnings,
                   warning="warning\0", first=first)
              for customer, policy, approval, warnings, first in itertools.product(
                  [False, True], [None, base], ["NotApplicable", "Missing", "Approved"],
                  [[], ["MUTATION-POLICY-001", "MUTATION-POLICY-002"]], [False, True])]
    cases.append(dict(type="defaults", policy=base))
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
        raise RuntimeError("Repeated original policy model outputs disagree")
    corpus = dict(baseline=json.loads((root / "upstream.lock.json").read_text())["commit"],
                  note="Actual original model constructors, full derived diagnostics in nine modes, Clone/equality/order and hash-set capability, source blank validation/error ordering and empty registry/approval defaults. Hash outputs are native process artifacts, not a persisted cross-language byte protocol.",
                  cases=[dict(input=data, expected=out) for data, out in zip(cases, outputs)])
    with gzip.GzipFile(filename=str(Path(__file__).with_name("policy_model_cases.json.gz")), mode="wb", mtime=0) as stream:
        stream.write(json.dumps(corpus, separators=(",", ":"), ensure_ascii=True).encode())
    print(f"Recorded {len(cases)} twice-agreeing original policy model inputs")


if __name__ == "__main__":
    main()
