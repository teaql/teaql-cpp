#!/usr/bin/env python3
"""Execute complete original metadata tests and actual descriptor observations."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_core_primitives_reference_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def boundary_inputs():
    text = ['', '\0tail', '雪😀"\\\n\t', 'long_' + 'x' * 384 + '雪']
    cases = []
    for data_type in range(10):
        for mode in range(8):
            value = text[mode % len(text)]
            cases.append(dict(type=data_type, mode=mode, entity='Entity' + value,
                              property='field' + value, column=value, relation='relation' + value,
                              target='Target' + value, local=value, foreign='foreign' + value,
                              table=value, service=None if mode == 0 else value,
                              length=None if mode == 0 else 0 if mode == 1 else 4294967295,
                              precision=None if mode == 0 else 4294967295,
                              scale=None if mode < 2 else 0 if mode == 2 else 4294967295,
                              masks=None if mode == 0 else [] if mode == 1 else [value, value, 'field'],
                              max=None if mode == 0 else 0 if mode == 1 else
                              18446744073709551615 if mode >= 4 else 4294967295))
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--module-build-dir', type=Path)
    parser.add_argument('--rust-target', type=Path, default=Path('/tmp/teaql_cpp_core_baselines_506'))
    parser.add_argument('--record-new-corpus', action='store_true')
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh receipt path; existing evidence is immutable')
    corpus_path = PARITY / 'core_metadata_source_boundaries_5_0_6_cases.json.gz'
    if args.record_new_corpus and corpus_path.exists():
        parser.error('The boundary corpus already exists; reproduce instead of overwriting')
    record = dict(complete=False, full_goal_accepted=False, steps=[], matrices=[],
                  scope='Complete meta.rs constructors/builders, fields/defaults, ordered lookups, '
                        'Clone/PartialEq/Eq/Debug and the three original tests. Native-only JSON '
                        'aliases/required lookups and whole generated SQL/Save remain separate.')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    environment = dict(os.environ, CARGO_TARGET_DIR=str(args.rust_target.resolve()),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')

    def run(command, input_text=None):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, env=environment, input=input_text,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    run(['python3', ROOT / 'scripts/verify_upstream.py'])
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    reference = ROOT / '.reference/teaql-rs'
    assert registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock')
    unit_output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                       reference / 'teaql-core/Cargo.toml', '--lib', 'meta::tests'])
    assert '3 passed; 0 failed; 0 ignored;' in unit_output
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml',
         '--bin', 'meta_api_reference_5_0_6'])
    original = args.rust_target.resolve() / 'debug/meta_api_reference_5_0_6'
    frontends = [('original', original), ('native', args.build_dir.resolve() / 'teaql_meta_api_probe')]
    if args.module_build_dir:
        frontends.append(('modules', args.module_build_dir.resolve() / 'modules/teaql_module_meta_api_probe'))
    for _, executable in frontends:
        if not executable.is_file():
            raise FileNotFoundError(f'Build the actual observer first: {executable}')
    previous_path = PARITY / 'meta_api_5_0_6_cases.json.gz'
    previous = json.loads(gzip.decompress(previous_path.read_bytes()))
    assert previous['baseline'] == baseline and len(previous['cases']) == 344
    assert digest(ROOT / previous['driver']) == previous['driver_sha256']
    retained_inputs = [case['input'] for case in previous['cases']]
    retained_outputs = [case['expected'] for case in previous['cases']]
    new_outputs = None
    for name, inputs, expected in [('retained', retained_inputs, retained_outputs),
                                   ('boundaries', boundary_inputs(), None)]:
        payload = ''.join(json.dumps(value, ensure_ascii=True) + '\n' for value in inputs)
        matrix = dict(name=name, cases=len(inputs), inputs=inputs, outputs={})
        for frontend, executable in frontends:
            output = run([executable], payload)
            assert output.endswith('\n'), f'Missing terminal LF: {frontend}'
            actual = [json.loads(line) for line in output.split('\n')[:-1]]
            assert len(actual) == len(inputs), f'Incomplete observation: {frontend}'
            matrix['outputs'][frontend] = dict(binary_sha256=digest(executable), values=actual)
            if expected is None:
                expected = actual
            if not same(expected, actual):
                record['matrices'].append(matrix)
                save()
                raise RuntimeError(f'Original descriptor observations differ: {name}/{frontend}')
            print(f'{name}: {frontend} {len(inputs)}/{len(inputs)} exact outcomes', flush=True)
        record['matrices'].append(matrix)
        save()
        if name == 'boundaries':
            new_outputs = expected
    corpus = dict(baseline=baseline, scope='Native64 source usize maximum, all ten DataTypes, '
                  'builder flags and embedded-NUL/Unicode/empty/long descriptor strings. No normalization.',
                  cases=[dict(input=value, expected=output)
                         for value, output in zip(boundary_inputs(), new_outputs)])
    corpus_bytes = gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n').encode(), mtime=0)
    if args.record_new_corpus:
        corpus_path.write_bytes(corpus_bytes)
    else:
        assert corpus_path.read_bytes() == corpus_bytes, 'Complete boundary corpus bytes differ'
    paths = [reference / 'teaql-core/src/meta.rs', reference / 'teaql-core/src/naming.rs',
             reference / 'teaql-core/Cargo.toml', reference / 'Cargo.lock',
             PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock',
             ROOT / previous['driver'], PARITY / 'rust_values/src/bin/meta_api_oracle.rs',
             PARITY / 'meta_api_probe.cpp', ROOT / 'teaql_core/tests/meta_api_tests.cpp',
             ROOT / 'teaql_core/include/teaql/core/meta.hpp',
             ROOT / 'teaql_core/include/teaql/core/meta_debug.hpp', ROOT / 'teaql_core/src/core.cpp',
             PARITY / 'fixture_value.hpp', PARITY / 'debug_modes.hpp', Path(__file__)]
    record.update(baseline=baseline, original_unit_tests=3, observations_per_frontend=424,
                  retained_corpus_sha256=digest(previous_path), boundary_corpus_sha256=digest(corpus_path),
                  reference_registry_versions_checksums_exact=True,
                  sources={str(path.resolve()): dict(sha256=digest(path), text=path.read_text()) for path in paths},
                  complete=True)
    save()
    print('Complete metadata source observations pass; whole generator/governed execution remain separate.')


if __name__ == '__main__':
    main()
