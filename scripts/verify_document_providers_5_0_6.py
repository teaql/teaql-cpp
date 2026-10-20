#!/usr/bin/env python3
"""Compare actual document-provider callbacks, tokens and failure ordering."""
import argparse
import copy
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
    retained = json.loads(gzip.decompress((PARITY / 'document_round_trip_5_0_6_cases.json.gz').read_bytes()))['cases'][0]['input']
    variants = []
    entries = ['direct', 'context']
    errors = ['invalid', 'expired', 'scope', 'authorization', 'configuration']
    for entry, error in itertools.product(entries, errors):
        variants.append(dict(entry=entry, startup_error=error))
        variants.append(dict(entry=entry, current_error=error))
        variants.append(dict(entry=entry, lookup_error=error))
        for position in [1, 2, 3]:
            variants.append(dict(entry=entry, deny_issue_at=position, deny_issue_error=error))
            variants.append(dict(entry=entry, deny_consume_at=position, deny_consume_error=error))
    for entry, mode in itertools.product(entries, ['missing', 'mismatch', 'wrong_master']):
        variants.append(dict(entry=entry, lookup_mode=mode))
    for entry, delta in itertools.product(entries, [-61, -60, -59, 0, 59, 60, 61]):
        variants.append(dict(entry=entry, delta=delta))
    for entry, seed, reissue, rotation in itertools.product(entries, [0, 51, 255], [False, True], [False, True]):
        variants.append(dict(entry=entry, seed=seed, reissue=reissue, rotation=rotation))
    for builders, (has_runtime, has_principal) in itertools.product([False, True], [(False, False), (False, True), (True, False)]):
        variants.append(dict(entry='context', builders=builders, has_runtime=has_runtime, has_principal=has_principal))
    for builders, namespace, actor in itertools.product([False, True], ['same', 'different'], ['same', 'different']):
        variants.append(dict(entry='context', builders=builders, namespace=namespace, actor=actor, replace=True))
    for entry, invalid in itertools.product(entries, ['principal', 'snapshot', 'lifetime']):
        variants.append(dict(entry=entry, invalid=invalid))
    assert len(variants) == 154
    for index, variant in enumerate(variants):
        case = copy.deepcopy(retained)
        case.update(entry='direct', startup_error='', current_error='', lookup_error='',
                    lookup_mode='present', deny_issue_at=0, deny_issue_error='', deny_consume_at=0,
                    deny_consume_error='', seed=51, reissue=False, rotation=False, has_runtime=True,
                    has_principal=True, builders=False, replace=False, namespace='same', actor='same',
                    invalid='', delta=0, nanos=123456789 if index & 1 else 0)
        case.update(variant)
        second = copy.deepcopy(case['snapshot']['entities'][0])
        second.update(key='item:1', id=21)
        case['snapshot']['entities'].append(second)
        case['snapshot']['relations'][0]['rows'].append('item:1')
        if index % 3 == 0:
            case['snapshot']['model'] = 'model_订单é'
        yield case


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, default=ROOT / 'build_native_current/teaql_document_providers_probe')
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--write-corpus', action='store_true')
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve previous evidence'
    project = PARITY / 'rust_document_providers_reference_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected_versions = versions(reference)
    for name, actual in versions(project / 'Cargo.lock').items():
        if name != 'teaql_document_providers_reference_506':
            assert actual <= expected_versions.get(name, set()), (name, actual)
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference)
    inputs = list(cases())
    assert len(inputs) == 154
    record = dict(scope='Actual frozen document issue/consume with custom key, authorization, clock and nonce providers. '
                        'Exact callback arguments/order, token bytes, all provider errors, selected-key rotation, '
                        'time boundaries and direct/Context registration/replacement outcomes. '
                        'C++ copies observations into Global, disables default PMR and deletes UserContext. '
                        'No application document projection/save implementation or whole G13/P7 acceptance inferred.',
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
    rust = Path(environment['CARGO_TARGET_DIR']) / 'debug/document_providers_506'
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
             PARITY / 'document_providers_5_0_6.rs', PARITY / 'document_providers_probe.cpp',
             PARITY / 'rust_field_layout/src/bin/document_round_trip_5_0_6.rs',
             PARITY / 'document_round_trip_probe.cpp', rust, args.cpp.resolve(),
             ROOT / '.reference/teaql-rs/teaql-runtime/src/context.rs',
             ROOT / '.reference/teaql-rs/teaql-runtime/src/document_round_trip.rs',
             ROOT / '.reference/teaql-rs/teaql-runtime/src/round_trip_reference.rs',
             ROOT / 'teaql_runtime/src/document_snapshot_codec.cpp',
             ROOT / 'teaql_runtime/src/round_trip_reference.cpp',
             ROOT / 'teaql_runtime/src/context/document_round_trip.cpp',
             ROOT / 'teaql_runtime/include/teaql/runtime/round_trip_reference.hpp',
             ROOT / 'teaql_runtime/include/teaql/runtime/detail/reference_crypto.hpp']
    record['hashes'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    save()
    print(f'Document providers: {len(inputs)} observations; {len(record["differences"])} differences')
    assert not record['differences'], record['differences'][:10]
    if args.write_corpus:
        destination = PARITY / 'document_providers_5_0_6_cases.json.gz'
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
