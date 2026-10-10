"""Record original entity-runtime model methods and derived/custom diagnostics."""
import argparse
import copy
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same


def value(kind, data=None):
    return dict(kind=kind, **({"value": data} if data is not None else {}))


def key(data, entity="Item"):
    return dict(entity=entity, id=data)


def inputs():
    values = [value("null"), value("typed_null"), value("bool", False), value("bool", True),
              value("i64", "-9223372036854775808"), value("i64", "-1"), value("i64", "0"),
              value("i64", "1"), value("u64", "1"), value("u64", "18446744073709551615"),
              value("f64", "-0"), dict(kind="f64", value="NaN", bits="7ff8000000000000"), value("f64", "inf"),
              value("decimal", "1.00"), value("text", 'quoted "value"\n边缘'),
              value("date", "2026-10-08"), value("timestamp", "-1"),
              value("json", {"a": 1, "b": False}), value("object", {"x": value("u64", "1")}),
              value("list", [value("u64", "1")])]
    cases = []
    for item, entity in itertools.product(values, ["Item", "边缘🧊", "x" * 256]):
        first = key(item, entity)
        for other in [copy.deepcopy(first), key(value("u64", "2"), entity)]:
            cases.append(dict(mode="key", key=first, other=other))
        changed = copy.deepcopy(first)
        changed["mutate_id"] = value("text", "modified public field")
        cases.append(dict(mode="key", key=changed, other=first))
    first, second = key(value("u64", "1")), key(value("i64", "2"))
    set_first = dict(op="set", key=first, field="field", value=value("text", "value"))
    set_second = dict(op="set", key=second, field="another", value=value("list", values[:4]))
    for steps in [[], [dict(op="push")], [set_first], [set_first, dict(op="push"), set_second],
                  [set_first, dict(op="push"), set_second, dict(op="pop")],
                  [set_first, dict(op="clear")], [set_first, dict(op="clear_entity", key=first)],
                  [set_first, dict(op="push"), set_first, set_second]]:
        cases.append(dict(mode="stack", steps=steps))
    cases += [dict(mode="error", variant=i) for i in [0, 1]]
    for entity, target, source in itertools.product(["Item", "边缘🧊", 'quoted "entity"\n'],
                                                  [-(1 << 63), 0, (1 << 63) - 1], [-1, 1]):
        cases.append(dict(mode="error", variant=2, entity=entity, id="18446744073709551615",
                          target=str(target), source=str(source)))
    actions = [[], [dict(op="comment", value="token=raw")], [set_first],
               [set_first, dict(op="push"), set_second],
               [dict(op="version", key=first, value="-1"), dict(op="version", key=second, value="7")],
               [dict(op="new", key=first), dict(op="delete", key=second)],
               [set_first, dict(op="snapshot", entity="Item", values=[["id", value("u64", "1")],
                                                                      ["version", value("i64", "9")]])],
               [set_first, dict(op="comment", value="reason"), dict(op="commit")]]
    cases += [dict(mode="state", actions=steps) for steps in actions]
    for list_values, option, steps in itertools.product([None, [], ["payload"]],
                                                       [False, None, "payload"], actions[:4]):
        relations = []
        if list_values is not None:
            relations.append(dict(owner="Item", id="1", name="items", list=True, values=list_values))
        if option is not False:
            relations.append(dict(owner="Item", id="1", name="optional", list=False, value=option))
        cases.append(dict(mode="graph", owner="Item", owner_id="1", actions=steps,
                          entities=[dict(id="1", integer=False, value="one"),
                                    dict(id="1", integer=False, value="replacement"),
                                    dict(id="2", integer=True, value=42)], relations=relations))
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
        raise RuntimeError("Repeated original entity-runtime observations disagree")
    corpus = dict(baseline=json.loads((root / "upstream.lock.json").read_text())["commit"],
                  note="Actual original constructors, public state transitions and Debug/Clone/equality/Error behavior in nine formats. Private graph/ledger state is observed through original Debug; fixture builders do not duplicate these algorithms. Native counterpart is pending.",
                  cases=[dict(input=case, expected=output) for case, output in zip(cases, outputs)])
    path = Path(__file__).with_name("entity_runtime_model_cases.json.gz")
    with gzip.GzipFile(filename=str(path), mode="wb", mtime=0) as stream:
        stream.write(json.dumps(corpus, ensure_ascii=True, separators=(",", ":")).encode())
    print(f"Recorded {len(cases)} twice-agreeing original entity-runtime model observations")


if __name__ == "__main__":
    main()
