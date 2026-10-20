#!/usr/bin/env python3
"""Reproduce complete SQL binding source and every original binding-policy test."""
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
PROJECT = PARITY / 'rust_sql_bindings_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from sql_bindings_source_inputs_5_0_6 import cases


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
    new_path = PARITY / 'sql_bindings_source_5_0_6_cases.json.gz'
    if args.record_new_corpus and new_path.exists():
        parser.error('Do not overwrite source corpora')
    record = dict(complete=False, full_goal_accepted=False, steps=[], matrices=[],
                  scope='Complete bindings.rs and binding_tests.rs: values/policies/original '
                        'positions, scope/combine/expression classification, typed LIKE validation, '
                        'rebind/trust, derives and all original compiler-policy assertions. '
                        'SQLite-only execution scope; full dialect/executor/provider and protected '
                        'runtime publication remain separate.')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')

    def run(command, payload=None):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, env=environment, text=True,
                                input=payload, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    run(['python3', ROOT / 'scripts/verify_upstream.py'])
    reference = ROOT / '.reference/teaql-rs'
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock')
    names = re.findall(r'#\[test\]\nfn (\w+)', (reference / 'teaql-sql/src/binding_tests.rs').read_text())
    assert len(names) == 11
    units = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                 reference / 'teaql-sql/Cargo.toml', '--lib', 'binding_tests::'])
    assert all('binding_tests::' + name + ' ... ok' in units for name in names)
    assert '11 passed; 0 failed; 0 ignored;' in units
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml', '--bins'])
    frontends = [('native', args.build_dir.resolve(), 'teaql_')]
    if args.module_build_dir:
        frontends.append(('modules', args.module_build_dir.resolve() / 'modules', 'teaql_module_'))
    matrices = [('retained_model', PARITY / 'sql_model_api_5_0_6_cases.json.gz',
                 'sql_model_api', 'sql_model_bindings_source_506'),
                ('bindings', new_path, 'sql_bindings_source', 'sql_bindings_source_506')]
    hashes = {}
    for name, path, cpp_stem, rust_name in matrices:
        if name == 'bindings':
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
            output = run([executable], payload)
            assert output.endswith('\n'), f'Unfinished observation: {name}/{label}'
            actual = [json.loads(line) for line in output.split('\n')[:-1]]
            assert len(actual) == len(inputs)
            matrix['outputs'][label] = dict(binary_sha256=digest(executable), values=actual)
            if expected is None:
                expected = actual
            if not same(expected, actual):
                record['matrices'].append(matrix)
                record['differences'] = [dict(index=i, input=value, original=a, cpp=b)
                    for i, (value, a, b) in enumerate(zip(inputs, expected, actual)) if not same(a, b)]
                save()
                raise RuntimeError(f'Actual SQL binding source differs: {name}/{label}')
            print(f'{name}: {label} {len(inputs)}/{len(inputs)} exact outcomes', flush=True)
        record['matrices'].append(matrix)
        if name == 'bindings':
            corpus = dict(baseline=baseline, scope=record['scope'],
                          cases=[dict(input=value, expected=out) for value, out in zip(inputs, expected)])
            compressed = gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n').encode(), mtime=0)
            if args.record_new_corpus:
                path.write_bytes(compressed)
            else:
                assert path.read_bytes() == compressed, 'Exact source corpus bytes differ'
        hashes[path.name] = digest(path)
        save()
    sources = [reference / 'teaql-sql/src/bindings.rs', reference / 'teaql-sql/src/binding_tests.rs',
               reference / 'teaql-sql/Cargo.toml', reference / 'Cargo.lock',
               PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock',
               PARITY / 'rust_field_layout/src/bin/sql_model_api_5_0_6.rs',
               PARITY / 'rust_values/src/fixture_value.rs', PARITY / 'sql_bindings_source_5_0_6.rs',
               PARITY / 'sql_bindings_source_inputs_5_0_6.py',
               PARITY / 'sql_bindings_source_probe.cpp', PARITY / 'sql_model_api_probe.cpp',
               PARITY / 'fixture_value.hpp', PARITY / 'debug_modes.hpp',
               ROOT / 'tests/parity/teaql_sql/binding_tests.cpp',
               ROOT / 'teaql_sql/include/teaql/sql/bindings.hpp',
               ROOT / 'teaql_sql/include/teaql/sql/model_debug.hpp',
               ROOT / 'teaql_sql/src/bindings.cpp', ROOT / 'teaql_core/src/expr.cpp',
               ROOT / 'teaql_sql/tests/sql_model_api_tests.cpp',
               ROOT / 'teaql_sql/tests/sql_query_tests.cpp', Path(__file__)]
    record.update(complete=True, baseline=baseline, original_unit_tests=names,
                  observations_per_frontend=sum(m['cases'] for m in record['matrices']),
                  reference_registry_versions_checksums_exact=True, corpus_sha256=hashes,
                  sources={str(p.resolve()): dict(sha256=digest(p), text=p.read_text()) for p in sources})
    save()
    print('Complete binding source models/private helpers/original test protocols passed.')


if __name__ == '__main__':
    main()
