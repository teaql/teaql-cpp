#!/usr/bin/env python3
"""Reproduce the complete data-service public model/protocol source foundation."""
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
sys.path.insert(0, str(PARITY))
from compare_values import same
from data_service_request_source_inputs_5_0_6 import cases


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
        parser.error('Use a fresh record path')
    corpus_path = PARITY / 'data_service_request_source_5_0_6_cases.json.gz'
    if args.record_new_corpus and corpus_path.exists():
        parser.error('Do not overwrite an existing source corpus')
    record = dict(complete=False, full_goal_accepted=False, steps=[], matrices=[],
                  scope='Data-service lib.rs public models/default protocols and complete request '
                        'vector/privacy source tests. Backend execution and whole runtime callers remain separate.')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')

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
    original_manifest = reference / 'teaql-data-service/Cargo.toml'
    output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', original_manifest, '--lib'])
    unit_names = ['mutation_comment_is_owned_and_never_derived_from_trace_tail',
                  'annotated_children_do_not_supply_a_batch_root_comment',
                  'trace_comment_cannot_supply_missing_request_comment_or_purpose']
    assert all('tests::' + name + ' ... ok' in output for name in unit_names)
    output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', original_manifest,
                  '--test', 'request_intent_vectors', '--test', 'request_privacy'])
    integration_names = ['shared_request_intent_vectors',
                         'query_execution_projects_owned_intent_and_retains_inherited_relation_route',
                         'native_mutation_execution_materializes_its_owned_root_reason_and_identity',
                         'mutation_execution_keeps_complete_parent_child_lineage_without_duplicate_roots',
                         'mutation_privacy_is_request_owned_and_debug_does_not_expose_it']
    assert all(name + ' ... ok' in output for name in integration_names)
    project = PARITY / 'rust_data_service_source_5_0_6'
    protocol = PARITY / 'rust_sql_protocol_5_0_6'
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project / 'Cargo.toml',
         '--bins'])
    frontends = [('native', args.build_dir.resolve(), 'teaql_')]
    if args.module_build_dir:
        frontends.append(('modules', args.module_build_dir.resolve() / 'modules', 'teaql_module_'))
    matrices = [('model', 'data_service_model_5_0_6_cases.json.gz', 'data_service_model', 'data_service_model_source_506'),
                ('model_scope', 'data_service_model_scope_baseline_5_0_6_cases.json.gz', 'data_service_model_scope', 'data_service_model_source_506'),
                ('defaults', 'executor_defaults_scope_protocol_5_0_6_cases.json.gz', 'executor_defaults_scope', 'executor_defaults_protocol'),
                ('request', None, 'data_service_request_source', 'data_service_request_source_506')]
    request_outputs = None
    for name, filename, cpp_stem, rust_name in matrices:
        if filename:
            path = PARITY / filename
            previous = json.loads(gzip.decompress(path.read_bytes()))
            assert previous['baseline'] == baseline
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
            output = run([executable], payload)
            assert output.endswith('\n'), f'Unfinished observer output: {label}/{name}'
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
                raise RuntimeError(f'Actual source observations differ: {name}/{label}')
            print(f'{name}: {label} {len(inputs)}/{len(inputs)} exact outcomes', flush=True)
        record['matrices'].append(matrix)
        save()
        if name == 'request':
            request_outputs = expected
    corpus = dict(baseline=baseline, scope='Actual request-owned trace projection and identity, '
                  'query-builder independence and nested batches. Exact outputs; no normalization.',
                  cases=[dict(input=value, expected=output) for value, output in zip(cases(), request_outputs)])
    corpus_bytes = gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n').encode(), mtime=0)
    if args.record_new_corpus:
        corpus_path.write_bytes(corpus_bytes)
    else:
        assert corpus_path.read_bytes() == corpus_bytes
    sources = [reference / 'teaql-data-service' / p for p in
               ['src/lib.rs', 'Cargo.toml', 'tests/request_intent_vectors.rs', 'tests/request_privacy.rs',
                'tests/fixtures/request-intent-v1.json']]
    sources += [project / 'Cargo.toml', project / 'Cargo.lock',
                PARITY / 'rust_field_layout/src/bin/data_service_model_5_0_6.rs',
                protocol / 'executor_defaults_protocol.rs', PARITY / 'data_service_request_source_5_0_6.rs',
                PARITY / 'data_service_request_source_probe.cpp',
                PARITY / 'data_service_request_source_inputs_5_0_6.py', Path(__file__),
                ROOT / 'teaql_data_service/tests/request_vector_tests.cpp']
    record.update(complete=True, baseline=baseline, root_unit_tests=unit_names,
                  integration_tests=integration_names, reference_registry_versions_checksums_exact=True,
                  observations_per_frontend=sum(m['cases'] for m in record['matrices']),
                  request_corpus_sha256=digest(corpus_path),
                  sources={str(p.resolve()): dict(sha256=digest(p), text=p.read_text()) for p in sources})
    save()
    print('Source model/default/envelope verification completed; whole runtime/provider acceptance remains separate.')


if __name__ == '__main__':
    main()
