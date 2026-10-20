#!/usr/bin/env python3
"""Replay query, row, schema, sidecar and initialized-layout source contracts."""
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
PROJECT = PARITY / 'rust_core_query_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from compare_initialized_snapshot_cache_5_0_6 import validate_ownership
from core_query_source_inputs_5_0_6 import cases, intent_cases


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    return json.loads(gzip.decompress(path.read_bytes()) if path.suffix == '.gz'
                      else path.read_bytes())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--module-build-dir', type=Path)
    parser.add_argument('--record-new-corpus', action='store_true')
    parser.add_argument('--record-new-intent-corpus', action='store_true')
    parser.add_argument('--intent-only', action='store_true')
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False, full_goal_accepted=False, whole_source_accepted=False,
                  steps=[], matrices=[],
                  scope='Query/row source continuation and core/runtime purpose repair. '
                        'The readonly schema builder has independent source/lifetime '
                        'qualification; this replay is not whole query.rs acceptance. '
                        'Source Arc/Weak retention becomes explicit Context '
                        'retention. Initialized private cache Debug identities/order are '
                        'retained as raw observations, not compared as stable wire values. '
                        'Runtime query execution, protected publication and whole G13/P7 '
                        'remain separate responsibilities.')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(),
                                             mtime=0))

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
    baseline = load(ROOT / 'upstream.lock.json')['commit']
    assert registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock')
    original_tests = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                          reference / 'teaql-core/Cargo.toml', '--lib'])
    match = re.search(r'test result: ok\. (\d+) passed; 0 failed; 0 ignored;', original_tests)
    assert match, original_tests
    query_tests = re.findall(r'^test query::hard_limit_tests::([^ ]+)(?: - should panic)? \.\.\. ok$',
                             original_tests, re.M)
    assert set(query_tests) == {
        'continuous_page_fetch_is_explicit_and_validated',
        'continuous_page_fetch_rejects_empty_namespace',
        'id_set_pagination_is_explicit_and_validated',
        'id_set_pagination_rejects_zero_limit',
        'list_limit_defaults_rejects_and_allows_explicit_override',
        'topn_001_002_003_probe_threshold_is_explicit_and_bounded'}, query_tests
    record['original_tests'] = dict(core=int(match[1]), query=query_tests)
    if not args.intent_only:
        shared_tests = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                            reference / 'teaql-core/Cargo.toml', '--test', 'shared_load_state'])
        assert '19 passed; 0 failed; 0 ignored;' in shared_tests
        record['original_tests']['shared_load_state'] = 19
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml', '--bins'])
    frontends = [('native', args.build_dir.resolve(), 'teaql_')]
    if args.module_build_dir:
        frontends.append(('modules', args.module_build_dir.resolve() / 'modules', 'teaql_module_'))
    matrices = [('query_api', 'query_api_5_0_6_cases.json.gz'),
                ('query_debug', 'query_debug_5_0_6_cases.json.gz'),
                ('compact_row_api', 'compact_row_api_5_0_6_cases.json.gz'),
                ('model_debug', 'model_debug_5_0_6_cases.json.gz'),
                ('core_shape_presentation', 'core_shape_presentation_5_0_6_cases.json'),
                ('dynamic_carrier', 'dynamic_carrier_5_0_6_cases.json'),
                ('initialized_snapshot_cache', 'initialized_snapshot_cache_5_0_6_cases.json.gz'),
                ('query_boundaries', 'core_query_boundaries_5_0_6_cases.json.gz'),
                ('core_query_intent_source', 'core_query_intent_source_5_0_6_cases.json.gz')]
    if args.intent_only:
        matrices = [matrices[-1]]
    corpus_hashes = {}
    drivers = set()
    for stem, filename in matrices:
        path = PARITY / filename
        authored = stem in ('query_boundaries', 'core_query_intent_source')
        create = args.record_new_corpus if stem == 'query_boundaries' else args.record_new_intent_corpus
        rust_stem = 'query_api' if stem == 'query_boundaries' else stem
        if authored:
            inputs = cases() if stem == 'query_boundaries' else intent_cases()
            expected = None
            if create:
                assert not path.exists(), 'Do not overwrite source corpora'
        else:
            previous = load(path)
            assert previous['baseline'] == baseline
            if previous.get('driver'):
                assert digest(ROOT / previous['driver']) == previous['driver_sha256']
            inputs = [case['input'] for case in previous['cases']]
            key = 'source' if stem == 'initialized_snapshot_cache' else 'expected'
            expected = [case[key] for case in previous['cases']]
        payload = ''.join(json.dumps(value, ensure_ascii=True) + '\n' for value in inputs)
        matrix = dict(name=stem, cases=len(inputs), inputs=inputs, outputs={})
        executables = [('original', Path(environment['CARGO_TARGET_DIR']) / 'debug' /
                        (rust_stem + '_query_source_506'))]
        executables += [(label, directory / (prefix + rust_stem + '_probe'))
                        for label, directory, prefix in frontends]
        for label, executable in executables:
            output = run([executable], payload)
            assert output.endswith('\n'), f'Unfinished observer: {stem}/{label}'
            actual = [json.loads(line) for line in output.split('\n')[:-1]]
            assert len(actual) == len(inputs)
            matrix['outputs'][label] = dict(binary_sha256=digest(executable), values=actual)
            if expected is None:
                expected = actual
            differences = []
            for i, (input_value, left, right) in enumerate(zip(inputs, expected, actual)):
                if stem == 'initialized_snapshot_cache':
                    if input_value['kind'] == 'ownership':
                        if label == 'original':
                            assert same(left, right)
                        else:
                            validate_ownership(left, right)
                        continue
                    equal = same(left['common'], right['common'])
                    assert right.get('diagnostics'), 'Do not omit raw cache diagnostics'
                else:
                    equal = same(left, right)
                if not equal:
                    differences.append(dict(index=i, input=input_value, expected=left, actual=right))
            if differences:
                record['matrices'].append(matrix)
                record['differences'] = differences
                save()
                raise RuntimeError(f'Actual query source differs: {stem}/{label}')
            print(f'{stem}: {label} {len(inputs)}/{len(inputs)} outcomes', flush=True)
        record['matrices'].append(matrix)
        if authored:
            scope = ('Exact query helper/validation source boundaries' if stem == 'query_boundaries'
                     else 'Exact core purpose setter and runtime purpose trace boundary')
            corpus = dict(baseline=baseline, scope=scope,
                          cases=[dict(input=value, expected=out) for value, out in zip(inputs, expected)])
            compressed = gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n').encode(), mtime=0)
            if create:
                path.write_bytes(compressed)
            else:
                assert path.read_bytes() == compressed, 'Exact source corpus bytes differ'
        corpus_hashes[filename] = digest(path)
        if stem in ('initialized_snapshot_cache', 'core_query_intent_source'):
            drivers.add(PARITY / (stem + '_5_0_6.rs'))
        else:
            drivers.add(PARITY / f'rust_field_layout/src/bin/{rust_stem}_5_0_6.rs')
        drivers.add(PARITY / (rust_stem + '_probe.cpp'))
        save()
    sources = list(drivers) + [reference / 'teaql-core/src/query.rs', reference / 'teaql-core/Cargo.toml',
               reference / 'Cargo.lock', PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock',
               PROJECT / 'examples/query_definitions.rs',
               PARITY / 'rust_values/src/fixture_value.rs', PARITY / 'fixture_value.hpp',
               PARITY / 'debug_modes.hpp',
               PARITY / 'core_query_source_inputs_5_0_6.py', Path(__file__).resolve(),
               ROOT / 'teaql_runtime/include/teaql/runtime/purposed_query.hpp',
               reference / 'teaql-runtime/src/generated_support.rs']
    sources += [path for package in ('teaql_core', 'teaql_memory')
                for path in (ROOT / package).rglob('*')
                if path.is_file() and path.suffix in ('.cpp', '.hpp', '.cppm')]
    gap = ROOT / 'docs/parity/core_query_schema_builder_gap_5_0_6.json.gz'
    assert load(gap)['responsibility_accepted'] is False
    builder = ROOT / 'docs/parity/query_schema_builder_qualification_5_0_6.json.gz'
    qualification = load(builder)
    assert qualification['responsibility_qualified'] is True
    for filename, expected_sha256 in qualification['source_current_sha256'].items():
        assert digest(ROOT / filename) == expected_sha256, filename
    record.update(complete=True, observations=sum(m['cases'] for m in record['matrices']),
                  historical_schema_builder_gap_sha256=digest(gap),
                  schema_builder_qualification_sha256=digest(builder),
                  corpus_sha256=corpus_hashes,
                  sources={str(path.relative_to(ROOT)): dict(sha256=digest(path), text=path.read_text())
                           for path in sources})
    save()
    print(f'Query/row source replay: {record["observations"]} inputs per frontend', flush=True)


if __name__ == '__main__':
    main()
