#!/usr/bin/env python3
"""Record actual descriptor builders, defaults, derived Debug and first-match lookup."""
import argparse
import gzip
import json
import subprocess
from pathlib import Path
from compare_values import observe


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    cases = []
    for dtype in range(10):
        for mode in range(8):
            for declared in (False, True):
                cases.append(dict(type=dtype, mode=mode, entity='UserProfile', property='duplicate',
                                  column='' if mode & 1 else 'mapped_column', relation='orders', target='Order',
                                  local='' if mode & 2 else 'user_id', foreign='order_id', table='profiles',
                                  service=None if mode < 2 else '' if mode < 4 else 'edge\n雪',
                                  length=None if mode == 0 else 0 if mode == 1 else 4294967295,
                                  precision=None if mode < 2 else 0 if mode == 2 else 19,
                                  scale=None if mode < 3 else 4294967295,
                                  masks=None if not declared else [] if mode < 4 else ['secret', 'secret', '雪'],
                                  max=None if mode < 2 else 0 if mode == 2 else 4294967295))
    fields = {
        'property': ['same', 'name', 'data_type', 'nullable', 'column_name', 'is_id',
                     'is_version', 'max_length', 'numeric_precision', 'numeric_scale'],
        'relation': ['same', 'name', 'target_entity', 'local_key', 'foreign_key',
                     'many', 'attach', 'delete_missing'],
        'entity': ['same', 'name', 'table_name', 'data_service', 'properties', 'relations',
                   'audit_mask_fields', 'audit_mask_fields_declared', 'audit_value_max_len',
                   'property_value', 'relation_value', 'property_order', 'relation_order',
                   'mask_order'],
    }
    for model, members in fields.items():
        cases.extend(dict(kind='equality', model=model, field=member) for member in members)
    names = ['', 'A', 'a', 'UserProfile', 'URLParser', 'HTTP', '_A', 'a_B', '1A',
             'éA', '雪A', '😀A', 'ÄBC', 'İA', 'a\x00B', 'A\nB', 'A B', 'a-data']
    # All ASCII boundaries; case folding is deliberately ASCII-only.
    names.extend(chr(code) + 'A' for code in range(128))
    names.extend(prefix + 'ABC' for prefix in ['é', '雪', '😀', 'ß', 'İ', '𐐀'])
    cases.extend(dict(kind='naming', name=name) for name in names)
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Actual metadata API; all fields, nine Debug modes, duplicate lookup and clone snapshots.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    payload = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('meta_api_cases.json.gz').write_bytes(gzip.compress(payload, mtime=0))
    print(f'Recorded {len(cases)} original metadata API inputs')


if __name__ == '__main__':
    main()
