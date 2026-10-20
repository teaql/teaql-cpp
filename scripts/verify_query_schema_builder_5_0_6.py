#!/usr/bin/env python3
"""Reproduce the original readonly query schema builder and Context adaptation."""
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
PROJECT = PARITY / 'rust_query_schema_builder_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from query_schema_builder_inputs_5_0_6 import cases


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--module-build-dir', type=Path)
    parser.add_argument('--record-new-corpus', action='store_true')
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    corpus_path = PARITY / 'query_schema_builder_5_0_6_cases.json.gz'
    if args.record_new_corpus and corpus_path.exists():
        parser.error('Do not overwrite the original corpus')
    record = dict(complete=False, full_goal_accepted=False, whole_source_accepted=False,
                  scope='Readonly query schema construction, immutable sharing, independent '
                        'replacement, Clone and list preparation. Context lifetime/copy/failure '
                        'controls remain explicit native contracts. Not whole query/runtime '
                        'or G13/G17/P7 acceptance.', steps=[], outputs={})

    def save():
        record_bytes = (json.dumps(record, indent=2) + '\n').encode()
        args.record.write_bytes(gzip.compress(record_bytes, compresslevel=1, mtime=0))

    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')

    def run(command, payload=None):
        command = [str(part) for part in command]
        result = subprocess.run(command, cwd=ROOT, env=environment, text=True,
                                input=payload, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        if payload is None or result.returncode:
            save()
        result.check_returncode()
        return result.stdout

    run(['python3', ROOT / 'scripts/verify_upstream.py'])
    reference = ROOT / '.reference/teaql-rs'
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock')
    unit_output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                       reference / 'teaql-core/Cargo.toml', '--lib'])
    assert '69 passed; 0 failed; 0 ignored;' in unit_output
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml'])
    inputs = cases()
    assert len(inputs) == 480
    record['inputs'] = inputs
    payload = ''.join(json.dumps(value, ensure_ascii=True) + '\n' for value in inputs)
    expected = None
    if not args.record_new_corpus:
        previous = json.loads(gzip.decompress(corpus_path.read_bytes()))
        assert previous['baseline'] == baseline
        assert [case['input'] for case in previous['cases']] == inputs
        expected = [case['expected'] for case in previous['cases']]
    original = Path(environment['CARGO_TARGET_DIR']) / 'debug/query_schema_builder_source_506'
    executables = [('original', original),
                   ('native', args.build_dir.resolve() / 'teaql_query_schema_builder_probe')]
    if args.module_build_dir:
        executables.append(('modules', args.module_build_dir.resolve() /
                            'modules/teaql_module_query_schema_builder_probe'))
    for label, executable in executables:
        output = run([executable], payload)
        assert output.endswith('\n'), 'Do not accept incomplete observer output'
        actual = [json.loads(line) for line in output.split('\n')[:-1]]
        assert len(actual) == len(inputs)
        record['outputs'][label] = dict(binary_sha256=digest(executable), count=len(actual),
                                        output_step=len(record['steps']) - 1)
        if expected is None:
            expected = actual
        differences = [dict(index=i, input=value, original=left, cpp=right)
                       for i, (value, left, right) in enumerate(zip(inputs, expected, actual))
                       if not same(left, right)]
        if differences:
            record['differences'] = differences
            save()
            raise RuntimeError(f'Actual schema-builder source differs: {label}')
        print(f'Schema builder: {label} {len(inputs)}/{len(inputs)} exact outcomes', flush=True)
    corpus = dict(baseline=baseline, scope=record['scope'],
                  cases=[dict(input=value, expected=result) for value, result in zip(inputs, expected)])
    corpus_bytes = gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n').encode(), mtime=0)
    if args.record_new_corpus:
        corpus_path.write_bytes(corpus_bytes)
    else:
        assert corpus_path.read_bytes() == corpus_bytes, 'Source corpus bytes must reproduce'
    paths = [reference / 'teaql-core/src/query.rs', reference / 'teaql-core/src/dynamic_properties.rs',
             reference / 'Cargo.lock', PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock',
             PARITY / 'query_schema_builder_5_0_6.rs', PARITY / 'query_schema_builder_probe.cpp',
             PARITY / 'query_schema_builder_inputs_5_0_6.py', PARITY / 'debug_modes.hpp',
             PARITY / 'fixture_value.hpp', ROOT / 'teaql_core/include/teaql/core/query.hpp',
             ROOT / 'teaql_core/src/query.cpp', ROOT / 'teaql_core/tests/query_schema_builder_tests.cpp',
             ROOT / 'teaql_core/include/teaql/core/dynamic_properties.hpp',
             ROOT / 'teaql_core/src/dynamic_properties.cpp',
             ROOT / 'teaql_memory/include/teaql/memory/scoped.hpp',
             ROOT / 'teaql_memory/include/teaql/memory/arena.hpp', ROOT / 'teaql_memory/src/arena.cpp',
             Path(__file__).resolve()]
    record.update(complete=True, observations=len(inputs), corpus_sha256=digest(corpus_path),
                  sources={str(path.relative_to(ROOT)): dict(sha256=digest(path), text=path.read_text())
                           for path in paths})
    save()
    print('Schema-builder source replay complete; whole query source remains unaccepted.', flush=True)


if __name__ == '__main__':
    main()
