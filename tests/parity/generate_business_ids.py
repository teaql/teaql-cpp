#!/usr/bin/env python3
"""Compare unmodified frozen-generated Business-ID wrappers and Checkers."""
import argparse
from copy import deepcopy
import gzip
import itertools
import json
from pathlib import Path
import subprocess

from generate_field_layout import observe

ROOT = Path(__file__).resolve().parents[2]
RESOURCE_MESSAGES = {
    'missing typed resource: teaql_runtime::business_id::BusinessIdService':
        'missing typed resource: teaql::BusinessIdService',
}


def adapt_source(value):
    value = deepcopy(value)
    for profile in value['diagnostics']:
        for result in profile:
            message = result.get('message')
            if message in RESOURCE_MESSAGES:
                result['message'] = RESOURCE_MESSAGES[message]
    return value


def cases():
    out = []
    for entity, root, service, key, update, loaded, dirty, before, tenant in itertools.product(
            ['receipt', 'ticket'], [None, 42], [False, True], [False, True],
            [False, True], [False, True], [False, True],
            ['', 'RCP-20261011-ABC123', 'TKT-20261011-00000001', 'bad-format'], [0, 43]):
        out.append(dict(kind='business_id', entity=entity, root=root, service=service, key=key,
                        update=update, loaded=loaded, dirty=dirty, before=before, tenant=tenant,
                        text=' \u3000value\u00a0 '))
    for entity in ['receipt', 'ticket']:
        for text in ['', ' ', '\t\n', '\u0085\u00a0\u1680\u2003\u2028\u202f\u205f\u3000',
                     'value', '雪', 'é🐾', 'e\u0301', '\u200b', 'x' * 4096]:
            out.append(dict(kind='business_id', entity=entity, root=42, service=True, key=True,
                            update=False, loaded=True, dirty=False, before='', tenant=0, text=text))
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    inputs = cases()
    expected, actual = observe(args.rust.resolve(), inputs), observe(args.cpp.resolve(), inputs)
    adapted = [adapt_source(value) for value in expected]
    for index, (left, right) in enumerate(zip(adapted, actual)):
        if left != right:
            raise SystemExit(f'Business-ID mismatch {index}: {inputs[index]}\n{left}\n{right}')
    provenance = json.loads((ROOT / 'tests/generator/parity/business_ids_rust/PROVENANCE.json').read_text())
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=provenance,
                  resource_message_mapping=RESOURCE_MESSAGES,
                  note='Original producer-generated wrapper construction/Debug/definition and complete typed Checker diagnostics in three naming profiles; exact values and availability. Native audited SQLite save supplies separate integration/evidence/lifetime coverage.',
                  cases=[dict(input=case, source_expected=source, expected=value)
                         for case, source, value in zip(inputs, expected, adapted)])
    data = (json.dumps(corpus, indent=2) + '\n').encode()
    Path(__file__).with_name('business_ids_5_0_6_cases.json.gz').write_bytes(
        gzip.compress(data, mtime=0))
    print(f'Compared {len(inputs)} actual generated Rust Business-ID inputs')


if __name__ == '__main__':
    main()
