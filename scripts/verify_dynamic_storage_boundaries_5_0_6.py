#!/usr/bin/env python3
"""Replay original SQLite storage and exercise selection/corruption error order."""
import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import observe, same


def inputs():
    base = dict(fields=[['field', 5]], revision='v1', namespace='profile', ids=['7'],
                selection=None, rollback=False,
                writes=[dict(namespace='profile', id='7',
                             changes={'field': dict(kind='text', value='original')})],
                corrupt=None)
    result = []
    mutations = [
        ('invalid_text_payload', "payload='not-json STORED_PAYLOAD_SECRET'"),
        ('blob_payload', 'payload=zeroblob(2)'),
        ('blob_revision', 'definition_revision=zeroblob(2)'),
        ('blob_type', 'data_type=zeroblob(2)'),
        ('blob_code', 'code=zeroblob(2)'),
        ('text_null_flag', "is_null='bad'"),
        ('noncanonical_true_null', 'is_null=2,payload=NULL'),
        ('missing_nonnull_payload', 'is_null=0,payload=NULL'),
    ]
    for mode, selection in [('all', None), ('empty', []), ('field', [['field', 5]])]:
        for label, mutation in mutations:
            case = copy.deepcopy(base)
            case.update(label=mode + '_' + label, selection=selection,
                        corrupt='PRAGMA ignore_check_constraints=ON; '
                                'UPDATE teaql_dynamic_field_storage_v1 SET ' + mutation)
            result.append(case)
        for ids in [[], ['7']]:
            case = copy.deepcopy(base)
            case.update(label=mode + '_dropped_table_' + str(len(ids)), selection=selection,
                        ids=ids, corrupt='DROP TABLE teaql_dynamic_field_storage_v1')
            result.append(case)
    for namespace in ['', 'profile']:
        for selection in [[['field', 1]], [['missing', 5]]]:
            case = copy.deepcopy(base)
            case.update(label='precedence_' + repr((namespace, selection)),
                        namespace=namespace, selection=selection,
                        corrupt='DROP TABLE teaql_dynamic_field_storage_v1')
            result.append(case)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, default=ROOT / 'build_native_current/teaql_sqlite_dynamic_storage_probe')
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve prior evidence; choose a new record'
    project = ROOT / 'tests/parity/rust_dynamic_storage_boundaries_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected = versions(reference)
    for name, resolved in versions(project / 'Cargo.lock').items():
        if name != 'teaql_dynamic_storage_boundaries_506' and not resolved <= expected.get(name, set()):
            raise RuntimeError(f'Observer dependency differs from reference: {name}: {resolved}')
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference)
    record = dict(scope='Original SQLite dynamic-store source and unchanged C++ observer; '
                        'complete historical replay plus selection/corruption boundaries',
                  runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  steps=[], comparisons=[], complete=False,
                  reference_registry_versions_preserved=True,
                  reference_registry_checksums_preserved=True)

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record, ensure_ascii=False).encode(), mtime=0))

    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    for command in [['python3', 'scripts/verify_upstream.py'],
                    ['cargo', 'build', '--offline', '--locked', '--manifest-path', str(project / 'Cargo.toml')]]:
        completed = subprocess.run(command, cwd=ROOT, env=environment, text=True,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        record['steps'].append(dict(command=command, exit_code=completed.returncode, output=completed.stdout))
        save()
        completed.check_returncode()
    rust = Path(environment['CARGO_TARGET_DIR']) / 'debug/teaql_dynamic_storage_boundaries_506'
    corpus_path = ROOT / 'tests/parity/sqlite_dynamic_storage_5_0_6_cases.json'
    corpus = json.loads(corpus_path.read_text())
    assert corpus['baseline'] == record['runtime_commit']
    matrices = [('historical_complete', [row['input'] for row in corpus['cases']],
                 [row['expected'] for row in corpus['cases']]),
                ('selection_and_corruption', inputs(), None)]
    for name, cases, retained in matrices:
        original = observe(str(rust), cases)
        actual = observe(str(args.cpp.resolve()), cases)
        differences = [i for i, (a, b) in enumerate(zip(original, actual)) if not same(a, b)]
        retained_differences = ([] if retained is None else
                                [i for i, (a, b) in enumerate(zip(retained, original)) if not same(a, b)])
        record['comparisons'].append(dict(name=name, inputs=cases, original=original, cpp=actual,
                                           differences=differences, retained_differences=retained_differences))
        save()
        assert not differences and not retained_differences, name
        print(name, len(cases), 'exact comparisons', flush=True)

    def check_errors(value):
        if isinstance(value, dict):
            if 'error' in value:
                assert 'STORED_PAYLOAD_SECRET' not in json.dumps(value)
            for child in value.values():
                check_errors(child)
        elif isinstance(value, list):
            for child in value:
                check_errors(child)

    check_errors(record['comparisons'][-1]['original'])
    check_errors(record['comparisons'][-1]['cpp'])
    paths = [Path(__file__), project / 'Cargo.toml', project / 'Cargo.lock', reference,
             ROOT / 'tests/parity/rust_field_layout/src/bin/sqlite_dynamic_storage_5_0_6.rs',
             ROOT / 'tests/parity/rust_values/src/fixture_value.rs', corpus_path,
             ROOT / 'tests/parity/sqlite_dynamic_storage_probe.cpp', args.cpp, rust]
    paths.extend(ROOT / '.reference/teaql-rs' / p for p in corpus['source'])
    record['hashes'] = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    record['complete'] = True
    save()


if __name__ == '__main__':
    main()
