#!/usr/bin/env python3
"""Compare actual document-service dispatch, wake, cancellation and replacement."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
sys.path.insert(0, str(PARITY))
from compare_values import same


def cases():
    for index, (method, pending, error, finish, replace) in enumerate(itertools.product(
            ['open', 'accept'], [0, 1, 3], ['', 'revision', 'validation', 'projection', 'not_found',
                                        'invalid', 'expired', 'scope', 'authorization', 'configuration'],
            ['ready', 'drop', 'close', 'unpolled'], [False, True])):
        if finish == 'close' and replace:
            continue
        yield dict(method=method, pending=pending, error=error, finish=finish, replace=replace,
                   service=True, principal=True, marker=bool(index & 1),
                   label=['first-service', 'service_订单\0é', 'long_' * 512][index % 3],
                   body=[None, {'rows': [False, 7, 'line\n雪\0']}, []][index % 3])
    for service, principal in [(False, False), (False, True), (True, False)]:
        for method, finish in itertools.product(['open', 'accept'], ['ready', 'unpolled']):
            yield dict(method=method, pending=1, error='', finish=finish, replace=False,
                       service=service, principal=principal, marker=False, label='absent', body=None)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, default=ROOT / 'build_native_current/teaql_document_service_async_probe')
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--write-corpus', action='store_true')
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve previous evidence'
    project = PARITY / 'rust_document_services_reference_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected_versions = versions(reference)
    for name, actual in versions(project / 'Cargo.lock').items():
        if name != 'teaql_document_services_reference_506':
            assert actual <= expected_versions.get(name, set()), (name, actual)
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference)
    inputs = list(cases())
    assert len(inputs) == 432
    record = dict(scope='Actual source Context open/accept dispatch with application-owned service; '
                        'complete results, request/principal/resource callbacks, poll/wake and drop. '
                        'C++ additionally cancels on actual UserContext deletion with null default PMR. '
                        'Rust drops its borrowed future before Context deletion; destructor timing '
                        'under resource replacement is an explicit Context ownership adaptation. '
                        'No application projection/save service or complete SQ08/G13 acceptance inferred.',
                  baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  registry_versions_preserved=True, registry_checksums_preserved=True,
                  inputs=inputs, steps=[], complete=False)

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record, ensure_ascii=False).encode(), mtime=0))

    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    for command in [['python3', 'scripts/verify_upstream.py'],
                    ['cargo', 'build', '--offline', '--locked', '--manifest-path', str(project / 'Cargo.toml')]]:
        result = subprocess.run(command, cwd=ROOT, env=environment, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        record['steps'].append(dict(command=command, exit_code=result.returncode, output=result.stdout))
        save()
        result.check_returncode()
    rust = Path(environment['CARGO_TARGET_DIR']) / 'debug/document_service_async_506'
    payload = ''.join(json.dumps(case, ensure_ascii=True) + '\n' for case in inputs)
    for key, executable in [('original', rust), ('cpp', args.cpp.resolve())]:
        result = subprocess.run([str(executable)], input=payload, env=environment,
                                text=True, capture_output=True)
        record[key + '_process'] = dict(exit_code=result.returncode, stderr=result.stderr, stdout=result.stdout)
        save()
        result.check_returncode()
        record[key] = [json.loads(line) for line in result.stdout.split('\n') if line]
        assert len(record[key]) == len(inputs), (key, len(record[key]))
        save()
    record['differences'] = [i for i, (a, b) in enumerate(zip(record['original'], record['cpp'])) if not same(a, b)]
    paths = [Path(__file__), project / 'Cargo.toml', project / 'Cargo.lock', reference,
             PARITY / 'document_service_async_5_0_6.rs', PARITY / 'document_service_async_probe.cpp',
             rust, args.cpp.resolve(), ROOT / '.reference/teaql-rs/teaql-runtime/src/context.rs',
             ROOT / '.reference/teaql-rs/teaql-runtime/src/document_round_trip.rs',
             ROOT / 'teaql_runtime/src/context/document_round_trip.cpp',
             ROOT / 'teaql_runtime/include/teaql/runtime/context.hpp',
             ROOT / 'teaql_async/include/teaql/async/operation.hpp']
    record['hashes'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    save()
    print(f'Document service async: {len(inputs)} observations; {len(record["differences"])} differences')
    assert not record['differences'], record['differences'][:10]
    if args.write_corpus:
        destination = PARITY / 'document_service_async_5_0_6_cases.json.gz'
        assert not destination.exists(), 'Never replace an established corpus'
        corpus = dict(baseline=record['baseline'], version='5.0.6', note=record['scope'],
                      source='teaql-runtime/src/context.rs',
                      source_sha256=record['hashes'][str(ROOT / '.reference/teaql-rs/teaql-runtime/src/context.rs')],
                      cases=[dict(input=i, expected=o) for i, o in zip(inputs, record['original'])])
        destination.write_bytes(gzip.compress(json.dumps(corpus, ensure_ascii=False).encode(), mtime=0))
    record['complete'] = True
    save()


if __name__ == '__main__':
    main()
