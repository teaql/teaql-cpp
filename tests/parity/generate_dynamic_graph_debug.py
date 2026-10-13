#!/usr/bin/env python3
"""Record unchanged public GraphNode Debug/equality and dynamic copy observations."""
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
    inputs = [dict(mode=mode, bound=bound, value='', after='updated')
              for mode in ['none', 'empty', 'null'] for bound in [False, True]]
    inputs.extend(dict(mode='value', bound=bound, value=value, after=after)
                  for bound in [False, True]
                  for value, after in [('old', 'updated'), ('same', 'same'), ('雪\n\t"\\', 'updated'), ('x'*4096, 'updated')])
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    expected = observe(args.rust, inputs)
    actual = observe(args.cpp, inputs)
    for index, (original, result) in enumerate(zip(expected, actual)):
        if not same(original, result):
            mismatch = Path('/tmp/teaql_dynamic_graph_debug_mismatch.json')
            mismatch.write_text(json.dumps(dict(index=index, input=inputs[index], rust=original, cpp=result), indent=2))
            raise SystemExit(f'Graph protocol mismatch {index}; see {mismatch}')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    sources = ['teaql-runtime/src/graph.rs', 'teaql-core/src/dynamic_fields.rs']
    corpus = dict(baseline=lock['commit'], version=lock['version'],
                  source_hashes={source: hashlib.sha256((ROOT/'.reference/teaql-rs'/source).read_bytes()).hexdigest() for source in sources},
                  note='Direct public GraphNode/definition/value Debug and independent clone equality. One dynamic field avoids randomized Rust HashMap diagnostic ordering; no normalization. C++ uses an explicit Global destination copy.',
                  cases=[dict(input=value, expected=output) for value, output in zip(inputs, expected)])
    Path(__file__).with_name('dynamic_graph_debug_5_0_6_cases.json').write_text(json.dumps(corpus, indent=2)+'\n')
    print(f'Compared and recorded {len(inputs)} graph protocol observations')


if __name__ == '__main__':
    main()
