#!/usr/bin/env python3
"""Append full-range temporal inputs without replacing prior logging evidence."""
import argparse
import copy
import json
from pathlib import Path
from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    path = Path(__file__).with_name('context_logging_cases.json')
    corpus = json.loads(path.read_text())
    old_cases = copy.deepcopy(corpus['cases'])
    old_outputs = observe(args.rust, [case['input'] for case in old_cases])
    assert same([case['expected'] for case in old_cases], old_outputs), 'Existing source observations changed'
    point = lambda seconds, nanos=0: dict(seconds=str(seconds), nanos=nanos)
    pairs = [(point(-(1 << 63)), point((1 << 63) - 1, 999999999)),
             (point((1 << 63) - 1, 999999999), point(-(1 << 63))),
             (point(-10000000000, 1), point(10000000000, 999999999)),
             (point(10000000000, 1), point(10000000000, 999999999)),
             (point(-1, 999999999), point(0, 1)),
             (point(0), point((1 << 63) - 1, 999999999))]
    inputs = []
    for start, end in pairs:
        for operation in ('0', '1'):
            case = copy.deepcopy(old_cases[0]['input'])
            metadata = case['steps'][0]['metadata']
            metadata.update(start=start, end=end, operation=operation)
            inputs.append(case)
    existing = {json.dumps(case['input'], sort_keys=True) for case in old_cases}
    inputs = [item for item in inputs if json.dumps(item, sort_keys=True) not in existing]
    outputs = observe(args.rust, inputs)
    corpus['cases'].extend(dict(input=i, expected=o) for i, o in zip(inputs, outputs))
    assert corpus['cases'][:len(old_cases)] == old_cases
    path.write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Preserved {len(old_cases)} logging inputs and outputs; appended {len(inputs)} full-range time inputs')


if __name__ == '__main__':
    main()
