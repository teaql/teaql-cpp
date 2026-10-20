#!/usr/bin/env python3
"""Reproduce complete Expr factories, state, derives and LIKE provenance."""
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
PROJECT = PARITY / 'rust_core_expression_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from core_expression_source_inputs_5_0_6 import api_cases, like_cases


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
                  scope='Complete core expr.rs factories, structural state/derives, composition, '
                        'subquery preparation and LIKE provenance. SQL execution, safe-expression '
                        'evaluation and protected publication retain their separate responsibilities.')

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
    unit_output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                       reference / 'teaql-core/Cargo.toml', '--lib'])
    unit_match = re.search(r'test result: ok\. (\d+) passed; 0 failed; 0 ignored;', unit_output)
    assert unit_match, unit_output
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml', '--bins'])
    frontends = [('native', args.build_dir.resolve(), 'teaql_')]
    if args.module_build_dir:
        frontends.append(('modules', args.module_build_dir.resolve() / 'modules', 'teaql_module_'))
    matrices = [('retained_api', 'query_api_5_0_6_cases.json.gz', None,
                 'query_api', 'query_api_expression_source_506'),
                ('retained_debug', 'query_debug_5_0_6_cases.json.gz', None,
                 'query_debug', 'query_debug_expression_source_506'),
                ('api_boundaries', 'core_expression_boundaries_5_0_6_cases.json.gz', api_cases,
                 'query_api', 'query_api_expression_source_506'),
                ('like_provenance', 'core_like_expression_source_5_0_6_cases.json.gz', like_cases,
                 'core_like_expression_source', 'core_like_expression_source_506')]
    corpus_hashes = {}
    for name, filename, authored_inputs, cpp_stem, rust_name in matrices:
        path = PARITY / filename
        if authored_inputs:
            if args.record_new_corpora:
                assert not path.exists(), 'Do not overwrite source corpora'
            inputs, expected = authored_inputs(), None
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
            assert output.endswith('\n'), f'Unfinished observer: {name}/{label}'
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
                raise RuntimeError(f'Actual expression source differs: {name}/{label}')
            print(f'{name}: {label} {len(inputs)}/{len(inputs)} exact outcomes', flush=True)
        record['matrices'].append(matrix)
        if authored_inputs:
            corpus = dict(baseline=baseline, scope=record['scope'],
                          cases=[dict(input=value, expected=out) for value, out in zip(inputs, expected)])
            corpus_bytes = gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n').encode(), mtime=0)
            if args.record_new_corpora:
                path.write_bytes(corpus_bytes)
            else:
                assert path.read_bytes() == corpus_bytes, f'Exact corpus bytes differ: {name}'
        corpus_hashes[filename] = digest(path)
        save()
    sources = [reference / 'teaql-core/src/expr.rs', reference / 'teaql-core/Cargo.toml',
               reference / 'Cargo.lock', PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock',
               PARITY / 'rust_field_layout/src/bin/query_api_5_0_6.rs',
               PARITY / 'rust_field_layout/src/bin/query_debug_5_0_6.rs',
               PARITY / 'rust_values/src/fixture_value.rs',
               PARITY / 'core_like_expression_source_5_0_6.rs',
               PARITY / 'core_expression_source_inputs_5_0_6.py',
               PARITY / 'core_like_expression_source_probe.cpp',
               PARITY / 'query_api_probe.cpp', PARITY / 'query_debug_probe.cpp',
               PARITY / 'fixture_value.hpp', PARITY / 'debug_modes.hpp',
               ROOT / 'teaql_core/src/expr.cpp', ROOT / 'teaql_core/include/teaql/core/expr.hpp',
               ROOT / 'teaql_core/include/teaql/core/query_debug.hpp',
               ROOT / 'teaql_core/tests/query_model_tests.cpp',
               ROOT / 'teaql_core/tests/query_api_tests.cpp', Path(__file__)]
    record.update(complete=True, baseline=baseline, core_unit_tests=int(unit_match[1]),
                  observations_per_frontend=sum(m['cases'] for m in record['matrices']),
                  reference_registry_versions_checksums_exact=True, corpus_sha256=corpus_hashes,
                  sources={str(p.resolve()): dict(sha256=digest(p), text=p.read_text()) for p in sources})
    save()
    print('Complete Expr source observations pass; wider SQL/runtime/governance remain separate.')


if __name__ == '__main__':
    main()
