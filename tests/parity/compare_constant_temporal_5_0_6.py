#!/usr/bin/env python3
"""Compare complete generated temporal bootstrap under the existing test wall clock."""
from compare_values import diagnostic_path
import argparse
import gzip
import hashlib
import itertools
import json
import os
import subprocess
from pathlib import Path

from compare_values import same
from sample_clock_driver import observe_fixed

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('constant_temporal_5_0_6_cases.json.gz')


def cases():
    for state, module, rounds, logging, wall, business, tick in itertools.product(
            ('empty', 'existing', 'corrupt', 'missing', 'deleted', 'extra'),
            ('plain', 'checker', 'behavior', 'combined'), (1, 2), (False, True),
            (0, 1001, 1790812800123), (-1, 946684800456, 1704067200789), (0, 1000)):
        yield dict(state=state, module=module, rounds=rounds, logging=logging,
                   wall_millis=wall, business_millis=business, tick_per_round=tick)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--clock', required=True)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    if args.record and not args.rust:
        parser.error('--record requires the actual original Rust executable')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    original = ROOT / 'tests/generator/parity/constants_temporal_required_rust'
    provenance = json.loads((original / 'PROVENANCE.json').read_text())
    if provenance['runtime_commit'] != baseline or provenance['hand_edited']:
        raise RuntimeError('Temporal constant fixture baseline differs')
    if provenance['model_sha256'] != hashlib.sha256((ROOT / provenance['model']).read_bytes()).hexdigest():
        raise RuntimeError('Temporal constant fixture model changed')
    actual_original = {p.relative_to(original).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                       for p in original.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'}
    if actual_original != provenance['files']:
        raise RuntimeError('Temporal constant original file set or bytes changed')
    cpp = ROOT / 'tests/generated_constants_temporal_required'
    cpp_manifest = json.loads((cpp / 'manifest.json').read_text())
    if (cpp_manifest['runtime_rust_baseline'] != baseline or cpp_manifest['model_sha256'] != provenance['model_sha256']
            or cpp_manifest['generator_reference_commit'] != provenance['generator_commit']):
        raise RuntimeError('Temporal C++ domain provenance differs')
    if {p.relative_to(cpp).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in cpp.rglob('*') if p.is_file() and p.name != 'manifest.json'} != cpp_manifest['files']:
        raise RuntimeError('Temporal C++ domain file set or bytes changed')
    inputs = list(cases())
    if args.record:
        expected = observe_fixed(args.rust, inputs, args.clock)
    else:
        with gzip.open(CORPUS, 'rt') as file:
            corpus = json.load(file)
        if (corpus['baseline'] != baseline or not same(corpus['provenance'], provenance)
                or not same(inputs, [row['input'] for row in corpus['cases']])):
            raise RuntimeError('Temporal bootstrap corpus source or inputs differ')
        expected = [row['expected'] for row in corpus['cases']]
        if args.rust and not same(observe_fixed(args.rust, inputs, args.clock), expected):
            raise RuntimeError('Temporal corpus differs from live unchanged original')
    actual = observe_fixed(args.cpp, inputs, args.clock)
    differences = [dict(index=i, input=value, rust=source, cpp=native)
                   for i, (value, source, native) in enumerate(zip(inputs, expected, actual))
                   if not same(source, native)]
    if differences:
        path = diagnostic_path(ROOT / 'docs/parity/constant_temporal_5_0_6_initial_gaps.json')
        if not path.exists():
            path.write_text(json.dumps(dict(count=len(differences), differences=differences), indent=2) + '\n')
        print(json.dumps(differences[0], indent=2))
        raise SystemExit(f'{len(differences)}/{len(inputs)} temporal bootstrap differences')
    if args.record:
        corpus = dict(baseline=baseline, provenance=provenance,
                      scope='Complete unchanged original bootstrap; exact physical rows/versions/ID floors/root/errors, Fix evidence and SQL/parameter/intent diagnostics. Existing Linux process clock controls only CLOCK_REALTIME; no timestamp normalization or generated-source patch.',
                      cases=[dict(input=value, expected=source) for value, source in zip(inputs, expected)])
        CORPUS.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
    print(f'Temporal constants: {len(inputs)}/{len(inputs)} exact original/native outcomes')


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as error:
        raise SystemExit(f'Temporal observer failed ({error.returncode}): {error.stderr}') from error
