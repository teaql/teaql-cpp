#!/usr/bin/env python3
"""Reproduce complete SQLite-profile SQL type/renderer responsibilities."""
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
PROJECT = PARITY / 'rust_sql_types_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from sql_types_source_inputs_5_0_6 import cases, model_cases


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--module-build-dir', type=Path)
    parser.add_argument('--record-new-corpora', action='store_true')
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False, full_goal_accepted=False, steps=[], matrices=[],
                  scope='Whole types.rs and original rendering test responsibilities in the '
                        'authorized SQLite profile. All models/errors/derives, comment/literal/'
                        'lexer/callback accounting and omission contracts are included. '
                        'PostgreSQL/MySQL lexer/literal/array/numbered/dollar-quote branches '
                        'remain explicit exclusions; valid discriminator diagnostics are preserved. '
                        'New usize-maximum matrices are native64 only.')

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
    names = re.findall(r'#\[test\]\nfn (\w+)', (reference / 'teaql-sql/src/log_rendering_tests.rs').read_text())
    assert len(names) == 6
    output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                  reference / 'teaql-sql/Cargo.toml', '--lib', 'log_rendering_tests::'])
    assert all('log_rendering_tests::' + name + ' ... ok' in output for name in names)
    assert '6 passed; 0 failed; 0 ignored;' in output
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml', '--bins'])
    frontends = [('native', args.build_dir.resolve(), 'teaql_')]
    if args.module_build_dir:
        frontends.append(('modules', args.module_build_dir.resolve() / 'modules', 'teaql_module_'))
    model = json.loads(gzip.decompress((PARITY / 'sql_model_api_5_0_6_cases.json.gz').read_bytes()))
    authored_models = model_cases(model['cases'][0]['input'])
    matrices = [('retained_types', 'sql_types_5_0_6_cases.json.gz', None, 'sql_types', 'sql_types_source_506'),
                ('retained_models', 'sql_model_api_5_0_6_cases.json.gz', None, 'sql_model_api', 'sql_model_types_source_506'),
                ('type_boundaries', 'sql_types_boundaries_5_0_6_cases.json.gz', cases(), 'sql_types', 'sql_types_source_506'),
                ('model_boundaries', 'sql_type_models_boundaries_5_0_6_cases.json.gz', authored_models, 'sql_model_api', 'sql_model_types_source_506')]
    hashes = {}
    for name, filename, authored, cpp_stem, rust_name in matrices:
        path = PARITY / filename
        if authored is not None:
            if args.record_new_corpora:
                assert not path.exists(), 'Do not overwrite source corpora'
            inputs, expected = authored, None
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
                raise RuntimeError(f'Actual SQL types source differs: {name}/{label}')
            print(f'{name}: {label} {len(inputs)}/{len(inputs)} exact outcomes', flush=True)
        record['matrices'].append(matrix)
        if authored is not None:
            corpus = dict(baseline=baseline, scope=record['scope'],
                          cases=[dict(input=value, expected=out) for value, out in zip(inputs, expected)])
            compressed = gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n').encode(), mtime=0)
            if args.record_new_corpora:
                path.write_bytes(compressed)
            else:
                assert path.read_bytes() == compressed, 'Exact source corpus bytes differ'
        hashes[path.name] = digest(path)
        save()
    sources = [reference / 'teaql-sql/src/types.rs', reference / 'teaql-sql/src/log_rendering_tests.rs',
               reference / 'teaql-sql/Cargo.toml', reference / 'Cargo.lock',
               PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock',
               PARITY / 'rust_field_layout/src/bin/sql_types_5_0_6.rs',
               PARITY / 'rust_field_layout/src/bin/sql_model_api_5_0_6.rs',
               PARITY / 'rust_values/src/fixture_value.rs', PARITY / 'sql_types_source_inputs_5_0_6.py',
               PARITY / 'sql_types_probe.cpp', PARITY / 'sql_model_api_probe.cpp',
               PARITY / 'fixture_value.hpp', PARITY / 'debug_modes.hpp',
               ROOT / 'tests/parity/teaql_sql/log_rendering_tests.cpp',
               ROOT / 'teaql_sql/include/teaql/sql/types.hpp',
               ROOT / 'teaql_sql/include/teaql/sql/error_debug.hpp',
               ROOT / 'teaql_sql/src/types.cpp', ROOT / 'teaql_sql/tests/sql_model_api_tests.cpp', Path(__file__)]
    record.update(complete=True, baseline=baseline, original_full_crate_tests=names,
                  excluded_original_test='postgres_repeated_bindings_dollar_quotes_and_nested_comments',
                  observations_per_frontend=sum(m['cases'] for m in record['matrices']),
                  reference_registry_versions_checksums_exact=True, corpus_sha256=hashes,
                  sources={str(p.resolve()): dict(sha256=digest(p), text=p.read_text()) for p in sources})
    save()
    print('Complete SQLite-profile SQL models, errors, renderer and original test responsibilities pass.')


if __name__ == '__main__':
    main()
