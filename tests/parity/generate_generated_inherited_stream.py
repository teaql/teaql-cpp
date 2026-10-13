#!/usr/bin/env python3
"""Observe actual inherited streaming outcomes and post-close SQLite reuse."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    models = ['school', 'academy', 'research_academy']
    for source, target, entry, chunk, stop, fault in itertools.product(
            models, models, ['normal', 'minimal'], [1, 2, 4], [0, 1, 2, 99], ['none', 'bad_rank']):
        yield dict(source=source, target=target, entry=entry, chunk=chunk, stop=stop, fault=fault)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    inputs = list(cases())
    expected = observe(str(args.rust.resolve()), inputs)
    actual = observe(str(args.cpp.resolve()), inputs)
    failures = [(i, case, a, b) for i, (case, a, b) in enumerate(zip(inputs, expected, actual)) if not same(a, b)]
    for i, case, a, b in failures[:3]:
        print(f'Mismatch {i}: {case}\nRust: {a}\nC++: {b}')
    if failures:
        raise SystemExit(f'{len(failures)} / {len(inputs)} inherited stream observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    producer = json.loads((ROOT/'tests/generator/parity/inheritance_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=producer,
                  note='Actual unchanged original generated runtime/request/model. Source/result streaming, chunk sizes, no polling, early stop, exhaustion and invalid second-row rank decoding; exact native values/errors and post-close executor reuse. Rust Drop releases the pool guard; C++ explicit cancel resets the cursor and releases its connection lease while prepared statements may remain cached. Rust availability uses try_lock; C++ availability requires an immediate real-query poll, and both require successful subsequent query reuse. A malformed row rejects the whole chunk. This is outcome/cleanup evidence, not identical startup or error-delivery timing.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_inherited_stream_5_0_6_cases.json.gz').write_bytes(
        gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
    print(f'Compared {len(inputs)} actual inherited stream observations')


if __name__ == '__main__':
    main()
