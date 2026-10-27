#!/usr/bin/env python3
"""Compare actual original E calls across mandatory and nullable scalar types."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path

from compare_values import observe, same
from expression_diagnostic_framing import to_source

ROOT = Path(__file__).resolve().parents[2]
PREFIX = Path(__file__).with_name('expression_model_types_5_0_6_cases.json.gz')
CORPUS = Path(__file__).with_name('expression_model_types_expanded_5_0_6_cases.json.gz')


def scalar(kind, value=None):
    return dict(kind=kind, value=value)


def inputs():
    values = {
        'text': [scalar('text', ''), scalar('text', 'Unicode 数据\n"quoted"\0' + 'x' * 1200)],
        'bool': [scalar('bool', False), scalar('bool', True)],
        'decimal': [scalar('decimal', '0'), scalar('decimal', '-12.5000')],
        'float': [scalar('f64', '-0.0'), scalar('f64', '1.5')],
        'integer': [scalar('i64', '0'), scalar('i64', '-9223372036854775808')],
        'id': [scalar('u64', '0'), scalar('u64', '18446744073709551615')],
        'date': [scalar('date', '1970-01-01'), scalar('date', '2024-02-29')],
        'timestamp': [scalar('timestamp', '0'), scalar('timestamp', '-1')],
        'json': [scalar('json', None), scalar('json', {'nested': [False, 0, None, '数据']})],
    }
    models = {
        'Person': {'id': 'id', 'email': 'text', 'active': 'bool', 'nickname': 'text',
                   'balance': 'decimal', 'height': 'float', 'ratio': 'float', 'age': 'integer',
                   'birthday': 'date', 'updated_at': 'timestamp', 'payload': 'json',
                   'version': 'integer'},
        'Reading': {'id': 'id', 'text': 'text', 'active': 'bool', 'balance': 'decimal',
                    'height': 'float', 'ratio': 'float', 'count': 'integer', 'day': 'date',
                    'recorded_at': 'timestamp', 'payload': 'json', 'version': 'integer'},
        'Device': {'owner_id': 'id'},
    }
    cases = []
    for model, fields in models.items():
        for field, kind in fields.items():
            fallback = values[kind][1]
            for value in [*values[kind], scalar('null'), scalar('typed_null')]:
                cases.append(dict(model=model, field=field, loaded=True, value=value,
                                  fallback=fallback))
            cases.append(dict(model=model, field=field, loaded=False, value=values[kind][1],
                              fallback=fallback))
    conversions = [
        scalar('text', '12'), scalar('bool', False), scalar('bool', True),
        *[scalar('i64', str(v)) for v in [-9223372036854775808, -1, 0, 1, 2, 9223372036854775807]],
        *[scalar('u64', str(v)) for v in [0, 1, 2, 18446744073709551615]],
        *[scalar('f64', v) for v in ['-0.0', '1.5', '1e39', '1e300', 'NaN', 'inf', '-inf']],
        *[scalar('decimal', v) for v in ['-1', '1', '2.5', '10000.0000', '18446744073709551616']],
        scalar('date', '2024-02-29'), scalar('timestamp', '-1'),
        scalar('json', None), scalar('json', {'nested': [False, 0, '数据']}),
    ]
    for model, fields in models.items():
        for field, kind in fields.items():
            cases.extend(dict(model=model, field=field, loaded=True, value=value,
                              fallback=values[kind][1]) for value in conversions)
    return cases


def adapted(value):
    if isinstance(value, list):
        return [adapted(v) for v in value]
    if isinstance(value, dict):
        if set(value) == {'panic'}:
            raw = value['panic']
            if isinstance(raw, dict):
                return dict(panic=to_source(raw))
            # Existing C++ generic null-resolution error is a language adaptation.
            # Only this exact known string maps; arbitrary errors remain failures.
            if raw == 'Expression value is legitimately null':
                return dict(panic='Value was legitimately null in database!')
        return {k: adapted(v) for k, v in value.items()}
    return value


def producer_integrity():
    result = {}
    for fixture in ['types_rust', 'optional_types_rust']:
        directory = ROOT / 'tests/generator/parity' / fixture
        manifest = json.loads((directory / 'PROVENANCE.json').read_text())
        if not (hashlib.sha256((ROOT / manifest['model']).read_bytes()).hexdigest() == manifest['model_sha256']):
            raise AssertionError()
        for name, digest in manifest['files'].items():
            if not (hashlib.sha256((directory / name).read_bytes()).hexdigest() == digest):
                raise AssertionError(name)
        result[fixture] = manifest
    ordinary = (ROOT / 'tests/generator/parity/types_rust/lib/src/e.rs').read_text()
    optional = (ROOT / 'tests/generator/parity/optional_types_rust/lib/src/e.rs').read_text()
    helper = ordinary[ordinary.index('pub fn trigger_logic_bug_panic'):]
    if not (helper == optional[optional.index('pub fn trigger_logic_bug_panic'):]):
        raise AssertionError()
    return result, hashlib.sha256(helper.encode()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    producers, helper = producer_integrity()
    cases = inputs()
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    if args.record:
        if not (args.rust and not CORPUS.exists()):
            raise AssertionError('Capture requires actual Rust and a fresh corpus')
        expected = observe(args.rust, cases)
        prefix = json.loads(gzip.decompress(PREFIX.read_bytes()))
        if not (same(cases[:len(prefix['cases'])], [c['input'] for c in prefix['cases']])):
            raise AssertionError()
        if not (same(expected[:len(prefix['cases'])], [c['expected'] for c in prefix['cases']])):
            raise AssertionError()
        corpus = dict(baseline=baseline, producers=producers, original_shared_helper_sha256=helper,
                      cases=[dict(input=i, expected=o) for i, o in zip(cases, expected)])
        CORPUS.write_bytes(gzip.compress(json.dumps(corpus, ensure_ascii=False).encode(), mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        if not (corpus['baseline'] == baseline and corpus['producers'] == producers):
            raise AssertionError()
        if not (corpus['original_shared_helper_sha256'] == helper):
            raise AssertionError()
        if not (same(cases, [c['input'] for c in corpus['cases']])):
            raise AssertionError('Declared matrix changed')
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original source observations changed')
    actual = observe(args.cpp, cases)
    failures = [i for i, (e, a) in enumerate(zip(expected, actual)) if not same(e, adapted(a))]
    if args.record:
        path = ROOT / 'docs/parity/expression_model_types_expanded_initial_5_0_6.json.gz'
        if not (not path.exists()):
            raise AssertionError()
        path.write_bytes(gzip.compress(json.dumps(dict(
            baseline=baseline, matching=len(cases)-len(failures), total=len(cases),
            failures=failures, cpp_actual=actual, original=expected,
            adaptation='Actual native frames map only documented C++ spelling/language and exact generic scalar-null message.'
        ), ensure_ascii=False).encode(), mtime=0))
    for i in failures[:8]:
        print(f'Case {i}: {cases[i]}\nRust: {expected[i]}\nC++: {actual[i]}')
    print(f'Generated E model-type parity: {len(cases)-len(failures)}/{len(cases)}; '
          f'{sum(5 for e in expected if "decode_error" not in e)} actual scalar resolutions; '
          f'{sum(1 for e in expected if "decode_error" in e)} decode rejections')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
