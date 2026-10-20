#!/usr/bin/env python3
"""Reproduce the complete core mutation models, map roles and compiler contracts."""
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
PROJECT = PARITY / 'rust_core_mutation_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from core_mutation_source_inputs_5_0_6 import cases


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
    new_path = PARITY / 'core_mutation_boundaries_5_0_6_cases.json.gz'
    if args.record_new_corpus and new_path.exists():
        parser.error('Do not overwrite source corpora')
    record = dict(complete=False, full_goal_accepted=False, steps=[], matrices=[], compiler_controls=[],
                  scope='Complete mutation.rs: distinct input/generated/snapshot maps, six commands, '
                        'MutationKind, map traits/conversions/iteration, defaults/builders/Clone/'
                        'PartialEq/Debug. Actual persistence, guarded execution, governance and '
                        'safe publication retain separate responsibilities.')

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
    units = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                 reference / 'teaql-core/Cargo.toml', '--lib'])
    unit_count = re.search(r'test result: ok\. (\d+) passed; 0 failed; 0 ignored;', units.stdout)
    assert unit_count
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml',
         '--bins', '--example', 'positive'])
    run([Path(environment['CARGO_TARGET_DIR']) / 'debug/examples/positive'])
    record['compiler_controls'].append(dict(name='positive', status='accepted_and_executed'))
    roles = {'mutation': 'MutationValues', 'generated': 'GeneratedValues', 'snapshot': 'EntitySnapshot'}
    controls = []
    for src, source_type in roles.items():
        for dest, destination_type in roles.items():
            if src != dest:
                controls.append((f'{src}_to_{dest}', 'E0308', [source_type, destination_type]))
    controls += [('generated_iteration', 'E0277', ['GeneratedValues', 'iterator']),
                 ('snapshot_iteration', 'E0277', ['EntitySnapshot', 'iterator'])]
    for name, type_name in [('insert', 'InsertCommand'), ('update', 'UpdateCommand'),
                            ('batch_insert', 'BatchInsertCommand'), ('batch_update', 'BatchUpdateCommand'),
                            ('delete', 'DeleteCommand'), ('recover', 'RecoverCommand')]:
        controls.append((name + '_default', 'E0277', [type_name, 'Default']))
    for name, code, terms in controls:
        result = run(['cargo', 'check', '--offline', '--locked', '--manifest-path',
                      PROJECT / 'Cargo.toml', '--example', name], expected_failure=True)
        assert 'error[' + code + ']' in result.stderr
        assert all(term in result.stderr for term in terms), result.stderr
        record['compiler_controls'].append(dict(name=name, status='expected_type_trait_rejection',
                                                error_code=code, required_terms=terms))
        save()
    frontends = [('native', args.build_dir.resolve(), 'teaql_')]
    if args.module_build_dir:
        frontends.append(('modules', args.module_build_dir.resolve() / 'modules', 'teaql_module_'))
    matrices = [('retained_api', PARITY / 'mutation_model_api_5_0_6_cases.json.gz',
                 'mutation_model_api', 'mutation_model_source_506'),
                ('retained_debug', PARITY / 'mutation_debug_5_0_6_cases.json.gz',
                 'mutation_debug', 'mutation_debug_source_506'),
                ('boundaries', new_path, 'mutation_model_api', 'mutation_model_source_506')]
    hashes = {}
    for name, path, cpp_stem, rust_name in matrices:
        if name == 'boundaries':
            inputs, expected = cases(), None
        else:
            previous = json.loads(gzip.decompress(path.read_bytes()))
            assert previous['baseline'] == baseline
            assert digest(ROOT / previous['driver']) == previous['driver_sha256']
            inputs = [case['input'] for case in previous['cases']]
            expected = [case['expected'] for case in previous['cases']]
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
                raise RuntimeError(f'Actual core mutation source differs: {name}/{label}')
            print(f'{name}: {label} {len(inputs)}/{len(inputs)} exact outcomes', flush=True)
        record['matrices'].append(matrix)
        if name == 'boundaries':
            corpus = dict(baseline=baseline, scope=record['scope'],
                          cases=[dict(input=value, expected=out) for value, out in zip(inputs, expected)])
            compressed = gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n').encode(), mtime=0)
            if args.record_new_corpus:
                path.write_bytes(compressed)
            else:
                assert path.read_bytes() == compressed, 'Exact source corpus bytes differ'
        hashes[path.name] = digest(path)
        save()
    sources = [reference / 'teaql-core/src/mutation.rs', reference / 'teaql-core/src/trace.rs',
               reference / 'teaql-core/Cargo.toml', reference / 'Cargo.lock',
               PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock',
               PARITY / 'rust_field_layout/src/bin/mutation_model_api_5_0_6.rs',
               PARITY / 'rust_field_layout/src/bin/mutation_debug_5_0_6.rs',
               PARITY / 'rust_values/src/fixture_value.rs', PARITY / 'core_mutation_source_inputs_5_0_6.py',
               PARITY / 'mutation_model_api_probe.cpp', PARITY / 'mutation_debug_probe.cpp',
               PARITY / 'fixture_value.hpp', PARITY / 'debug_modes.hpp',
               ROOT / 'teaql_core/include/teaql/core/mutation.hpp',
               ROOT / 'teaql_core/include/teaql/core/mutation_debug.hpp',
               ROOT / 'teaql_core/include/teaql/core/trace.hpp',
               ROOT / 'teaql_core/tests/mutation_model_api_tests.cpp',
               ROOT / 'teaql_core/tests/mutation_debug_tests.cpp', Path(__file__)]
    sources += sorted((PROJECT / 'examples').glob('*.rs'))
    record.update(complete=True, baseline=baseline, original_core_unit_tests=int(unit_count[1]),
                  observations_per_frontend=sum(m['cases'] for m in record['matrices']),
                  reference_registry_versions_checksums_exact=True, corpus_sha256=hashes,
                  sources={str(p.resolve()): dict(sha256=digest(p), text=p.read_text()) for p in sources})
    save()
    print('Complete mutation source models, map roles and compiler protocols passed.')


if __name__ == '__main__':
    main()
