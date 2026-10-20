#!/usr/bin/env python3
"""Decode and read back actual generated typed JSON scalar and graph projections."""
import argparse
import copy
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
DRIVER = Path(__file__).parent / "rust_field_layout/src/bin/generated_inherited_json_5_0_6.rs"
TARGET = Path(__file__).with_name("generated_inherited_json_5_0_6_cases.json.gz")


def inputs():
    result = []
    for entity in ('School', 'Academy', 'ResearchAcademy', 'Specialist', 'Enrollment', 'Platform'):
        values = [None, True, 1, [], {}, {'id': 7}, {'id': 7, 'version': 3},
                  {'id': 7, '_note': {'x': [1, None]}}, {'id': 7, '_type': 'ResearchAcademy'},
                  {'id': 7, 'unknown': 'PRIVATE_LITERAL'}, {'id': 7, '_is_new': True}]
        names = {
            'School': ['name', 'rank', 'advisor_id', 'platform_id'],
            'Academy': ['name', 'rank', 'advisor_id', 'platform_id', 'campus_code'],
            'ResearchAcademy': ['name', 'rank', 'advisor_id', 'platform_id', 'campus_code', 'budget'],
            'Specialist': ['name', 'active', 'platform_id'],
            'Enrollment': ['note', 'school_id'], 'Platform': ['name'],
        }[entity]
        for field in names:
            for value in (None, True, 0, -1, 2**64-1, 'text', '12.50', {}, []):
                values.append({'id': 7, 'version': 3, field: value})
        relations = {
            'School': ['advisor', 'platform', 'enrollment_list'],
            'Academy': ['advisor', 'platform'], 'ResearchAcademy': ['advisor', 'platform'],
            'Specialist': ['platform', 'school_list'], 'Enrollment': ['school'],
            'Platform': ['school_list', 'specialist_list'],
        }[entity]
        for relation in relations:
            for value in (None, {}, [], {'id': 7}, {'id': 7, 'version': 3, 'name': 'nested'},
                          [{'id': 7}], [None]):
                values.append({'id': 7, relation: value})
        for value in values:
            result.append(dict(entity=entity, array=False, value=value))
        for array in ([], [{'id': 7}, {'id': 8}], [{'id': 7, 'name': 'one'}, {'id': 7, 'rank': 2}],
                      [values[-1], values[-2]], [{'id': 7}, True]):
            result.append(dict(entity=entity, array=True, value=array))
    for entity in ('School', 'Academy', 'ResearchAcademy'):
        for advisor, platform, selected in itertools.product((None, 7, 8), (None, 1, 2), (False, True)):
            value = dict(id=7, version=3, name='root', rank=2, advisor_id=advisor, platform_id=platform)
            if entity != 'School':
                value['campus_code'] = 'north'
            if entity == 'ResearchAcademy':
                value['budget'] = '12.50'
            if selected:
                value['advisor'] = None if advisor is None else dict(id=advisor, name='advisor', platform=dict(id=1))
                value['platform'] = None if platform is None else dict(id=platform, name='tenant')
            result.append(dict(entity=entity, array=False, value=value))
    result.extend([
        dict(entity='School', array=False, value=dict(id=7, enrollment_list=[dict(id=21, school=dict(id=7))])),
        dict(entity='School', array=False, value=dict(id=7, enrollment_list=[dict(id=21, school_id=8)])),
        dict(entity='Enrollment', array=False, value=dict(id=21, school=dict(id=7, name='school', advisor=dict(id=2)))),
        dict(entity='Academy', array=False, value=dict(id=7, enrollment_list=[])),
        dict(entity='ResearchAcademy', array=False, value=dict(id=7, enrollment_list=[])),
    ])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust", required=True)
    parser.add_argument("--cpp")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    subprocess.run(["python3", str(ROOT / "scripts/verify_upstream.py")], check=True)
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    stimuli = inputs()
    outputs = observe(args.rust, stimuli)
    corpus = dict(baseline=lock["commit"], version=lock["version"], driver=str(DRIVER.relative_to(ROOT)),
                  driver_sha256=hashlib.sha256(DRIVER.read_bytes()).hexdigest(),
                  note="Unchanged original inheritance_graph domain module, Context JSON decoding and derived Entity presentation/load/dirty status across three inheritance levels. No output normalization or persistence authority qualification.",
                  cases=[dict(input=i, expected=o) for i, o in zip(stimuli, outputs)])
    if args.check:
        with gzip.open(TARGET, "rt", encoding="utf-8") as f:
            assert same(corpus, json.load(f)), "Corpus differs from actual source"
    else:
        TARGET.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + "\n").encode(), mtime=0))
    if args.cpp:
        actual = observe(args.cpp, stimuli)
        gaps = [dict(index=n, input=i, rust=a, cpp=b)
                for n, (i, a, b) in enumerate(zip(stimuli, outputs, actual)) if not same(a, b)]
        if gaps:
            path = ROOT / "docs/parity/generated_inherited_json_5_0_6_initial_gaps.json"
            if not path.exists():
                path.write_text(json.dumps(gaps, indent=2) + "\n")
            raise SystemExit(f"{len(gaps)}/{len(stimuli)} generated typed JSON observations differ")
    print(f"Reproduced {len(stimuli)} actual inherited JSON observations" + (" and C++" if args.cpp else ""))


if __name__ == "__main__":
    main()
