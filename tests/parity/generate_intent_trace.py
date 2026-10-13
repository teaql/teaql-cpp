#!/usr/bin/env python3
"""Compare original 5.0.6 intent models and public trace-token recovery."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import subprocess

from generate_field_layout import observe

ROOT = Path(__file__).resolve().parents[2]


def cases():
    texts = [None, '', ' ', '\t\n', '\u0085\u00a0\u1680\u2003\u2028\u202f\u205f\u3000',
             ' what ', '雪', '\u200b', 'long ' + 'x' * 4096]
    out = [dict(kind='request_intent', comment=comment, purpose=purpose,
                extra_query=query, extra_field=field)
           for comment, purpose, query, field in itertools.product(texts, texts, [False, True],
                                                                   ['comment', 'purpose', 'unexpected'])]
    for root, child, leaf in itertools.product(texts[1:8], texts[1:8], ['', ' leaf ', '\u3000']):
        out.append(dict(kind='trace_tokens', nodes=[
            dict(parent=None, entity='Order', id=None, comment=root),
            dict(parent=0, entity='Item', id=7, comment=child),
            dict(parent=1, entity='Payment', id=8, comment=leaf),
            dict(parent=0, entity='Sibling', id=9, comment='sibling')]))
    routes = [[],
        [dict(kind=2, entity='items', id=None, comment='Root.items')],
        [dict(kind=6, entity='Ancestor', id=42, comment='stale comment'),
         dict(kind=7, entity='Other', id=None, comment='stale purpose'),
         dict(kind=2, entity='items', id=None, comment='Root.items')],
        [dict(kind=3, entity='Ignored', id=7, comment='entity'),
         dict(kind=0, entity='\u3000', id=None, comment='blank root'),
         dict(kind=8, entity=' Real root ', id=99, comment='audit'),
         dict(kind=5, entity='SQLite', id=None, comment='sql')]]
    for explicit, comment, purpose, metadata, debug in itertools.product(
            [False, True], texts[:8], texts[:8], [False, True], [False, True]):
        for route in routes:
            out.append(dict(kind='request_envelope', entity='Item', explicit=explicit,
                comment=None if explicit else comment, purpose='query-local purpose' if explicit else purpose,
                intent_comment=comment, intent_purpose=purpose, metadata=metadata, debug=debug,
                trace=route))
    ids = [dict(kind='null'), dict(kind='i64', value='-1'), dict(kind='i64', value='0'),
           dict(kind='i64', value='7'), dict(kind='u64', value='18446744073709551615'),
           dict(kind='text', value='7'), dict(kind='text', value='not an id')]
    mutation_routes = [[], [dict(kind=3, entity='Item', id=7, comment='diagnostic')],
        [dict(kind=2, entity='items', id=None, comment='Root.items'),
         dict(kind=8, entity='Ancestor', id=9, comment='stale root'),
         dict(kind=8, entity='Item', id=7, comment='local child'),
         dict(kind=6, entity='Untrusted', id=None, comment='irrelevant comment')]]
    for command, comment, identity, route, via_command in itertools.product(
            ['insert', 'update', 'delete', 'recover', 'batch'], texts, ids, mutation_routes, [False, True]):
        out.append(dict(kind='mutation_envelope', command=command, entity='Item', comment=comment,
                        id=identity, trace=route, via_command=via_command))
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    inputs = cases()
    expected = observe(args.rust.resolve(), inputs)
    actual = observe(args.cpp.resolve(), inputs)
    for index, (left, right) in enumerate(zip(expected, actual)):
        if left != right:
            raise SystemExit(f'Intent/trace mismatch {index}: {inputs[index]}\n{left}\n{right}')
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    sources = ['teaql-core/src/request_intent.rs', 'teaql-runtime/src/error.rs', 'teaql-runtime/src/graph.rs', 'teaql-data-service/src/lib.rs']
    corpus = dict(baseline=lock['commit'], version=lock['version'],
                  source_sha256={source: hashlib.sha256((ROOT / '.reference/teaql-rs' / source).read_bytes()).hexdigest()
                                 for source in sources},
                  note='Direct original public intent/token/request-envelope calls. Private scope capture/enrichment are separately evidenced by source regression and native lifecycle/integration contracts. No output normalization.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('intent_trace_5_0_6_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Compared {len(inputs)} actual original intent/trace inputs')


if __name__ == '__main__':
    main()
