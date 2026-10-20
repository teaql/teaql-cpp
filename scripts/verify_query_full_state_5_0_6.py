#!/usr/bin/env python3
"""Replay independent mutations of all 31 locked SelectQuery fields."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_query_full_state_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def query_fields(diagnostic):
    """Split only the outer Debug struct; never normalize a field's value."""
    prefix = 'SelectQuery { '
    assert diagnostic.startswith(prefix) and diagnostic.endswith(' }')
    body = diagnostic[len(prefix):-2]
    parts = []
    depth = 0
    quoted = False
    escaped = False
    start = 0
    for index, character in enumerate(body):
        if quoted:
            if escaped:
                escaped = False
            elif character == '\\':
                escaped = True
            elif character == '"':
                quoted = False
        elif character == '"':
            quoted = True
        elif character in '([{':
            depth += 1
        elif character in ')]}':
            depth -= 1
            assert depth >= 0
        elif character == ',' and depth == 0:
            parts.append(body[start:index].strip())
            start = index + 1
    assert depth == 0 and not quoted
    parts.append(body[start:].strip())
    pairs = [part.split(': ', 1) for part in parts]
    assert len({pair[0] for pair in pairs}) == len(pairs)
    return dict(pairs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--module-build-dir', type=Path)
    parser.add_argument('--record-new-corpus', action='store_true')
    parser.add_argument('--record-new-row-corpus', action='store_true')
    parser.add_argument('--record-new-error-corpus', action='store_true')
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    corpus_path = PARITY / 'query_full_state_5_0_6_cases.json.gz'
    row_path = PARITY / 'row_extend_error_5_0_6_cases.json.gz'
    error_path = PARITY / 'dynamic_field_error_debug_5_0_6_cases.json.gz'
    if args.record.exists() or (args.record_new_corpus and corpus_path.exists()):
        parser.error('Use fresh evidence paths')
    if args.record_new_row_corpus and row_path.exists():
        parser.error('Do not overwrite the row source corpus')
    if args.record_new_error_corpus and error_path.exists():
        parser.error('Do not overwrite the error source corpus')
    record = dict(complete=False, full_goal_accepted=False, whole_source_accepted=False,
                  scope='Every SelectQuery field: independent equality/Clone changes, '
                        'complete Debug, immutable metadata sharing and actual recursive '
                        'list preparation. Whole query.rs includes separate row/layout '
                        'responsibilities and is not accepted by this observer alone.',
                  steps=[], outputs={})

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(),
                                             compresslevel=1, mtime=0))

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
    source = (reference / 'teaql-core/src/query.rs').read_text()
    fields = re.findall(r'^    pub ([a-z_]+):', source.split('pub struct SelectQuery {', 1)[1]
                        .split('\n}', 1)[0], re.M)
    assert len(fields) == 31
    record['source_fields'] = fields
    unit_output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                       reference / 'teaql-core/Cargo.toml', '--lib'])
    assert '69 passed; 0 failed; 0 ignored;' in unit_output
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml',
         '--bins', '--example', 'row_extend_error'])
    inputs = [dict(profile=profile, field=field) for profile in range(4) for field in range(31)]
    payload = ''.join(json.dumps(value) + '\n' for value in inputs)
    record['inputs'] = inputs
    expected = None
    if not args.record_new_corpus:
        previous = json.loads(gzip.decompress(corpus_path.read_bytes()))
        assert previous['baseline'] == baseline and previous['fields'] == fields
        assert [case['input'] for case in previous['cases']] == inputs
        expected = [case['expected'] for case in previous['cases']]
    executables = [('original', Path(environment['CARGO_TARGET_DIR']) / 'debug/query_full_state_source_506'),
                   ('native', args.build_dir.resolve() / 'teaql_query_full_state_probe')]
    if args.module_build_dir:
        executables.append(('modules', args.module_build_dir.resolve() /
                            'modules/teaql_module_query_full_state_probe'))
    for label, executable in executables:
        output = run([executable], payload)
        assert output.endswith('\n'), 'Incomplete query observer output'
        actual = [json.loads(line) for line in output.split('\n')[:-1]]
        assert len(actual) == len(inputs)
        for input_value, case in zip(inputs, actual):
            assert case['self_equal'] and case['equal_clone'] and case['base_unchanged']
            assert not case['changed_equal_base'], 'A declared field was omitted from equality'
            before = query_fields(case['base']['compact'])
            after = query_fields(case['changed']['compact'])
            assert set(before) == set(after) == set(fields)
            changed_fields = {field for field in fields if before[field] != after[field]}
            assert changed_fields == {fields[input_value['field']]}, changed_fields
        record['outputs'][label] = dict(binary_sha256=digest(executable), count=len(actual),
                                        output_step=len(record['steps']) - 1)
        if expected is None:
            expected = actual
        differences = [dict(index=i, input=value, original=a, cpp=b)
                       for i, (value, a, b) in enumerate(zip(inputs, expected, actual)) if not same(a, b)]
        if differences:
            record['differences'] = differences
            save()
            raise RuntimeError(f'Full query source differs: {label}')
        print(f'Complete query state: {label} {len(inputs)}/{len(inputs)} exact outputs', flush=True)
    corpus = dict(baseline=baseline, fields=fields, scope=record['scope'],
                  cases=[dict(input=value, expected=result) for value, result in zip(inputs, expected)])
    compressed = gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n').encode(), mtime=0)
    if args.record_new_corpus:
        corpus_path.write_bytes(compressed)
    else:
        assert corpus_path.read_bytes() == compressed, 'The original corpus must reproduce byte-for-byte'
    row_inputs = [dict(name=name, left=left, right=right)
                  for name, left, right in itertools.product(
                      ['_rank', '_雪\0name', '_' + 'x' * 256], range(10), range(10))]
    row_payload = ''.join(json.dumps(value, ensure_ascii=True) + '\n' for value in row_inputs)
    record['row_inputs'] = row_inputs
    row_expected = None
    if not args.record_new_row_corpus:
        previous = json.loads(gzip.decompress(row_path.read_bytes()))
        assert previous['baseline'] == baseline
        assert [case['input'] for case in previous['cases']] == row_inputs
        row_expected = [case['expected'] for case in previous['cases']]
    row_executables = [('original', Path(environment['CARGO_TARGET_DIR']) / 'debug/examples/row_extend_error'),
                       ('native', args.build_dir.resolve() / 'teaql_row_extend_error_probe')]
    if args.module_build_dir:
        row_executables.append(('modules', args.module_build_dir.resolve() /
                                'modules/teaql_module_row_extend_error_probe'))
    record['row_outputs'] = {}
    for label, executable in row_executables:
        output = run([executable], row_payload)
        assert output.endswith('\n')
        actual = [json.loads(line) for line in output.split('\n')[:-1]]
        assert len(actual) == len(row_inputs)
        assert all(case['unchanged'] for case in actual)
        record['row_outputs'][label] = dict(binary_sha256=digest(executable), count=len(actual),
                                            output_step=len(record['steps']) - 1)
        if row_expected is None:
            row_expected = actual
        differences = [dict(index=i, input=value, original=a, cpp=b)
                       for i, (value, a, b) in enumerate(zip(row_inputs, row_expected, actual))
                       if not same(a, b)]
        if differences:
            record['differences'] = differences
            save()
            raise RuntimeError(f'Row extension source differs: {label}')
        print(f'Row extension: {label} {len(row_inputs)}/{len(row_inputs)} exact outputs', flush=True)
    row_corpus = dict(baseline=baseline, scope='Public unchecked extension: exact source panic '
                     'message, unchanged failed rows and successful compatible schemas',
                     cases=[dict(input=value, expected=result)
                            for value, result in zip(row_inputs, row_expected)])
    row_compressed = gzip.compress((json.dumps(row_corpus, separators=(',', ':')) + '\n').encode(), mtime=0)
    if args.record_new_row_corpus:
        row_path.write_bytes(row_compressed)
    else:
        assert row_path.read_bytes() == row_compressed
    error_inputs = [dict(kind='error_debug', code=code, name=name)
                    for code, name in itertools.product(range(8),
                        ['', '_rank', '_雪\0name', '_' + 'x' * 256, 'line\n\r\ttab', 'quote"\\'])]
    record['error_inputs'] = error_inputs
    error_payload = ''.join(json.dumps(value, ensure_ascii=True) + '\n' for value in error_inputs)
    error_expected = None
    if not args.record_new_error_corpus:
        previous = json.loads(gzip.decompress(error_path.read_bytes()))
        assert previous['baseline'] == baseline
        assert [case['input'] for case in previous['cases']] == error_inputs
        error_expected = [case['expected'] for case in previous['cases']]
    record['error_outputs'] = {}
    for label, executable in row_executables:
        output = run([executable], error_payload)
        assert output.endswith('\n')
        actual = [json.loads(line) for line in output.split('\n')[:-1]]
        assert len(actual) == len(error_inputs)
        assert all(case['equal_clone'] for case in actual)
        record['error_outputs'][label] = dict(binary_sha256=digest(executable), count=len(actual),
                                              output_step=len(record['steps']) - 1)
        if error_expected is None:
            error_expected = actual
        differences = [dict(index=i, input=value, original=a, cpp=b)
                       for i, (value, a, b) in enumerate(zip(error_inputs, error_expected, actual))
                       if not same(a, b)]
        if differences:
            record['differences'] = differences
            save()
            raise RuntimeError(f'DynamicFieldError source differs: {label}')
        print(f'DynamicFieldError: {label} {len(error_inputs)}/{len(error_inputs)} exact outputs', flush=True)
    error_corpus = dict(baseline=baseline, scope='Exact DynamicFieldError Debug in nine modes, '
                       'Clone/equality and Display; source static codes remain static literals',
                       cases=[dict(input=value, expected=result)
                              for value, result in zip(error_inputs, error_expected)])
    error_compressed = gzip.compress((json.dumps(error_corpus, separators=(',', ':')) + '\n').encode(), mtime=0)
    if args.record_new_error_corpus:
        error_path.write_bytes(error_compressed)
    else:
        assert error_path.read_bytes() == error_compressed
    paths = [reference / 'teaql-core/src/query.rs', reference / 'teaql-core/src/request.rs',
             reference / 'Cargo.lock', PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock',
             PARITY / 'query_full_state_5_0_6.rs', PARITY / 'query_full_state_probe.cpp',
             ROOT / 'tests/support/query_full_state_fixture.hpp', PARITY / 'debug_modes.hpp',
             ROOT / 'teaql_core/src/query.cpp', ROOT / 'teaql_core/src/query_debug.cpp',
             ROOT / 'teaql_core/include/teaql/core/query.hpp', ROOT / 'teaql_core/src/request.cpp',
             ROOT / 'teaql_core/include/teaql/core/request.hpp', ROOT / 'teaql_core/src/row.cpp',
             ROOT / 'teaql_core/include/teaql/core/dynamic_fields_debug.hpp',
             PARITY / 'row_extend_error_probe.cpp', PROJECT / 'examples/row_extend_error.rs',
             Path(__file__).resolve()]
    record.update(complete=True, observations=len(inputs), row_observations=len(row_inputs),
                  error_observations=len(error_inputs),
                  corpus_sha256=digest(corpus_path), row_corpus_sha256=digest(row_path),
                  error_corpus_sha256=digest(error_path),
                  sources={str(path.relative_to(ROOT)): dict(sha256=digest(path), text=path.read_text())
                           for path in paths})
    save()
    print('All 31 field obligations observed; complete source acceptance remains separate.', flush=True)


if __name__ == '__main__':
    main()
