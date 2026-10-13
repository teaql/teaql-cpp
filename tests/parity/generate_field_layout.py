#!/usr/bin/env python3
"""Record direct Rust 5.0.6 FieldLayout calls and compare the C++ adapter."""
import argparse
from copy import deepcopy
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def observe(binary, inputs):
    process = subprocess.run([str(binary)], input=''.join(json.dumps(case) + '\n' for case in inputs),
                             text=True, capture_output=True, check=True)
    output = [json.loads(line) for line in process.stdout.split('\n') if line]
    if len(output) != len(inputs):
        raise SystemExit('FieldLayout observer returned an incomplete response')
    return output


def cases():
    base = dict(entity='Item', revision='generated-v1',
                indexes=[['id', 0], ['version', 1], ['code', 2]],
                mappings=[['id', 'id', 'db_id'], ['version', 'version', 'db_version'],
                          ['code', 'code', 'db_code']], relations=['lines'],
                expected_members=['id', 'version', 'code'],
                lookups=['db_code', 'id', 'db_version', 'code', 'lines', 'unknown', '#extra', '_derived'])
    result = [deepcopy(base)]
    for property, value in [('entity', ''), ('revision', ''), ('entity', '雪'),
                            ('entity', 'E' * 4096), ('revision', 'R' * 4096),
                            ('relations', ['', 'lines', 'lines', '#extra']),
                            ('expected_members', ['db_id', 'db_version', 'db_code']),
                            ('expected_members', ['id', 'version']),
                            ('expected_members', ['id', 'version', 'code', 'db_code']),
                            ('expected_members', ['id', 'version', 'id']),
                            ('expected_members', ['id', 'version', 'absent']),
                            ('mappings', [])]:
        case = deepcopy(base)
        case[property] = value
        result.append(case)
    for name, position in [('', 2), ('#extra', 2), ('_derived', 2), ('code', 3),
                           ('code', 0), ('code', 1), ('id', 2), ('version', 2), ('code', 65535)]:
        case = deepcopy(base)
        case['indexes'][2] = [name, position]
        result.append(case)
    for slot, triple in [(0, ['absent', 'id', 'db_id']), (0, ['id', '', 'db_id']),
                         (0, ['id', 'id', '']), (1, ['version', 'db_id', 'db_version']),
                         (2, ['code', 'code', 'db_version']), (2, ['id', 'id', 'db_id']),
                         (2, ['db_id', 'code', 'db_code'])]:
        case = deepcopy(base)
        case['mappings'][slot] = triple
        result.append(case)
    for property in ['indexes', 'mappings', 'expected_members']:
        case = deepcopy(base)
        case[property] = list(reversed(case[property]))
        result.append(case)
    for missing in ['id', 'version']:
        case = deepcopy(base)
        case['indexes'] = [[('replacement' if name == missing else name), i]
                           for name, i in case['indexes']]
        result.append(case)
    for count in [2, 31, 32, 63, 64, 65, 66, 129, 130, 141]:
        names = ['id', 'version'] + [f'field_{i}' for i in range(2, count)]
        case = dict(entity='WideItem', revision=f'layout-{count}',
                    indexes=[[name, i] for i, name in enumerate(names)],
                    mappings=[[name, name, f'db_{i}'] for i, name in enumerate(names)],
                    relations=['lines'], expected_members=names,
                    lookups=list(reversed(names)) + [f'db_{i}' for i in range(count)] +
                    ['lines', 'missing', '#custom', '_readonly'])
        result.append(case)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    inputs = cases()
    expected = observe(args.rust.resolve(), inputs)
    actual = observe(args.cpp.resolve(), inputs)
    for i, (left, right) in enumerate(zip(expected, actual)):
        if left != right:
            raise SystemExit(f'FieldLayout source mismatch at case {i}:\n{left}\n{right}')
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'],
                  source='teaql-core/src/load_state.rs::FieldLayout',
                  note='Direct original FieldLayout constructors and getters; excludes LoadedSnapshot and generator qualification.',
                  cases=[dict(input=i, expected=o) for i, o in zip(inputs, expected)])
    Path(__file__).with_name('field_layout_5_0_6_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Compared and recorded {len(inputs)} direct Rust FieldLayout calls')


if __name__ == '__main__':
    main()
