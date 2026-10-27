#!/usr/bin/env python3
"""Compare actual original request-marker semantics through generated C++ facades."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import sys

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

CORPUS = Path(__file__).with_name('query_result_marker_5_0_6_cases.json.gz')


def inputs():
    return [dict(entity=entity, minimal=minimal, marker=marker, text=text, shape=shape)
            for entity, minimal, marker, text, shape in itertools.product(
                ['person', 'order'], [False, True], ['integer', 'tuple', 'custom'],
                ['ordinary', '', 'Long owned "雪" NUL\0 % _ \\'],
                ['plain', 'options', 'child', 'combined'])]


def identity():
    project = ROOT / 'tests/parity/rust_request_state_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    if not (registry_identities(project / 'Cargo.lock') <= registry_identities(reference)):
        raise AssertionError()
    wanted = versions(reference)
    for name, values in versions(project / 'Cargo.lock').items():
        if name not in {'teaql_request_state_506', 'cpp-dynamic-json-core'}:
            if not (values <= wanted.get(name, set())):
                raise AssertionError(name)
    generated = ROOT / 'tests/generator/parity/dynamic_json_rust'
    provenance = json.loads((generated / 'PROVENANCE.json').read_text())
    for name, expected in provenance['files'].items():
        if not (hashlib.sha256((generated / name).read_bytes()).hexdigest() == expected):
            raise AssertionError(name)
    if not (hashlib.sha256((ROOT / provenance['model']).read_bytes()).hexdigest() == provenance['model_sha256']):
        raise AssertionError()
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    if not (provenance['runtime_commit'] == baseline):
        raise AssertionError()
    paths = [project / 'Cargo.toml', project / 'Cargo.lock', project / 'src/bin/query_result_marker.rs']
    return dict(baseline=baseline, producer=provenance,
                sources={str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                         for path in paths})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp')
    parser.add_argument('--rust')
    parser.add_argument('--record-source', action='store_true')
    parser.add_argument('--record-comparison', type=Path)
    args = parser.parse_args()
    current, cases = identity(), inputs()
    if args.record_source:
        if not (args.rust and not CORPUS.exists()):
            raise AssertionError()
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(**current, cases=[
            dict(input=i, expected=e) for i, e in zip(cases, expected)]), ensure_ascii=False).encode(), mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        for key, value in current.items():
            if not (corpus[key] == value):
                raise AssertionError(key)
        if not (same(cases, [row['input'] for row in corpus['cases']])):
            raise AssertionError()
        expected = [row['expected'] for row in corpus['cases']]
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original marker observations changed')
    if not args.cpp:
        if not (args.record_source):
            raise AssertionError()
        print(f'Recorded {len(cases)} original marker observations')
        return 0
    actual = observe(args.cpp, cases)
    failures = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    if args.record_comparison:
        if not (not args.record_comparison.exists()):
            raise AssertionError()
        args.record_comparison.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,
            original=expected, cpp=actual, failures=failures), ensure_ascii=False).encode(), mtime=0))
    for i in failures[:3]:
        print('Case', i, cases[i])
        print('Different fields:', [key for key in expected[i] if not same(expected[i][key], actual[i][key])])
    print(f'Query result markers: {len(cases)-len(failures)}/{len(cases)} exact original outcomes')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
