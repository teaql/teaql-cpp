#!/usr/bin/env python3
"""Reproduce full SafeExpression behavior, emptiness and source Clone bounds."""
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
PROJECT = PARITY / 'rust_safe_expression_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from safe_expression_source_inputs_5_0_6 import cases


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
    new_path = PARITY / 'safe_expression_empty_source_5_0_6_cases.json.gz'
    if args.record_new_corpus and new_path.exists():
        parser.error('Do not overwrite source corpora')
    record = dict(complete=False, full_goal_accepted=False, steps=[], matrices=[], compiler_controls=[],
                  scope='Complete safe_expression.rs: all 24 methods, six TeaqlEmpty '
                        'implementations and generic capability boundaries. Explicit Context '
                        'borrows/copies adapt source Arc ownership; generated E diagnostics and '
                        'whole runtime/provider execution retain separate requirements.')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')

    def run(command, payload=None, expected_failure=False):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, env=environment, text=True,
                                input=payload, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    expected_failure=expected_failure,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        if expected_failure:
            assert result.returncode != 0, 'Expected source compiler rejection'
        else:
            result.check_returncode()
        return result

    run(['python3', ROOT / 'scripts/verify_upstream.py'])
    reference = ROOT / '.reference/teaql-rs'
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock')
    original = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                    reference / 'teaql-core/Cargo.toml', '--lib', 'safe_expression::tests::'])
    tests = ['safe_expression_eval_with_uses_the_supplied_root',
             'safe_expression_apply_optional_short_circuits_remaining_mappers',
             'safe_expression_lazy_fallback_and_error_only_run_for_missing_values',
             'safe_expression_callbacks_only_run_for_their_matching_branch']
    assert all('safe_expression::tests::' + name + ' ... ok' in original.stdout for name in tests)
    assert '4 passed; 0 failed; 0 ignored;' in original.stdout
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml',
         '--bins', '--example', 'positive'])
    run([Path(environment['CARGO_TARGET_DIR']) / 'debug/examples/positive'])
    record['compiler_controls'].append(dict(name='positive', status='accepted_and_executed'))
    controls = [('clone_root', 'clone'), ('clone_result', 'clone'),
                ('value_nonclone', 'value'), ('root_nonclone', 'root'),
                ('root_mismatched', 'root'), ('list_size', 'size'),
                ('list_first', 'first'), ('list_get', 'get')]
    for name, method in controls:
        result = run(['cargo', 'check', '--offline', '--locked', '--manifest-path',
                      PROJECT / 'Cargo.toml', '--example', name], expected_failure=True)
        assert 'error[E0599]' in result.stderr or 'error[E0277]' in result.stderr
        assert method in result.stderr
        if name != 'root_mismatched':
            assert 'NoClone: Clone' in result.stderr, result.stderr
        else:
            assert 'no method named `root`' in result.stderr
        record['compiler_controls'].append(dict(name=name, method=method,
                                                status='expected_bound_rejection'))
        save()
    frontends = [('native', args.build_dir.resolve(), 'teaql_')]
    if args.module_build_dir:
        frontends.append(('modules', args.module_build_dir.resolve() / 'modules', 'teaql_module_'))
    matrices = [('retained', PARITY / 'safe_expression_api_5_0_6_cases.json.gz',
                 'safe_expression_api', 'safe_expression_api_source_506'),
                ('builtins', new_path, 'safe_expression_empty_source', 'safe_expression_empty_source_506')]
    hashes = {}
    for name, path, cpp_stem, rust_name in matrices:
        if name == 'retained':
            previous = json.loads(gzip.decompress(path.read_bytes()))
            assert previous['baseline'] == baseline
            assert digest(ROOT / previous['driver']) == previous['driver_sha256']
            inputs = [case['input'] for case in previous['cases']]
            expected = [case['expected'] for case in previous['cases']]
        else:
            inputs, expected = cases(), None
        payload = ''.join(json.dumps(value, ensure_ascii=True) + '\n' for value in inputs)
        matrix = dict(name=name, cases=len(inputs), inputs=inputs, outputs={})
        executables = [('original', Path(environment['CARGO_TARGET_DIR']) / 'debug' / rust_name)]
        executables += [(label, directory / (prefix + cpp_stem + '_probe'))
                        for label, directory, prefix in frontends]
        for label, executable in executables:
            result = run([executable], payload)
            assert result.stdout.endswith('\n'), f'Unfinished observation: {name}/{label}'
            actual = [json.loads(line) for line in result.stdout.split('\n')[:-1]]
            assert len(actual) == len(inputs)
            matrix['outputs'][label] = dict(binary_sha256=digest(executable), values=actual)
            if expected is None:
                expected = actual
            if not same(expected, actual):
                record['matrices'].append(matrix)
                record['differences'] = [dict(index=i, input=value, original=a, cpp=b)
                    for i, (value, a, b) in enumerate(zip(inputs, expected, actual)) if not same(a, b)]
                save()
                raise RuntimeError(f'Actual SafeExpression source differs: {name}/{label}')
            print(f'{name}: {label} {len(inputs)}/{len(inputs)} exact outcomes', flush=True)
        record['matrices'].append(matrix)
        if name == 'builtins':
            corpus = dict(baseline=baseline, scope=record['scope'],
                          cases=[dict(input=value, expected=out) for value, out in zip(inputs, expected)])
            compressed = gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n').encode(), mtime=0)
            if args.record_new_corpus:
                path.write_bytes(compressed)
            else:
                assert path.read_bytes() == compressed, 'Exact source corpus bytes differ'
        hashes[path.name] = digest(path)
        save()
    sources = [reference / 'teaql-core/src/safe_expression.rs', reference / 'teaql-core/Cargo.toml',
               reference / 'Cargo.lock', PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock',
               PARITY / 'rust_field_layout/src/bin/safe_expression_api_5_0_6.rs',
               PARITY / 'rust_values/src/fixture_value.rs',
               PARITY / 'safe_expression_empty_source_5_0_6.rs',
               PARITY / 'safe_expression_source_inputs_5_0_6.py',
               PARITY / 'safe_expression_empty_source_probe.cpp',
               PARITY / 'safe_expression_api_probe.cpp', PARITY / 'fixture_value.hpp',
               ROOT / 'tests/support/safe_expression_empty_fixture.hpp',
               ROOT / 'teaql_core/include/teaql/core/safe_expression.hpp',
               ROOT / 'teaql_memory/include/teaql/memory/function.hpp',
               ROOT / 'teaql_memory/include/teaql/memory/clone.hpp',
               ROOT / 'teaql_core/tests/safe_expression_api_tests.cpp',
               ROOT / 'teaql_core/tests/entity_expression_tests.cpp',
               ROOT / 'teaql_core/tests/core_collection_tests.cpp', Path(__file__)]
    sources += sorted((PROJECT / 'examples').glob('*.rs'))
    record.update(complete=True, baseline=baseline, original_unit_tests=tests,
                  observations_per_frontend=sum(m['cases'] for m in record['matrices']),
                  reference_registry_versions_checksums_exact=True, corpus_sha256=hashes,
                  sources={str(path.resolve()): dict(sha256=digest(path), text=path.read_text())
                           for path in sources})
    save()
    print('Complete SafeExpression source observations and actual compiler bounds passed.')


if __name__ == '__main__':
    main()
