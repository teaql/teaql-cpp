#!/usr/bin/env python3
"""Check source-test batching boundaries through the C++ relation dispatch operation."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--cpp-emulator', default='')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if args.output and args.output.exists():
        raise RuntimeError('Preserve earlier receipts')
    subprocess.run(['python3', 'scripts/verify_upstream.py'], cwd=ROOT, check=True)
    original = ROOT/'docs/parity/relation_source_original_tests_5_0_6.json.gz'
    receipt = json.loads(gzip.decompress(original.read_bytes()))
    source = ROOT/'.reference/teaql-rs/teaql-runtime/src/data_service/relation.rs'
    assert receipt['complete'] and receipt['provenance']['all_original_bodies_preserved']
    assert receipt['provenance']['source_files_sha256']['src/data_service/relation.rs'] == digest(source)
    assert len(receipt['expected_tests']) == 3
    # (parent identities, provider capability, explicit threshold, selected plan,
    #  callback count with capture disabled, callback count with capture enabled)
    boundaries = [(0, True, None, 'BOUNDED_PROBES', 0, 0),
                  (1, True, None, 'BOUNDED_PROBES', 1, 1),
                  (2, True, None, 'BOUNDED_PROBES', 1, 2),
                  (32, True, None, 'BOUNDED_PROBES', 1, 32),
                  (2, True, 0, 'WINDOW', 1, 1),
                  (2, True, 32, 'BOUNDED_PROBES', 2, 2),
                  (2, False, None, 'WINDOW', 1, 1),
                  (32, True, 32, 'BOUNDED_PROBES', 32, 32)]
    inputs, expected = [], []
    for parents, probes, threshold, plan, quiet, captured in boundaries:
        for capture in [False, True]:
            for compact in [False, True]:
                inputs.append(dict(parents=parents, probes=probes, threshold=threshold,
                                   capture=capture, compact=compact, many=True, query=True,
                                   slice='limit', pending=0, polls=100, fail_at=0))
                expected.append((plan, captured if capture else quiet))
    binary = args.cpp.resolve()
    before = digest(binary)
    result = subprocess.run([*filter(None, args.cpp_emulator.split(';')), str(binary)],
                            input=''.join(json.dumps(row)+'\n' for row in inputs),
                            text=True, capture_output=True, check=True)
    actual = [json.loads(line) for line in result.stdout.splitlines()]
    assert digest(binary) == before and len(actual) == len(inputs) == 32
    for index, (stimulus, row, (plan, count)) in enumerate(zip(inputs, actual, expected)):
        assert 'error' not in row and 'cancelled' not in row, index
        assert len(row['queries']) == count and row['drops'] == count, index
        assert row['rows'] == list(range(1, count+1)), index
        assert row['capabilities_calls'] == (0 if stimulus['parents'] == 0 else 1), index
        assert row['telemetry']['teaql.relation.selected_plan'] == plan, index
        assert row['telemetry']['teaql.relation.parent_count'] == stimulus['parents'], index
        scalar = plan == 'BOUNDED_PROBES' and count == stimulus['parents']
        for query in row['queries']:
            assert query['partition'] == (None if scalar else 'owner_id'), index
            assert query['entity'] == 'Child' and query['relations'] == [], index
    record = dict(complete=True, cpp_contract_passed=True, cases=32,
                  scope='C++ dispatch assertions derived from original planner tests; not a new Rust output differential corpus',
                  original_receipt_sha256=digest(original), executable_sha256=before,
                  controller=Path(__file__).read_text(), inputs=inputs, observations=actual,
                  source_sha256={p: digest(ROOT/p) for p in [
                      'tests/parity/relation_execution_probe.cpp',
                      'teaql_runtime/include/teaql/runtime/detail/data_service/relation_execution.hpp']})
    if args.output:
        args.output.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(), mtime=0))
    print('Relation probe source-test boundaries: 32/32 contracts pass')


if __name__ == '__main__':
    main()
