#!/usr/bin/env python3
"""Replay existing generated-support observations with exact reference dependencies.

This gate preserves every historical observer and expected output. It establishes
dependency provenance and focused behavior, not whole-source or generator parity.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_generated_support_reference_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same

MATRICES = (
    ('public_facets_5_0_6', 1178, 'support_reference_506', 'generated_support_probe'),
    ('public_facet_logs_5_0_6', 36, 'support_reference_506', 'generated_support_probe'),
    ('purposed_query_debug_5_0_6', 46, 'purpose_reference_506', 'generated_support_probe'),
    ('derived_query_diagnostics_5_0_6', 576, 'derived_reference_506',
     'derived_query_diagnostics_probe'),
    ('derived_query_composition_source_5_0_6', 360, 'derived_reference_506',
     'derived_query_composition_probe'),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--module-build-dir', type=Path)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh evidence path; preserve previous outcomes')
    record = dict(complete=False, whole_source_accepted=False, full_goal_accepted=False,
                  steps=[], matrices=[], original_tests={},
                  scope='Exact-reference replay of five unchanged corpora and nineteen '
                        'original SQLite tests. Whole-source/capability exits remain separate.')
    target = Path('/tmp/teaql_cpp_core_baselines_506')
    env = dict(os.environ, CARGO_TARGET_DIR=str(target),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
               TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress(
            (json.dumps(record, indent=2) + '\n').encode(), compresslevel=1, mtime=0))

    def run(command, payload=None):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, env=env, input=payload,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    run([sys.executable, ROOT / 'scripts/verify_upstream.py'])
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    identities = registry_identities(PROJECT / 'Cargo.lock')
    assert identities <= registry_identities(reference_lock), 'Reference registry drift'
    record['registry_packages'] = len(identities)
    # The old locks are evidence of historical execution, not a current authority.
    record['historical_dependency_differences'] = {
        name: len(registry_identities(PARITY / name / 'Cargo.lock') -
                  registry_identities(reference_lock))
        for name in ('rust_generated_support_5_0_6', 'rust_runtime_baselines_5_0_6',
                     'rust_field_layout')}
    corpus_hashes = {
        stem: digest(PARITY / (stem + '_cases.json.gz'))
        for stem, _, _, _ in MATRICES}
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
         PROJECT / 'Cargo.toml', '--bins'])
    for binary, count in (('original_request_reference_506', 9),
                          ('original_facet_reference_506', 10)):
        output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                      PROJECT / 'Cargo.toml', '--bin', binary])
        names = re.findall(r'^test original::([^ ]+) \.\.\. ok$', output, re.M)
        assert len(names) == count and f'{count} passed; 0 failed; 0 ignored;' in output
        record['original_tests'][binary] = names

    targets = ['generated_support_tests', 'generated_support_probe',
               'derived_query_diagnostics_probe', 'derived_query_composition_probe']
    run(['cmake', '--build', args.build_dir.resolve(), '--target',
         *('teaql_' + name for name in targets), '--parallel', '4'])
    if args.module_build_dir:
        run(['cmake', '--build', args.module_build_dir.resolve(), '--target',
             *('teaql_module_' + name for name in targets), '--parallel', '4'])

    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    for stem, count, driver, native in MATRICES:
        corpus = json.loads(gzip.decompress((PARITY / (stem + '_cases.json.gz')).read_bytes()))
        assert corpus['baseline'] == baseline and len(corpus['cases']) == count
        for path, expected in corpus.get('source_hashes', {}).items():
            assert digest(ROOT / '.reference/teaql-rs' / path) == expected
        inputs = [case['input'] for case in corpus['cases']]
        expected = [case['expected'] for case in corpus['cases']]
        payload = ''.join(json.dumps(value, ensure_ascii=True) + '\n' for value in inputs)
        binaries = [('original', target / 'debug' / driver),
                    ('native', args.build_dir.resolve() / ('teaql_' + native))]
        if args.module_build_dir:
            binaries.append(('modules', args.module_build_dir.resolve() /
                             'modules' / ('teaql_module_' + native)))
        matrix = dict(name=stem, observations=count, inputs=inputs, outputs={}, differences=[])
        record['matrices'].append(matrix)
        for frontend, executable in binaries:
            output = run([executable], payload)
            assert output.endswith('\n'), 'Incomplete JSON Lines output'
            actual = [json.loads(line) for line in output.split('\n')[:-1]]
            assert len(actual) == count, 'Incomplete observation count'
            matrix['outputs'][frontend] = dict(step=len(record['steps']) - 1,
                                                executable_sha256=digest(executable))
            matrix['differences'].extend(
                dict(frontend=frontend, index=i, input=inputs[i], expected=a, actual=b)
                for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b))
            save()
            assert not matrix['differences'], f'{stem}: exact output differences'
            print(f'{stem}: {frontend} {count}/{count} exact', flush=True)

    native_names = ['generated_support_contracts', 'generated_support_rust_parity',
                    'public_facet_logs_rust_parity', 'purposed_query_debug_rust_parity',
                    'derived_query_diagnostics_rust_parity',
                    'derived_query_diagnostics_context_contracts',
                    'derived_query_composition_rust_parity']
    frontends = [('native', args.build_dir.resolve(), native_names)]
    if args.module_build_dir:
        frontends.append(('modules', args.module_build_dir.resolve(),
                          ['module_' + ('public_facets_rust_parity'
                                        if name == 'generated_support_rust_parity' else name)
                           for name in native_names]))
    for frontend, build, names in frontends:
        pattern = '^(' + '|'.join(names) + ')$'
        registry = json.loads(run(['ctest', '--test-dir', build, '--show-only=json-v1',
                                   '-R', pattern]))
        assert {test['name'] for test in registry['tests']} == set(names)
        output = run(['ctest', '--test-dir', build, '--output-on-failure', '-R', pattern])
        assert re.search(r'100% tests passed(?:, 0 tests failed)? out of 7', output)
        run(['cmake', '--build', build, '--target',
             *(('teaql_module_' if frontend == 'modules' else 'teaql_') + name
               for name in targets), '--parallel', '4'])

    assert all(digest(PARITY / (stem + '_cases.json.gz')) == value
               for stem, value in corpus_hashes.items()), 'Historical corpus changed'
    paths = [PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock', PROJECT / 'original_facet.rs',
             Path(__file__).resolve(), reference_lock,
             ROOT / '.reference/teaql-rs/teaql-runtime/src/generated_support.rs',
             ROOT / '.reference/teaql-rs/teaql-provider-sqlite/tests/facet_trace.rs',
             ROOT / '.reference/teaql-rs/teaql-provider-sqlite/tests/request_trace_log.rs']
    record.update(complete=True, observations_per_frontend=sum(row[1] for row in MATRICES),
                  baseline=baseline, normalization='none', corpus_sha256=corpus_hashes,
                  sources={str(path.relative_to(ROOT)): dict(sha256=digest(path),
                           text=path.read_text()) for path in paths})
    save()
    print('Exact-reference closure complete: 2196 observations per frontend; '
          '19 original tests; whole-source/capability exits remain separate.', flush=True)


if __name__ == '__main__':
    main()
