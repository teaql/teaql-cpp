#!/usr/bin/env python3
"""Compare policy observations through unchanged public typed ledger save calls."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--cpp', required=True)
    args = parser.parse_args()
    payloads = [dict(kind='text', value=value) for value in ['pending', '雪\n\t"\\', 'x' * 4096]]
    payloads += [dict(kind='null'), dict(kind='typed_null')]
    inputs = [dict(mode='set', native=native, other=other, value=value)
              for native in [False, True] for other in [False, True] for value in payloads]
    inputs += [dict(mode='delete', native=native, other=other, value=dict(kind='null'))
               for native in [False, True] for other in [False, True]]
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    expected = observe(args.rust, inputs)
    actual = observe(args.cpp, inputs)
    for index, (original, result) in enumerate(zip(expected, actual)):
        if not same(original, result):
            mismatch = Path('/tmp/teaql_dynamic_governance_mismatch.json')
            mismatch.write_text(json.dumps(dict(index=index, input=inputs[index], rust=original, cpp=result), indent=2))
            raise SystemExit(f'Governance mismatch {index}; see {mismatch}')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    sources = ['teaql-runtime/src/entity_save.rs', 'teaql-runtime/src/mutation_policy.rs']
    corpus = dict(baseline=lock['commit'], version=lock['version'],
                  source_hashes={source: hashlib.sha256((ROOT/'.reference/teaql-rs'/source).read_bytes()).hexdigest() for source in sources},
                  note='Unchanged public typed ledger save invokes a denying policy in both implementations. Compare canonical request/root/reason, ordered entity operations, original versions, native/dynamic operands, exact denial and retained pending intent. Volatile execution IDs are outside the explicit projection. No SQL occurs after denial; no output normalization.',
                  cases=[dict(input=value, expected=output) for value, output in zip(inputs, expected)])
    Path(__file__).with_name('dynamic_governance_5_0_6_cases.json').write_text(json.dumps(corpus, indent=2)+'\n')
    print(f'Compared and recorded {len(inputs)} public dynamic governance observations')


if __name__ == '__main__':
    main()
