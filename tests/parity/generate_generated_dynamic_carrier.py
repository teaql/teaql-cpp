#!/usr/bin/env python3
"""Compare actual generated indexed carrier adapters with frozen Rust entity macros."""
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
    old = dict(kind='text', value='old')
    stored = [{}, {'count':dict(kind='i64', value='9')}, {'title':old},
              {'title':dict(kind='null')}, {'title':old,'count':dict(kind='i64',value='9')}]
    payloads = [dict(kind='text', value=value) for value in ['new', '', '雪\n\t"\\', 'x' * 4096]]
    payloads += [dict(kind='null'), dict(kind='typed_null'), dict(kind='bool', value=True)]
    inputs = []
    for values in stored:
        inputs.extend(dict(mode='normal', stored=values, op='set', code='title', value=value) for value in payloads)
        inputs.append(dict(mode='normal', stored=values, op='delete', code='title', value=dict(kind='null')))
        inputs.extend(dict(mode=mode, stored=values, op=op, code='title', value=old)
                      for mode in ['missing','layout'] for op in ['set','delete'])
        inputs.extend(dict(mode=mode, stored=values, op='set', code='title', value=old)
                      for mode in ['install_owner','install_layout'])
        inputs.extend(dict(mode='normal', stored=values, op=op, code='unknown', value=old)
                      for op in ['set','delete'])
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    expected = observe(args.rust, inputs)
    actual = observe(args.cpp, inputs)
    for index, (original, result) in enumerate(zip(expected, actual)):
        if not same(original, result):
            mismatch = Path('/tmp/teaql_generated_dynamic_carrier_mismatch.json')
            mismatch.write_text(json.dumps(dict(index=index, input=inputs[index], rust=original, cpp=result), indent=2))
            raise SystemExit(f'Generated carrier mismatch {index}; see {mismatch}')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    sources = ['teaql-macros/src/derive_impl.rs','teaql-runtime/src/entity_runtime.rs']
    corpus = dict(baseline=lock['commit'], version=lock['version'],
                  source_hashes={source:hashlib.sha256((ROOT/'.reference/teaql-rs'/source).read_bytes()).hexdigest() for source in sources},
                  note='Direct frozen generated School entity macro and regenerated C++ Product adapters. Explicit model-independent projection compares dynamic values, #code availability, shared snapshot retention/transitions, clone isolation, pending intent and exact common errors. Native per-model layout bit positions/names are outside this projection, not normalized.',
                  cases=[dict(input=value,expected=output) for value,output in zip(inputs,expected)])
    Path(__file__).with_name('generated_dynamic_carrier_5_0_6_cases.json').write_text(json.dumps(corpus,indent=2)+'\n')
    print(f'Compared and recorded {len(inputs)} generated dynamic carrier observations')


if __name__ == '__main__':
    main()
