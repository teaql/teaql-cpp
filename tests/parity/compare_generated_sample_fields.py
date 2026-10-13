#!/usr/bin/env python3
"""Compare full actual sample outcomes without temporal, numeric or error normalization."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
from compare_values import same
from sample_clock_driver import observe_fixed
ROOT = Path(__file__).resolve().parents[2]
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--clock', required=True)
    args = parser.parse_args()
    corpus = json.loads(gzip.decompress(Path(__file__).with_name('generated_sample_fields_5_0_6_cases.json.gz').read_bytes()))
    assert corpus['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert corpus['clock_fixture_sha256'] == hashlib.sha256((ROOT / 'tests/support/fixed_sample_clock.cpp').read_bytes()).hexdigest()
    for kind, fixture in [('temporal', 'sample_fields_temporal'), ('json_runtime', 'sample_fields_runtime')]:
        directory = ROOT / 'tests/generator/parity' / (fixture + '_rust')
        producer = corpus['producers'][kind]
        assert producer == json.loads((directory / 'PROVENANCE.json').read_text())
        assert hashlib.sha256((ROOT / producer['model']).read_bytes()).hexdigest() == producer['model_sha256']
        for file, digest in producer['files'].items():
            assert hashlib.sha256((directory / file).read_bytes()).hexdigest() == digest, file
    inputs = [case['input'] for case in corpus['cases']]
    expected = [case['expected'] for case in corpus['cases']]
    if args.rust and not same(expected, observe_fixed(args.rust, inputs, args.clock)):
        raise RuntimeError('Committed sample fields differ from live original outcomes')
    actual = observe_fixed(args.cpp, inputs, args.clock)
    failures = [index for index, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    for index in failures[:12]:
        print(f'Case {index}: {inputs[index]}\n Rust: {expected[index]}\n C++:  {actual[index]}')
    print(f'Generated sample fields parity: {len(inputs) - len(failures)}/{len(inputs)} observations match')
    raise SystemExit(bool(failures))
