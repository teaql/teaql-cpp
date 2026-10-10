"""Record unchanged original public in-memory query/evaluator observations."""
import argparse
import itertools
import json
from pathlib import Path
import random
import subprocess
from compare_values import observe
from compare_inmemory import target_input


def v(kind, value=None):
    out = dict(kind=kind)
    if value is not None:
        out["value"] = value
    return out


def literal(value):
    return dict(kind="value", literal=value)


def column(name="x"):
    return dict(kind="column", name=name)


def binary(left, op, right):
    return dict(kind="binary", args=[left, right], op=op)


def unary(kind, *args):
    return dict(kind=kind, args=list(args))


def inputs():
    cases = []
    values = [v("null"), v("typed_null"), v("bool", False), v("bool", True),
              v("i64", "-1"), v("i64", "1"), v("i64", "9223372036854775807"),
              v("u64", "1"), v("u64", "18446744073709551615"), v("f64", "1"),
              v("f64", "NaN"), v("f64", "inf"), v("decimal", "1.00"),
              v("text", "1"), v("date", "2026-10-06"), v("timestamp", "0"),
              v("json", {"x": 1}), v("list", [v("i64", "1")]),
              v("object", {"x": v("i64", "1")})]
    for left, right, op in itertools.product(values, values, range(12)):
        cases.append(dict(op="eval", row=[["x", left]], expr=binary(column(), op, literal(right))))
    for value in values + [None]:
        row = [] if value is None else [["x", value]]
        terms = [column(), unary("isnull", column()), unary("notnull", column()),
                 unary("between", column(), literal(v("i64", "0")), literal(v("u64", "10"))),
                 unary("function", column()), dict(kind="subquery"),
                 unary("and"), unary("or")]
        for term in terms:
            for expr in [term, unary("not", term), unary("and", literal(v("bool", True)), term),
                         unary("or", literal(v("bool", False)), term)]:
                cases.append(dict(op="eval", row=row, expr=expr))
    texts = ["", "a", "b", "aa", "ab", "ba", "a_b", "a%b", "a\\b", "\0", "中", "😀", "中😀", "é", "a\n中"]
    patterns = ["".join(chars) for size in range(4) for chars in itertools.product("ab%_", repeat=size)]
    patterns += ["中_", "_😀", "%中%", "%😀", "a\\%", "e_", "a%中", "%\0%"]
    for text, pattern in itertools.product(texts, patterns):
        cases.append(dict(op="eval", row=[["x", v("text", text)]],
                          expr=binary(column(), 6, literal(v("text", pattern)))))

    sample = [[["id", v("u64", str(i + 1))], ["name", v("text", name)], ["age", v("i64", str(age))]]
              for i, (name, age) in enumerate([("Alice", 30), ("Bob", 25), ("Charlie", 35)])]
    def query(rows, **q):
        cases.append(dict(op="query", rows=rows, query=q))
    # Original 19 source-test behaviors, with evaluator cases above completing
    # AND/OR/NOT, null, BETWEEN and IN assertions.
    query(sample)
    query(sample, filter=binary(column("name"), 0, literal(v("text", "Bob"))))
    query(sample, filter=binary(column("age"), 2, literal(v("i64", "28"))))
    for desc in [False, True]:
        query(sample, order=[["age", desc]])
    query(sample, slice=[1, 1])
    query(sample, projection=["name"])
    for func in range(5):
        query(sample, aggregates=[[func, "*" if func == 0 else "age", "answer"]])
    for pattern in ["A%", "_ob", "%li%"]:
        query(sample, filter=binary(column("name"), 6, literal(v("text", pattern))))
    for term in [unary("and", binary(column("age"), 2, literal(v("i64", "26"))),
                       binary(column("age"), 4, literal(v("i64", "34")))),
                 unary("or", binary(column("name"), 0, literal(v("text", "Alice"))),
                       binary(column("name"), 0, literal(v("text", "Bob")))),
                 unary("not", binary(column("name"), 0, literal(v("text", "Bob"))))]:
        query(sample, filter=term)
    # Strict grouping (including typed null, cross-variant numbers and NaN),
    # all aggregates, missing fields, encounter order and alias collisions.
    heterogeneous = [[ ["group", value], ["x", value], ["seq", v("i64", str(i))] ]
                     for i, value in enumerate(values + values + [v("null")])]
    heterogeneous += [[], [["group", v("text", "missing x")]]]
    for rows, group, field in itertools.product([[], sample, heterogeneous], [[], ["group"], ["group", "x"]], ["x", "*", "missing"]):
        query(rows, groups=group, aggregates=[[i, field, f"agg{i}"] for i in range(12)])
    for groups in [[], ["group"], ["group", "group"]]:
        query(heterogeneous, groups=groups,
              aggregates=[[0, "*", "group"], [1, "x", "same"], [0, "x", "same"]],
              projection=["absent"], order=[["same", True]], slice=[500, 0],
              having=literal(v("bool", False)), comment="ignored purpose", hard_limit=1)
    for offset, limit in itertools.product([0, 1, 3, 4, 2**32, 2**64-1], [None, 0, 1, 3, 2**32, 2**64-1]):
        query(sample, slice=[offset, limit], order=[["age", False]], projection=["name", "id", "missing", "name"])
    duplicate = [[["z", v("i64", "1")], ["a", v("i64", "2")], ["z", v("i64", "3")]]]
    for projection in [[], ["z"], ["z", "a", "z", "missing"], ["missing"]]:
        query(duplicate, projection=projection)
    for ignored_expr in [column("age"), unary("function", column("age"))]:
        query(sample, order=[["", True, ignored_expr]])
    query(sample, groups=["name"], hard_limit=1, having=literal(v("bool", False)), comment="not metadata")
    # Stable sorting under total comparisons, both sides of the small-sort
    # threshold and larger PMR merge buffers; deliberate non-total inputs use
    # only the source's small insertion-sort path.
    rng = random.Random(505)
    for size, desc in itertools.product([0, 1, 2, 19, 20, 21, 32, 64, 129], [False, True]):
        rows = [[["group", v("i64", str(rng.randrange(5)))], ["seq", v("i64", str(i))]] for i in range(size)]
        query(rows, order=[["group", desc]])
        query(rows, order=[["group", desc], ["seq", not desc]])
    for value in values:
        rows = [[["x", v("null")]], [["x", value]], [], [["x", value]]]
        for desc in [False, True]:
            query(rows, order=[["x", desc]])
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify-upstream.py")], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    # Isolate Rust's architecture-dependent pagination casts; the engine body
    # remains unchanged. Record only the cases whose 32-bit input differs.
    narrowed = [(i, target_input(case, 32)) for i, case in enumerate(cases)
                if target_input(case, 32) != case]
    outputs_32 = dict(zip((i for i, _ in narrowed), observe(args.rust, [c for _, c in narrowed])))
    lock = json.loads((root / "upstream.lock.json").read_text())
    corpus = dict(baseline=lock["commit"],
        note="Direct unchanged public InMemoryQueryEngine and ExprEvaluator calls. Debug Value output retains variants, TypedNull, nonfinite floats and row layout. Timestamps are compared by ordering, not nondeterministic clock values. The 64-bit host oracle records 32-bit expectations by narrowing only offset/limit exactly as the source's explicit `as usize` casts before unchanged engine calls. This is not an original Rust MIPS execution. Non-total comparisons have no portable general sort-order guarantee; larger sort cases have total keys.",
        cases=[dict(input=case, expected=output,
                    **({"expected_32": outputs_32[i]} if i in outputs_32 else {}))
               for i, (case, output) in enumerate(zip(cases, outputs))])
    Path(__file__).with_name("inmemory_cases.json").write_text(json.dumps(corpus, indent=2) + "\n")
    print(f"Recorded {len(cases)} in-memory engine/evaluator observations")


if __name__ == "__main__":
    main()
