#!/usr/bin/env python3
"""Compare frozen 5.0.6 Checker helpers and entity-comment composition."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from generate_field_layout import observe
ROOT = Path(__file__).resolve().parents[2]
SOURCE = 'examples/shared-load-state/fixtures/narrow/lib/src/school/checker.rs'
RUNTIME_SOURCE = 'teaql-runtime/src/entity_runtime.rs'

def cases():
    out = []
    for text in ['', ' ', '\t\n', '\u0085\u00a0\u1680\u2003\u2028\u202f\u205f\u3000',
                 'a', '雪', 'é🐾', 'e\u0301', ' text ', '\u200b', 'x'*4096]:
        for required in [False, True]:
            for optional in [False, True]:
                for low,high in [(0,0), (1,1), (2,2), (3,10), (4096,4096)]:
                    out.append(dict(kind='checker_helpers', text=text, required=required,
                        optional=optional, min=low, max=high, field='display_name/~雪'))
    for receiving_reason in [None, '', 'receiving root reason']:
        for source_reason in [None, '', 'source root reason']:
            for receiving in [[], [['Item', 7, 'old local'], ['Other', 7, 'other local']]]:
                for source in [[], [['Item', 7, 'new local']], [['Item', 8, '雪'], ['Other', 7, 'replacement']]]:
                    out.append(dict(kind='comments', receiving_reason=receiving_reason,
                        source_reason=source_reason, receiving=receiving, source=source,
                        lookups=[['Item',7],['Item',8],['Other',7],['Unknown',7]]))
    return out

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    inputs = cases()
    expected, actual = observe(args.rust.resolve(),inputs), observe(args.cpp.resolve(),inputs)
    for i,(left,right) in enumerate(zip(expected,actual)):
        if left != right:
            raise SystemExit(f'Generated Checker source mismatch {i}:\n{left}\n{right}')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], source=SOURCE,
        source_sha256=hashlib.sha256((ROOT/'.reference/teaql-rs'/SOURCE).read_bytes()).hexdigest(),
        runtime_source=RUNTIME_SOURCE,
        runtime_source_sha256=hashlib.sha256((ROOT/'.reference/teaql-rs'/RUNTIME_SOURCE).read_bytes()).hexdigest(),
        note='Unmodified retained generated SchoolCheckerLogic methods and original runtime entity-comment composition APIs; complete diagnostics in three naming profiles. Not full model/save qualification.',
        cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
    Path(__file__).with_name('checker_provenance_5_0_6_cases.json').write_text(json.dumps(corpus,indent=2)+'\n')
    print(f'Compared {len(inputs)} Rust Checker helper and entity-comment inputs')

if __name__ == '__main__':
    main()
