#!/usr/bin/env python3
"""Extend graph observations with actual pinned RuntimeModule graph registrations."""
import argparse
import json
import subprocess
from pathlib import Path

from compare_values import observe, same


def inputs():
    sequences = [
        [],
        [('initial', [0]), ('root', [1]), ('initial', [2]), ('root', [0])],
        [('initials', [0, 1, 2]), ('roots', [2, 1, 0])],
        [('roots', [0, 1]), ('initials', [2, 0]), ('roots', []), ('initials', [])],
        [('initials', [0, 0]), ('root', [0]), ('roots', [0, 0]), ('initial', [0])],
        [('initial', [2]), ('initials', [1, 0]), ('root', [1]), ('roots', [2, 0])],
        [('root', [2]), ('roots', [1, 2]), ('initials', [2, 1]), ('initial', [0]), ('roots', [0])],
        [('initials', []), ('roots', []), ('root', [0]), ('initial', [1])],
    ]
    for operation in range(4):
        nodes = []
        for index in range(3):
            child = dict(entity='Child-关系-' + 'x' * 80, operation=(operation + index) % 4,
                         values={'id': dict(kind='u64', value=str(index + 1)),
                                 'payload': dict(kind='list', value=[dict(kind='text', value='nested-λ' * 24)])},
                         comment='', dirty=[], old={}, relations={})
            nodes.append(dict(
                entity=f'Root-{index}-雪', operation=operation,
                values={'id': dict(kind='u64', value=str((1 << 64) - 1 - index)),
                        'payload': dict(kind='object', value={'private': dict(kind='text', value='long-\u0000-\u2028-é' * 16)})},
                comment=[None, '', 'intent-λ' * 20][index],
                dirty=[None, [], ['payload', 'id']][index],
                old=[None, {}, {'payload': dict(kind='text', value='original-' * 16)}][index],
                relations={'children': [child, child], 'empty': []},
            ))
        for sequence in sequences:
            yield dict(mode='module_graphs', nodes=nodes,
                       steps=[dict(op=operation, indices=indices) for operation, indices in sequence])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
    path = Path(__file__).with_name('runtime_graph_cases.json')
    corpus = json.loads(path.read_text())
    baseline = json.loads((root / 'upstream.lock.json').read_text())['commit']
    if corpus['baseline'] != baseline:
        raise RuntimeError('Graph observations belong to a different source baseline')
    preserved = [case for case in corpus['cases'] if case['input']['mode'] != 'module_graphs']
    cases = preserved + [dict(input=value) for value in inputs()]
    observed = observe(args.rust, [case['input'] for case in cases])
    if not same(observed[:len(preserved)], [case['expected'] for case in preserved]):
        raise RuntimeError('Existing graph observations changed')
    if not same(observed, observe(args.rust, [case['input'] for case in cases])):
        raise RuntimeError('Original module graph observations changed between executions')
    for case, output in zip(cases, observed):
        case['expected'] = output
    corpus['cases'] = cases
    path.write_text('{"baseline":' + json.dumps(baseline) + ',"cases":[\n' +
                    ',\n'.join(json.dumps(case, ensure_ascii=True, separators=(',', ':')) for case in cases) +
                    '\n]}\n')
    print(f'Preserved {len(preserved)} graph inputs; recorded 32 original module graph inputs; '
          f'revalidated {len(cases)} observations twice')


if __name__ == '__main__':
    main()
