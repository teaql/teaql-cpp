"""Record unchanged original facet/restriction/purpose calls, not copied algorithms."""
import argparse
import copy
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe


def literal(kind="u64", value="7"):
    return dict(kind="value", value=dict(kind=kind, value=value))


def predicate(name="id", op=0, value=None):
    return dict(kind="binary", left=dict(kind="column", name=name), op=op,
                right=value or literal())


def query(entity="Parent", rich=False):
    q = dict(entity=entity)
    if rich:
        q.update(hard_limit=777, projection=["id", "name"],
                 expr_projection=[dict(alias="calculated", expr=literal())], search="search text",
                 filter=predicate(), having=predicate("count", 2),
                 order=[dict(field="name", direction=1)], slice=dict(offset=2, limit=3),
                 partition="root_id", threshold=17,
                 aggregates=[dict(function=0, field="id", alias="count")], group=["name"],
                 relations=[dict(name="existing", query=query("Existing"))],
                 cache=dict(enabled=True, age=100, propagate=True, propagate_age=200),
                 comment="existing comment", trace=[dict(kind=7, entity=entity, id=5, comment="original purpose")],
                 raw_sql="SELECT original", raw_criteria=["original criterion"],
                 dynamic=[dict(alias="dynamic", sql="original dynamic")],
                 raw_projection=[dict(alias="raw", sql="original projection")],
                 object_groups=[dict(property="object", storage="object_id", query=query("Object"))],
                 children=[query("PreviousChild")], stream=23,
                 continuous=dict(namespace="cursor", ttl=60), ids=dict(namespace="ids", ttl=61, max=100))
    return q


def selection(entity="Child", rich=False):
    s = dict(query=query(entity, rich), options={})
    if rich:
        s.update(selections=[dict(name="selected", selection=selection("Selected"))],
                 filters=[dict(name="filtered", selection=selection("Filtered"))],
                 children=[selection("Enhanced", False)])
        s["options"] = dict(comment="option comment", raw_sql="SELECT option", raw_criteria=["option criterion"],
                            dynamic=[dict(alias="option_dynamic", sql="option dynamic")],
                            raw_projection=[dict(alias="option_raw", sql="option projection")],
                            object_groups=[dict(property="grouped", storage="group_id", selection=selection("Grouped"))])
    return s


def descriptors(parent="Parent", local="id", foreign="parent_id"):
    return [dict(name=parent, table="parents", service="edge", declared=True,
                 masked=["secret"], audit_max=21,
                 properties=[dict(name="id", type=2, id=True), dict(name="name", type=5, length=128)],
                 relations=[dict(name="kids", target="Child", local=local, foreign=foreign,
                                 many=True, attach=False, delete_missing=False)])]


def facet(name="facet", all=False, rich=False, aggregates=False):
    s = selection("Child", rich)
    if aggregates:
        s["options"]["aggregates"] = [dict(relation="parents", alias="parents", single=False,
            selection=selection("Parent", True)), dict(relation="other", alias="other", single=True,
            selection=selection("Other", True))]
    return dict(name=name, relation="kids", selection=s, all=all)


def case(op="execute", outer=None, **kwargs):
    return dict(op=op, outer=outer or query(), descriptors=descriptors(), cancel=False, **kwargs)


