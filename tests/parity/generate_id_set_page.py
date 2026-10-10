"""Record the original private ID-set method, not a reproduced algorithm."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe


def inputs():
    cases = []
    for capture, shape, root in itertools.product((False, True),
            ('plain', 'desc', 'tie', 'aggregate', 'grouped', 'continuous', 'raw', 'partition', 'having', 'expr_order'),
            (None, ['Tenant', 42], ['Tenant', 43], ['Organization', 42])):
        cases.append(dict(capture=capture, shape=shape, root=root, enabled=True,
                          max_ids=3, ids=[1, 2, 3], limit=1, offset=1, calls=2,
                          fail_read=0, fail_put=False, failure=False))
    for ids, max_ids, limit, offset in itertools.product(([], [1], [1, 1, 3], [None], ['invalid'], [1, 2, 3, 4]),
                                                       (1, 3), (None, 0, 2), (0, 5, (1 << 64) - 1)):
        cases.append(dict(capture=True, shape='plain', root=None, enabled=True,
                          max_ids=max_ids, ids=ids, limit=limit, offset=offset, calls=2,
                          fail_read=0, fail_put=False, failure=False))
    for enabled, fail_read, fail_put, failure in itertools.product((False, True), (0, 1, 2), (False, True), (False, True)):
        cases.append(dict(capture=True, shape='plain', root=['Tenant', 42], enabled=enabled,
                          max_ids=3, ids=[1, 2, 3], limit=1, offset=0, calls=2,
                          fail_read=fail_read, fail_put=fail_put, failure=failure))
    for entity, id_value in itertools.product(('Tenant', 'tenant', 'TenantID', 'mixed-Root', '', ' \t\n', '\u2003'),
                                             (0, 42, (1 << 64) - 1)):
        cases.append(dict(capture=True, root=None, fail_read=0, fail_put=False,
                          root_controls=[['require', entity], ['bootstrap', entity, id_value],
                                         ['require', entity], ['bootstrap', entity, id_value],
                                         ['bootstrap', 'Organization', 42], ['require', 'Tenant']]))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
    lock = json.loads((root / 'upstream.lock.json').read_text())
    cases = inputs()
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=lock['commit'], note='Original private prepare_id_set_page in a git archive with one field-only public bridge. The reference is unchanged.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('id_set_page_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} ID-set page observations')


if __name__ == '__main__':
    main()
