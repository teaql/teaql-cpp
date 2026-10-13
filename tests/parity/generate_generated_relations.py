#!/usr/bin/env python3
"""Record direct frozen generated-model relation evaluation and handle calls."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import subprocess

from generate_field_layout import observe

ROOT = Path(__file__).resolve().parents[2]
SOURCES = [
    'examples/shared-load-state/fixtures/narrow/lib/src/school/entity.rs',
    'examples/shared-load-state/fixtures/narrow/lib/src/platform/entity.rs',
    'teaql-runtime/src/entity_runtime.rs',
]


def cases():
    return [dict(kind='generated_relations', forward=forward, selected=selected,
                 cached=cached, count=count, foreign_id=foreign_id)
            for forward, selected, cached, count, foreign_id in itertools.product(
                ['missing', 'present', 'empty', 'wrong_type'], [False, True],
                [False, True], [None, 0, 1, 2], [0, 9])]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    inputs = cases()
    expected = observe(args.rust.resolve(), inputs)
    actual = observe(args.cpp.resolve(), inputs)
    for index, (left, right) in enumerate(zip(expected, actual)):
        if left != right:
            raise SystemExit(f'Generated relation mismatch {index}: {inputs[index]}\n{left}\n{right}')
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'],
                  source_sha256={source: hashlib.sha256(
                      (ROOT / '.reference/teaql-rs' / source).read_bytes()).hexdigest()
                      for source in SOURCES},
                  note='Unmodified retained School/Platform generated-model calls. C++ uses matching entity/relation identities in a compact supported fixture. No source output normalization. Optional and reverse to-one native contracts are separate evidence.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_relations_5_0_6_cases.json').write_text(
        json.dumps(corpus, indent=2) + '\n')
    print(f'Compared {len(inputs)} actual generated Rust relation inputs')


if __name__ == '__main__':
    main()