def inputs():
    cases = []
    for rich, all, aggregates, filtered in itertools.product([False, True], repeat=4):
        outer = query(rich=rich)
        if filtered:
            outer["filter"] = predicate()
        else:
            outer.pop("filter", None)
        cases.append(case(outer=outer, options=dict(facets=[facet(all=all, rich=rich, aggregates=aggregates)])))
    for name, parent, local, foreign in itertools.product(
            ["", "facet", "same\0name", "facet \u4e2d"], ["Parent", "parent", "\u4e2dRoot"],
            ["id", "root-key", ""], ["parent_id", ""]):
        j = case(outer=query(parent, True), options=dict(facets=[facet(name, rich=True, aggregates=True)]))
        j["descriptors"] = descriptors(parent, local, foreign)
        cases.append(j)
    for metadata, relation in itertools.product([[], descriptors(), descriptors("Other")], ["kids", "missing", ""]):
        j = case(op="restrict", outer=query(rich=True), selection=selection(rich=True), relation=relation)
        j["descriptors"] = metadata
        cases.append(j)
        for all in [False, True]:
            f = facet(all=all, rich=True, aggregates=True)
            f["relation"] = relation
            j = case(outer=query(rich=True), options=dict(facets=[f]))
            j["descriptors"] = metadata
            cases.append(j)
    filters = [None, predicate(), dict(kind="and", args=[]), dict(kind="or", args=[]),
               dict(kind="and", args=[predicate(), predicate("version", 2)]),
               dict(kind="not", args=[predicate("name", value=literal("text", "secret"))])]
    for outer_filter, inner_filter, all in itertools.product(filters, filters, [False, True]):
        outer = query(rich=True)
        if outer_filter is None:
            outer.pop("filter", None)
        else:
            outer["filter"] = outer_filter
        f = facet(all=all, rich=True, aggregates=True)
        f["selection"]["query"].pop("filter", None)
        if inner_filter is not None:
            f["selection"]["query"]["filter"] = inner_filter
            f["selection"]["options"]["aggregates"][0]["selection"]["query"]["filter"] = inner_filter
        cases.append(case(outer=outer, options=dict(facets=[f])))
    for names, fail_at, pending_at, cancel in itertools.product(
            [[], ["single"], ["z", "a", "z"], ["same", "same", "last"]],
            [None, 0, 1, 2], [None, 0, 1, 2], [False, True]):
        j = case(outer=query(rich=True), options=dict(facets=[facet(n, all=True, rich=True) for n in names]))
        j["cancel"] = cancel
        if fail_at is not None:
            j["fail_at"] = fail_at
        if pending_at is not None:
            j["pending_at"] = pending_at
        cases.append(j)
    for field, value in query(rich=True).items():
        outer = query()
        outer[field] = value
        cases.append(case(op="restrict", outer=outer, selection=selection(rich=True), relation="kids"))
    for purpose in ["", " ", "\t\n", "\u2003", "\u00a0", " normal ", "\0", "reason\0tail", "purpose \u4e2d", "\u200b"]:
        cases.append(case(op="purpose", outer=query(rich=True), purpose=purpose))
    for count in [1, 3]:
        j = case(outer=query(rich=True), options=dict(facets=[facet(str(i), rich=True) for i in range(count)]))
        j["options"]["facets"][-1]["relation"] = "missing"
        cases.append(j)
    for all, aggregates, root, duplicate in itertools.product([False, True], [False, True], [1, 2, 3], [False, True]):
        parent = descriptors()[0]
        parent["properties"] = [dict(name="id", type=2, id=True), dict(name="label", type=5)]
        child = dict(name="Child", table="children", properties=[dict(name="id", type=2, id=True),
                     dict(name="parent_id", type=2), dict(name="label", type=5)],
                     relations=[dict(name="parent", target="Parent", local="parent_id", foreign="id")])
        f = facet(all=all)
        f["selection"]["query"].update(projection=["id", "parent_id", "label"], order=[dict(field="id", direction=0)])
        if aggregates:
            agg = selection("Parent")
            agg["query"]["aggregates"] = [dict(function=0, field="id", alias="count")]
            f["selection"]["options"]["aggregates"] = [dict(relation="parent", alias="parent_count", selection=agg, single=False)]
        facets = [f]
        if duplicate:
            second = copy.deepcopy(f)
            second["selection"]["query"]["filter"] = predicate("id", value=literal("u64", "11"))
            facets.append(second)
        j = case(op="sqlite", outer=dict(entity="Parent", filter=predicate("id", value=literal("u64", str(root)))),
                 options=dict(facets=facets))
        j["descriptors"] = [parent, child]
        cases.append(j)
    # Deduplicate exact scripts while preserving deterministic fixture order.
    unique = {}
    for j in cases:
        unique.setdefault(json.dumps(j, sort_keys=True), j)
    return list(unique.values())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify_upstream.py")], check=True)
    scripts = inputs()
    outputs = observe(args.rust, scripts)
    lock = json.loads((root / "upstream.lock.json").read_text())
    corpus = dict(baseline=lock["commit"],
                  note="Direct unchanged public execute_facets, restrict_facet_to_outer_query, PurposedQuery and PurposedSelectQuery calls. Recording runtime, metadata and suspension fixtures only; no copied support algorithms. Complete query Debug captures all core fields. Provider rows include nested values/facets. Cancellation is original future drop; failures retain typed category and Display.",
                  cases=[dict(input=i, expected=o) for i, o in zip(scripts, outputs)])
    Path(__file__).with_name("generated_support_cases.json").write_text(json.dumps(corpus, indent=2) + "\n")
    print(f"Recorded {len(scripts)} public generated-support scripts")


if __name__ == "__main__":
    main()
