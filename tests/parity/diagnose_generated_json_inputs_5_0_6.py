#!/usr/bin/env python3
"""Retain independent JSON input observations, including known original failures.

This diagnostic does not produce an accepted parity corpus or suppress failures.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    cases = [{'case': name} for name in ('combined', 'rejections', 'target')]
    cases.extend({'case': 'aliases', 'count': n} for n in (1, 100, 10000))
    observations = []
    for case in [{'case': 'payload_diagnostic'}] + cases:
        entry = {'input': case}
        for runtime, executable in [('rust', args.rust), ('cpp', args.cpp)]:
            if runtime == 'cpp' and case['case'] == 'payload_diagnostic':
                continue
            result = subprocess.run([executable], input=json.dumps(case) + '\n',
                                    text=True, capture_output=True, check=False)
            entry[runtime] = dict(exit_code=result.returncode,
                                  stdout=result.stdout, stderr=result.stderr)
        if 'cpp' in entry:
            entry['equal'] = (entry['rust']['exit_code'] == 0
                              and entry['cpp']['exit_code'] == 0
                              and json.loads(entry['rust']['stdout'])
                              == json.loads(entry['cpp']['stdout']))
        observations.append(entry)
    sources = [Path(__file__).relative_to(ROOT),
               Path('tests/parity/generated_json_inputs_probe.cpp'),
               Path('tests/parity/rust_field_layout/src/bin/generated_json_inputs_5_0_6.rs')]
    receipt = dict(baseline=json.loads((ROOT / 'upstream.lock.json').read_text()),
                   complete=False, scope='Diagnostic; not source or capability acceptance',
                   source_sha256={str(p): hashlib.sha256((ROOT / p).read_bytes()).hexdigest()
                                  for p in sources}, observations=observations)
    args.output.write_bytes(gzip.compress((json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    for observation in observations:
        print(observation['input'], 'equal=', observation.get('equal', 'original only'))


if __name__ == '__main__':
    main()
