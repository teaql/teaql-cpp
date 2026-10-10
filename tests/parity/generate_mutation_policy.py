"""Record public Rust v5.0.5 policy review, approval, warning and exception behavior."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe


def inputs():
    cases = []
    for policy, approval, sink, repeat in itertools.product(
            ['none', 'allow', 'deny', 'throw', 'registry_throw'],
            ['none', 'match', 'id', 'version', 'fingerprint', 'trim', 'blank', 'throw'],
            ['ok', 'error', 'throw'], [1, 3]):
        cases.append(dict(policy=policy, approval=approval, sink=sink, repeat=repeat,
                          request='Order.saveGraph', code=' DENIED ', message=''))
    for code, message, request in itertools.product(
            [None, '', 'custom', '\u2003'], [None, '', 'reason\x00tail'],
            ['', 'Order|none|MUTATION-POLICY-001', 'Unicode\u2003.saveGraph']):
        cases.append(dict(policy='deny', approval='match', sink='ok', repeat=2,
                          request=request, code=code, message=message))
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
    corpus = dict(baseline=lock['commit'],
                  note='Original public UserContext review methods; fixtures implement SPI callbacks only. Covers all identity components, mutated blank approvers, denial option fields, warning repetition, sink Result errors and propagated exceptions.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('mutation_policy_cases.json').write_text(
        json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} mutation-policy observations')


if __name__ == '__main__':
    main()
