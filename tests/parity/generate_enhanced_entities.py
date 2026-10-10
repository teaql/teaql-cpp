"""Record actual public enhanced entity calls through the locked runtime."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe


def case(**changes):
    value = dict(capture=True, child_decoder=True, leaf_decoder=True, relations=False,
                 nested=False, behavior=False, enhancement=False, cache=False,
                 idset=False, continuous=False, owned=False, failure='', fail_model='')
    value.update(changes)
    return value


def inputs():
    cases = []
    for capture, decoders, shape, owned, cache in itertools.product((False, True),
            ((True, True), (True, False), (False, False)),
            ('direct', 'simple', 'nested', 'enhanced'), (False, True), (False, True)):
        cases.append(case(capture=capture, child_decoder=decoders[0], leaf_decoder=decoders[1],
                          relations=shape != 'direct', nested=shape in ('nested', 'enhanced'),
                          enhancement=shape == 'enhanced', owned=owned, cache=cache))
    for behavior, idset, continuous, capture in itertools.product((False, True), (False, True), (False, True), (False, True)):
        cases.append(case(behavior=behavior, idset=idset, continuous=continuous, capture=capture,
                          relations=True, nested=True))
    for decoders, field, failure in itertools.product(((True, True), (True, False), (False, False)),
                                                      ('failure', 'fail_model'), ('Parent', 'Child', 'Leaf')):
        cases.append(case(child_decoder=decoders[0], leaf_decoder=decoders[1], relations=True,
                          nested=True, **{field: failure}))
    for explicit, continuous, idset, decoders in itertools.product((False, True), (False, True),
            (False, True), ((True, True), (True, False), (False, False))):
        cases.append(case(sqlite=True, explicit_child_projection=explicit, continuous=continuous,
                          idset=idset, child_decoder=decoders[0], leaf_decoder=decoders[1],
                          relations=True, nested=True))
    return cases


def main():
    parser = argparse.ArgumentParser(); parser.add_argument('--rust', required=True); args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    cases = inputs(); outputs = observe(args.rust, cases)
    lock = json.loads((root / 'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'], note='Original public enhanced entity/owned calls with recording-provider and real SQLite cases; no copied orchestration. Fixed-row provider cases do not implement SQL filtering.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('enhanced_entities_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} enhanced-entity observations')


if __name__ == '__main__':
    main()
