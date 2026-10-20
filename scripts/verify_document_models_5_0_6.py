#!/usr/bin/env python3
"""Compare document model Debug, equality and explicit C++ destination copies."""
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
PARITY = ROOT / 'tests/parity'
sys.path.insert(0, str(PARITY))
from compare_values import observe, same


def cases():
    corpus = json.loads(gzip.decompress((PARITY / 'document_round_trip_5_0_6_cases.json.gz').read_bytes()))
    base = corpus['cases'][0]['input']
    bodies = [None, True, 18446744073709551615, -9223372036854775808, -0.0,
              'line\n\t"\\\0雪', [None, False, -0.0, 'é'],
              {'nested': {'password': 'FIXTURE_ONLY_SECRET'}, 'rows': [1, 2]}]
    durations = [(0, 0), (0, 1), (0, 999999999), (1, 500000000),
                 (17, 1500000001), (18446744073709551615, 999999999)]
    for text_index, label in enumerate(['doc', 'line\n\t"\\\0', '订单é🙂', 'long_' * 400]):
        for duration_index, (seconds, nanos) in enumerate(durations):
            for body_index, body in enumerate(bodies):
                case = copy.deepcopy(base)
                case.update(token=label, command='command_' + label, body=body,
                            seconds=seconds, nanos=nanos, key_id='key_' + label,
                            issued=[-1, 0, 1800000000, 253402300799][text_index],
                            timestamp_nanos=[0, 1, 1000, 1000000, 123456789, 999999999][duration_index])
                case['now'] = case['issued'] + 600
                snapshot = case['snapshot']
                snapshot['model'] = 'model_' + label
                snapshot['business'] = 'business_' + label
                snapshot['scope']['document'] = 'document_' + label
                snapshot['scope']['purpose'] = 'purpose_' + label
                revision = 9223372036854775806 if body_index & 1 else 0
                identifier = 18446744073709551615 if body_index & 2 else 1
                for value in [snapshot['aggregate'], snapshot['scope']]:
                    value.update(id=identifier, version=revision)
                first = snapshot['entities'][0]
                first.update(loaded=[] if body_index == 0 else ['z', 'a', 'a', label],
                             writable=[] if body_index == 0 else ['a'],
                             mutations=['update', 'create', 'remove'] if body_index & 1 else ['update'])
                second = copy.deepcopy(first)
                second.update(key='item:1', id=21)
                snapshot['entities'].append(second)
                relation = snapshot['relations'][0]
                relation.update(rows=[] if body_index == 0 else ['item:1', 'item:0'],
                                completeness='partial' if duration_index & 1 else 'complete',
                                writable=bool(body_index & 1),
                                mutations=['remove', 'create'] if body_index & 1 else [])
                yield case


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, default=ROOT / 'build_native_current/teaql_document_model_debug_probe')
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--write-corpus', action='store_true')
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve prior evidence; choose a fresh record'
    project = PARITY / 'rust_document_models_reference_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected_versions = versions(reference)
    for name, actual in versions(project / 'Cargo.lock').items():
        if name != 'teaql_document_models_reference_506' and not actual <= expected_versions.get(name, set()):
            raise RuntimeError(f'Document oracle dependency differs: {name}: {actual}')
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference)
    record = dict(scope='Actual source document model Debug/equality/Clone observations; '
                        'C++ additionally checks explicit Global copies after actual User deletion. '
                        'No document-service or whole G13/P7 acceptance is inferred.',
                  runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  steps=[], complete=False, reference_registry_versions_preserved=True,
                  reference_registry_checksums_preserved=True)

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
    inputs = list(cases())
    assert len(inputs) == 192
    rust = Path(environment['CARGO_TARGET_DIR']) / 'debug/document_model_debug_506'
    record.update(inputs=inputs, original=observe(str(rust), inputs),
                  cpp=observe(str(args.cpp.resolve()), inputs))
    record['differences'] = [i for i, (a, b) in enumerate(zip(record['original'], record['cpp'])) if not same(a, b)]
    paths = [Path(__file__), project / 'Cargo.toml', project / 'Cargo.lock', reference,
             PARITY / 'document_model_debug_5_0_6.rs', PARITY / 'document_model_debug_probe.cpp',
             PARITY / 'rust_field_layout/src/bin/document_round_trip_5_0_6.rs',
             PARITY / 'document_round_trip_probe.cpp', rust, args.cpp.resolve(),
             ROOT / '.reference/teaql-rs/teaql-runtime/src/document_round_trip.rs',
             ROOT / 'teaql_runtime/include/teaql/runtime/document_model_debug.hpp',
             ROOT / 'teaql_runtime/include/teaql/runtime/document_round_trip.hpp']
    record['hashes'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    save()
    print('Document models:', len(inputs), 'observations;', len(record['differences']), 'differences')
    assert not record['differences'], record['differences'][:10]
    if args.write_corpus:
        destination = PARITY / 'document_model_debug_5_0_6_cases.json.gz'
        assert not destination.exists(), 'Never overwrite an established corpus'
        data = dict(baseline=record['runtime_commit'], version='5.0.6',
                    source='teaql-runtime/src/document_round_trip.rs',
                    source_sha256=record['hashes'][str(ROOT / '.reference/teaql-rs/teaql-runtime/src/document_round_trip.rs')],
                    note=record['scope'], cases=[dict(input=i, expected=o) for i, o in zip(inputs, record['original'])])
        destination.write_bytes(gzip.compress(json.dumps(data, ensure_ascii=False).encode(), mtime=0))
    record['complete'] = True
    save()


if __name__ == '__main__':
    main()
