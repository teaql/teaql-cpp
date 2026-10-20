#!/usr/bin/env python3
"""Compare current resource bindings and generated Checker results with ownership adaptations."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('resource_bindings_5_0_6_cases.json.gz')


PROFILES = (
    ('check',),
    ('typed', 'check', 'typed', 'check'),
    ('named', 'check', 'named', 'check'),
    ('typed', 'check', 'named', 'check', 'other', 'check', 'typed', 'check', 'named', 'check'),
    ('other', 'check', 'named', 'check', 'other', 'check'),
    ('typed', 'named', 'check', 'typed', 'other', 'check', 'named', 'typed', 'check'),
)

def cases():
    for module, actions, lookup, delta, rounds, text_mode in itertools.product(
            ('plain', 'checker', 'behavior', 'combined'), PROFILES,
            ('typed', 'named'), (1, 9), (1, 2), (0, 1)):
        tag = 'resource-' + ('r' * 128 if text_mode == 0 else '\u96ea' * 64)
        key = 'settings-' + ('k' * 64 if text_mode == 0 else '\0' + '\u96ea' * 32)
        yield dict(module=module, actions=list(actions), lookup=lookup, delta=delta,
                   rounds=rounds, tag=tag, key=key)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    if args.record and not args.rust:
        parser.error('--record requires actual unchanged original execution')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    producer = json.loads((ROOT / 'tests/generator/parity/constants_numeric_rust/PROVENANCE.json').read_text())
    original = ROOT / 'tests/generator/parity/constants_numeric_rust'
    if (producer['runtime_commit'] != baseline or producer['hand_edited']
            or producer['model_sha256'] != hashlib.sha256((ROOT / producer['model']).read_bytes()).hexdigest()
            or {p.relative_to(original).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in original.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'} != producer['files']):
        raise RuntimeError('Complete original resource binding fixture changed')
    cpp_domain = ROOT / 'tests/generated_constants_numeric'
    native = json.loads((cpp_domain / 'manifest.json').read_text())
    if (native['runtime_rust_baseline'] != baseline or native['model_sha256'] != producer['model_sha256']
            or native['generator_reference_commit'] != producer['generator_commit']
            or {p.relative_to(cpp_domain).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in cpp_domain.rglob('*') if p.is_file() and p.name != 'manifest.json'} != native['files']):
        raise RuntimeError('Complete native resource binding fixture changed')
    inputs = list(cases())
    captured = {}
    def observed(executable, language):
        result = subprocess.run([executable], input=''.join(json.dumps(value) + '\n' for value in inputs),
                                text=True, capture_output=True)
        captured[language] = result.stderr
        if result.returncode:
            raise RuntimeError(f'{language} resource observer failed ({result.returncode}): {result.stderr}')
        values = [json.loads(line) for line in result.stdout.split('\n') if line]
        if len(values) != len(inputs):
            raise RuntimeError('Resource binding output is incomplete')
        return values
    if args.record:
        expected = observed(args.rust, 'rust')
    else:
        with gzip.open(CORPUS, 'rt') as file:
            corpus = json.load(file)
        if corpus['baseline'] != baseline or not same(corpus['original_provenance'], producer):
            raise RuntimeError('Resource binding corpus source differs')
        if not same(inputs, [case['input'] for case in corpus['cases']]):
            raise RuntimeError('Resource binding matrix differs')
        expected = [case['expected'] for case in corpus['cases']]
        if args.rust and not same(observed(args.rust, 'rust'), expected):
            raise RuntimeError('Resource binding corpus differs from live original')
    actual = observed(args.cpp, 'cpp')
    gaps = [dict(index=i,input=c,rust=a,cpp=b) for i,(c,a,b) in enumerate(zip(inputs,expected,actual)) if not same(a['semantic'],b['semantic'])]
    for value, source, input in zip(actual, expected, inputs):
        round_groups = (source['semantic']['rounds'], value['semantic']['rounds'],
                        source['ownership'], value['ownership'])
        if any(not isinstance(rounds, list) or len(rounds) != input['rounds']
               for rounds in round_groups):
            raise RuntimeError('Resource binding round/ownership record count differs from the request')
        total = 0
        for original_round, native_round, original_ownership, native_ownership in zip(
                source['semantic']['rounds'], value['semantic']['rounds'], source['ownership'], value['ownership']):
            previous = total
            typed_live = named_live = False
            source_drops = []
            for action in input['actions']:
                if action in ('typed', 'named', 'other'):
                    total += 1
                    if action == 'typed': typed_live = True
                    else: named_live = True
                source_drops.append(total - int(typed_live) - int(named_live))
            if (original_ownership['during'] != source_drops
                    or native_ownership['during'] != [previous] * len(input['actions'])
                    or original_round['created'] != total or native_round['created'] != total
                    or original_round['destroyed_after_close'] != total
                    or native_round['destroyed_after_close'] != total):
                raise RuntimeError('Declared resource ownership/destruction contract failed')
        for round in value['ownership']:
            if (not round['expired_rejected'] or not round['expired_copy_rejected']
                    or round['captured'] != round['old_borrow_live_before_close']):
                raise RuntimeError('Native resource borrow lifecycle failed')
    if gaps:
        path = ROOT / 'docs/parity/resource_bindings_5_0_6_initial_gaps.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress((json.dumps(dict(count=len(gaps),differences=gaps),indent=2)+'\n').encode(),mtime=0))
        print(json.dumps(gaps[0],indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)} resource binding differences')
    if args.record:
        corpus=dict(baseline=baseline,original_provenance=producer,
                    scope='Complete unchanged original Numeric module/Checker/Context APIs. Exact current typed/named bindings, wrong-type shadowing, key bytes, nested payloads, Checker hooks/scalars/errors and final destructor totals. Rust Box replacement drops immediately; C++ arena objects remain until Context close. Per-step ownership timing and raw typed RTTI/path error names are separately retained language adaptations. Native borrows reject get/copy after actual User deletion. No SQL or audited persistence is performed here.',
                    cases=[dict(input=c,expected=v) for c,v in zip(inputs,expected)])
        CORPUS.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
        path = ROOT / 'docs/parity/resource_bindings_5_0_6_native_ownership.json.gz'
        path.write_bytes(gzip.compress((json.dumps(dict(cases=[dict(input=c,ownership=v['ownership'])
                            for c,v in zip(inputs,actual)]),indent=2)+'\n').encode(),mtime=0))
        path = ROOT / 'docs/parity/resource_bindings_5_0_6_raw_lookup_diagnostics.json.gz'
        path.write_bytes(gzip.compress((json.dumps(dict(scope='Raw language-specific typed missing-resource names; named errors remain in semantic comparison.',stderr=captured),indent=2)+'\n').encode(),mtime=0))
    print(f'Resource bindings: {len(inputs)}/{len(inputs)} exact override/resource outcomes')


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as error:
        raise SystemExit(f'Extension observer failed ({error.returncode}): {error.stderr}') from error
