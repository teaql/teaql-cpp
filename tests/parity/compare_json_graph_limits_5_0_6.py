#!/usr/bin/env python3
"""Check graph node limits against retained actual-source observations."""
import argparse
import gzip
import json
from pathlib import Path
from compare_values import observe, same

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cpp', required=True)
parser.add_argument('--cpp-emulator', default='')
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
with gzip.open(Path(__file__).with_name('json_graph_limits_5_0_6_cases.json.gz'), 'rt') as source:
    corpus = json.load(source)
assert corpus['baseline'] == json.loads((root / 'upstream.lock.json').read_text())['commit']
if type(corpus.get('cases')) is not list or not corpus['cases']:
    raise RuntimeError('Comparison corpus cases must be a nonempty list')
cases = corpus['cases']
actual = observe(args.cpp, [case['input'] for case in cases],
                 emulator=[part for part in args.cpp_emulator.split(';') if part],
                 arguments=['--large-fixture'])
assert same([case['expected'] for case in cases], actual), 'JSON graph-limit outcomes differ'
print('Matched 11 JSON graph boundary/view cases')
