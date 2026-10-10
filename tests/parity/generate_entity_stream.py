"""Record actual pinned public EntityDataService::fetch_stream observations."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe


def inputs():
    cases = []
    for capture, shape, size, error_at in itertools.product(
            (False, True), ('root', 'relation', 'child', 'object', 'aggregate', 'grouped', 'continuous'),
            (None, 0, 1, 1001), (-1, 0, 1)):
        cases.append(dict(capture=capture, shape=shape, size=size, error_at=error_at,
                          last_first=(size == 1), limit=None, purpose='Inspect root records'))
    for purpose, limit in itertools.product(('', ' \t\n', '\u2003', '  Inspect  ', 'Purpose\nline'),
                                            (0, 1, 10001)):
        cases.append(dict(capture=True, shape='root', size=None, error_at=-1,
                          last_first=False, limit=limit, purpose=purpose))
    for ceiling in (1, 2, 10000, 20000, (1 << 64) - 2):
        for limit in (ceiling, ceiling + 1):
            cases.append(dict(capture=True, shape='root', size=None, error_at=-1,
                              last_first=False, limit=limit, hard_limit=ceiling,
                              purpose='Inspect bounded root records'))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
    cases = inputs()
    outputs = observe(args.rust, cases)
    lock = json.loads((root / 'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'], note='Original public fetch_stream; native Stream fixture only, no copied runtime algorithm.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('entity_stream_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} public entity-stream observations')


if __name__ == '__main__':
    main()
