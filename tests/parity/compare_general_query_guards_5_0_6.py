#!/usr/bin/env python3
"""Compare complete constructor/selection guard branches with original requests."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import sys
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

CORPUS = Path(__file__).with_name('general_query_guards_5_0_6_cases.json.gz')
DOMAINS = {'dynamic': ('rust_request_state_5_0_6', 'dynamic_json_rust',
                       ['platform', 'person', 'order', 'product']),
           'inherited': ('rust_dynamic_json_privacy_5_0_6', 'inheritance_graph_rust',
                         ['platform', 'specialist', 'school', 'academy', 'research_academy', 'enrollment'])}


def inputs():
    cases = []
    for domain, (_, _, entities) in DOMAINS.items():
        for entity, entry, selection in itertools.product(entities,
                ['bare', 'default', 'minimal', 'with_children'],
                ['none', 'self', 'self_fields', 'without_parent', 'all', 'children', 'any']):
            for mode, marker in [('construct', 'model'), ('construct', 'integer'),
                                 ('construct', 'custom'), ('rows', 'model'), ('list', 'model')]:
                cases.append(dict(domain=domain, entity=entity, entry=entry,
                                  selection=selection, mode=mode, marker=marker))
    assert len(cases) == 1400
    return cases


def identity():
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    wanted = versions(reference)
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    domains = {}
    paths = [ROOT / 'tests/parity/general_query_guard_observer_5_0_6.rs',
             ROOT / 'tests/parity/rust_field_layout/src/bin/original_like_privacy_5_0_6.rs']
    for domain, (crate, generated, _) in DOMAINS.items():
        project = ROOT / 'tests/parity' / crate
        assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference)
        for name, values in versions(project / 'Cargo.lock').items():
            if name not in {'teaql_request_state_506', 'teaql_dynamic_json_privacy_506',
                            'cpp-dynamic-json-core', 'cpp-inheritance-graph-core'}:
                assert values <= wanted.get(name, set()), name
        sdk = ROOT / 'tests/generator/parity' / generated
        provenance = json.loads((sdk / 'PROVENANCE.json').read_text())
        assert provenance['runtime_commit'] == baseline
        assert hashlib.sha256((ROOT / provenance['model']).read_bytes()).hexdigest() == provenance['model_sha256']
        for name, digest in provenance['files'].items():
            assert hashlib.sha256((sdk / name).read_bytes()).hexdigest() == digest, name
        domains[domain] = provenance
        paths.extend([project / 'Cargo.toml', project / 'Cargo.lock', project / 'src/bin/general_query_guards.rs'])
    paths.extend(ROOT / 'tests/support' / name for name in
                 ['dynamic_json_schema.sql', 'dynamic_json_dataset.sql', 'dynamic_json_inherited_rows.sql'])
    return dict(baseline=baseline, domains=domains,
                sources={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths})


def original(args, cases):
    outputs = []
    for domain in DOMAINS:
        exe = getattr(args, 'rust_' + domain)
        assert exe, domain
        outputs.extend(observe(exe, [case for case in cases if case['domain'] == domain]))
    return outputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp')
    parser.add_argument('--rust-dynamic')
    parser.add_argument('--rust-inherited')
    parser.add_argument('--record-source', action='store_true')
    parser.add_argument('--record-comparison', type=Path)
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    current, cases = identity(), inputs()
    if args.record_source:
        assert not CORPUS.exists()
        expected = original(args, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(**current, cases=[
            dict(input=i, expected=e) for i, e in zip(cases, expected)]), ensure_ascii=False).encode(), mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        for key, value in current.items():
            assert corpus[key] == value, key
        assert same(cases, [row['input'] for row in corpus['cases']])
        expected = [row['expected'] for row in corpus['cases']]
        if args.rust_dynamic or args.rust_inherited:
            if not same(expected, original(args, cases)):
                raise RuntimeError('Original guard observations changed')
    if not args.cpp:
        assert args.record_source
        print(f'Recorded {len(cases)} original constructor/selection guard outcomes')
        return 0
    actual = observe(args.cpp, cases)
    failures = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    if args.record_comparison:
        assert not args.record_comparison.exists()
        args.record_comparison.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,
            original=expected, cpp=actual, failures=failures), ensure_ascii=False).encode(), mtime=0))
    for index in failures[:3]:
        print('Case', index, cases[index])
        print('Different fields:', [key for key in expected[index] if not same(expected[index][key], actual[index][key])])
    print(f'General query guards: {len(cases)-len(failures)}/{len(cases)} exact original outcomes')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
