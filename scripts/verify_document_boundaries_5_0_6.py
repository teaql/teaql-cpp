#!/usr/bin/env python3
"""Exercise actual document collection limits and canonical token-size boundaries."""
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


def base():
    historical = json.loads(gzip.decompress((PARITY / 'document_round_trip_5_0_6_cases.json.gz').read_bytes()))
    value = copy.deepcopy(historical['cases'][0]['input'])
    value.update(issued=1800000000, now=1800000000)
    return value


def collection_cases():
    for count in [0, 9999, 10000, 10001]:
        value = base()
        item = copy.deepcopy(value['snapshot']['entities'][0])
        # Isolate collection cardinality from the separate 64 MiB Context budget.
        # Full field-count limits are exercised independently below.
        item.update(loaded=[], writable=[])
        value['snapshot']['entities'] = [dict(copy.deepcopy(item), key=f'r{i}', id=i + 1)
                                        for i in range(count)]
        value['snapshot']['relations'] = []
        value['label'] = f'entities_{count}'
        yield value
    for count in [0, 1023, 1024, 1025]:
        value = base()
        relation = value['snapshot']['relations'][0]
        value['snapshot']['relations'] = [dict(copy.deepcopy(relation), name=f'r{i}', rows=[])
                                         for i in range(count)]
        value['label'] = f'relations_{count}'
        yield value
    for count in [0, 1023, 1024, 1025]:
        value = base()
        fields = [f'f{i}' for i in range(count)]
        value['snapshot']['entities'][0].update(loaded=fields, writable=fields)
        value['label'] = f'fields_{count}'
        yield value
    for count in [9999, 10000, 10001]:
        value = base()
        item = copy.deepcopy(value['snapshot']['entities'][0])
        item.update(loaded=[], writable=[])
        value['snapshot']['entities'] = [dict(copy.deepcopy(item), key=f'r{i}', id=i + 1)
                                        for i in range(min(count, 10000))]
        value['snapshot']['relations'][0]['rows'] = [f'r{i}' for i in range(count)]
        value['label'] = f'relation_rows_{count}'
        yield value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, default=ROOT / 'build_native_current/teaql_document_round_trip_probe')
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--write-corpus', action='store_true')
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve existing evidence; select a fresh record'
    project = PARITY / 'rust_document_models_reference_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected_versions = versions(reference)
    for name, actual in versions(project / 'Cargo.lock').items():
        if name != 'teaql_document_models_reference_506' and not actual <= expected_versions.get(name, set()):
            raise RuntimeError(f'Boundary observer dependency differs: {name}: {actual}')
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference)
    record = dict(scope='Complete collection maxima and exact canonical token-size controls '
                        'through unchanged original/C++ issue/consume observers. Calibration '
                        'only queries original output lengths; no encoder/decoder is reimplemented.',
                  runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  steps=[], calibration=[], complete=False)

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))

    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    command = ['cargo', 'build', '--offline', '--locked', '--manifest-path', str(project / 'Cargo.toml')]
    result = subprocess.run(command, cwd=ROOT, env=environment, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    record['steps'].append(dict(command=command, exit_code=result.returncode, output=result.stdout))
    save()
    result.check_returncode()
    rust = Path(environment['CARGO_TARGET_DIR']) / 'debug/document_round_trip_reference_506'
    template = base()
    names = [f'f{i}_' + 'x' * 7896 for i in range(12)]
    template['snapshot']['entities'][0].update(loaded=names, writable=names)

    def token_length(padding):
        value = copy.deepcopy(template)
        value['snapshot']['business'] = 'B' * padding
        output = observe(str(rust), [value])[0]
        length = len(output['token'])
        record['calibration'].append(dict(business_bytes=padding, token_bytes=length))
        return length

    inputs = list(collection_cases())
    for desired in [255995, 255998, 255999, 256000, 256002]:
        low, high = 1, 16384
        while low < high:
            middle = (low + high) // 2
            if token_length(middle) < desired:
                low = middle + 1
            else:
                high = middle
        assert token_length(low) == desired, desired
        value = copy.deepcopy(template)
        value['snapshot']['business'] = 'B' * low
        value['label'] = f'canonical_token_{desired}'
        inputs.append(value)
    extra = copy.deepcopy(inputs[-5])
    extra.update(label='submitted_token_256001', modification='extra')
    inputs.append(extra)
    assert len(inputs) == 21
    record['inputs'] = inputs
    record['original'] = observe(str(rust), inputs)
    save()
    try:
        record['cpp'] = observe(str(args.cpp.resolve()), inputs)
    except subprocess.CalledProcessError as error:
        record['cpp_process_failure'] = dict(exit_code=error.returncode,
                                             stderr=error.stderr, stdout=error.stdout)
        save()
        raise
    record['differences'] = [i for i, (a, b) in enumerate(zip(record['original'], record['cpp'])) if not same(a, b)]
    paths = [Path(__file__), project / 'Cargo.toml', project / 'Cargo.lock', reference,
             PARITY / 'rust_field_layout/src/bin/document_round_trip_5_0_6.rs',
             PARITY / 'document_round_trip_probe.cpp', rust, args.cpp.resolve(),
             ROOT / '.reference/teaql-rs/teaql-runtime/src/document_round_trip.rs',
             ROOT / 'teaql_runtime/src/document_round_trip.cpp',
             ROOT / 'teaql_runtime/src/document_snapshot_codec.cpp']
    record['hashes'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    save()
    print('Document boundaries:', len(inputs), 'observations;', len(record['differences']), 'differences')
    assert not record['differences'], record['differences']
    for case, output in zip(inputs, record['original']):
        if case['label'].startswith('canonical_token_'):
            length = int(case['label'].rsplit('_', 1)[1])
            assert len(output['token']) == length
            assert (output['error'] is None) == (length <= 256000)
    if args.write_corpus:
        destination = PARITY / 'document_boundaries_5_0_6_cases.json.gz'
        assert not destination.exists(), 'Never overwrite an established corpus'
        destination.write_bytes(gzip.compress(json.dumps(dict(
            baseline=record['runtime_commit'], version='5.0.6',
            source='teaql-runtime/src/document_round_trip.rs', note=record['scope'],
            cases=[dict(input=i, expected=o) for i, o in zip(inputs, record['original'])])).encode(), mtime=0))
    record['complete'] = True
    save()


if __name__ == '__main__':
    main()
