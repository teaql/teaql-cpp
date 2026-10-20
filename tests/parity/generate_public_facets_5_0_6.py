#!/usr/bin/env python3
"""Observe original public facet execution with explicit 5.0.6 intent inputs."""
import argparse
import copy
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import subprocess

from compare_values import observe, same
from generate_generated_support import case, descriptors, facet, query

ROOT = Path(__file__).resolve().parents[2]
BASELINE = 'fe82a18982415f1746df9a2cf7c42f5e9abdc6d3'
CORPUS = Path(__file__).with_name('public_facets_5_0_6_cases.json.gz')


def inputs():
    historical = json.loads(Path(__file__).with_name('generated_support_cases.json').read_text())
    values = [copy.deepcopy(item['input']) for item in historical['cases']]
    for item in historical['cases']:
        if item['input']['op'] in ('execute', 'sqlite'):
            value = copy.deepcopy(item['input'])
            value['outer']['comment'] = 'Load the selected facet graph'
            value['outer']['purpose'] = 'Review the facet result'
            values.append(value)
    for comment, purpose, populated, modeled in itertools.product(
            (None, '', ' \t', '\u2003', 'business what'),
            (None, '', '\n', '\u00a0', 'business why'), (False, True), (False, True)):
        outer = query()
        if comment is not None:
            outer['comment'] = comment
        if purpose is not None:
            outer['purpose'] = purpose
        value = case(outer=outer, options=dict(facets=[facet(all=True)] if populated else []))
        value['descriptors'] = [] if not modeled else descriptors()
        values.append(value)
    for depth, siblings, all_rows, pending, failed, cancel in itertools.product(
            (1, 2, 3), (1, 2), (False, True), (None, 0, 1, 2),
            (None, 0, 1, 2), (False, True)):
        parent = descriptors()[0]
        child = copy.deepcopy(parent)
        child['name'] = 'Child'
        child['relations'][0]['target'] = 'Grandchild'
        grandchild = copy.deepcopy(child)
        grandchild['name'] = 'Grandchild'
        grandchild['relations'][0]['target'] = 'Child'
        def nested(level):
            entry = facet('same', all=all_rows)
            entry['selection']['query']['entity'] = 'Child' if level % 2 == 0 else 'Grandchild'
            if level + 1 < depth:
                entry['selection']['options']['facets'] = [nested(level + 1)]
            return entry
        outer = query(rich=True)
        outer['comment'] = 'Load nested facet members'
        outer['purpose'] = 'Review nested facet composition'
        value = case(outer=outer, options=dict(facets=[nested(0) for _ in range(siblings)]))
        value['descriptors'] = [parent, child, grandchild]
        value['cancel'] = cancel
        if pending is not None:
            value['pending_at'] = pending
        if failed is not None:
            value['fail_at'] = failed
        values.append(value)
    unique = {}
    for value in values:
        unique.setdefault(json.dumps(value, sort_keys=True), value)
    return list(unique.values())


def log_inputs():
    values = []
    for logging, all_rows, shape, secret in itertools.product(
            (False, True), (False, True), ('direct', 'nested', 'aggregate'),
            ('FUTURE_SECRET_ALPHA', 'future_\u96ea_\U0001f9ca', 'X' * 96)):
        parent = descriptors()[0]
        parent['properties'] = [dict(name='id', type=2, id=True), dict(name='label', type=5)]
        parent['masked'] = ['label']
        child = dict(name='Child', table='children', masked=['label'],
                     properties=[dict(name='id', type=2, id=True),
                                 dict(name='parent_id', type=2), dict(name='label', type=5)],
                     relations=[dict(name='parent', target='Parent', local='parent_id', foreign='id')])
        base = facet('base', all=all_rows)
        base['selection']['query'] = dict(entity='Child', projection=['id', 'parent_id', 'label'])
        future = copy.deepcopy(base)
        future['name'] = 'future'
        condition = dict(kind='binary', left=dict(kind='column', name='label'), op=0,
                         right=dict(kind='value', value=dict(kind='text', value=secret)))
        private = dict(query=dict(entity='Parent', filter=condition), options={})
        if shape == 'direct':
            future['selection']['query']['filter'] = condition
        elif shape == 'nested':
            future['selection']['options']['facets'] = [dict(name='nested', relation='parent',
                                                           selection=private, all=True)]
        else:
            private['query']['aggregates'] = [dict(function=0, field='id', alias='count')]
            future['selection']['options']['aggregates'] = [dict(relation='parent', alias='future_count',
                                                               selection=private, single=True)]
        value = case(op='sqlite', outer=dict(entity='Parent',
                     comment=f'Load facet members {secret}', purpose=f'Review facet members {secret}'),
                     options=dict(facets=[base, future]))
        value.update(descriptors=[parent, child], logging=logging, capture_logs=True, secret=secret)
        values.append(value)
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--cpp')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--logs', action='store_true', help='Observe actual SQLite privacy and same-Context follow-up')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    assert json.loads((ROOT / 'upstream.lock.json').read_text())['commit'] == BASELINE
    values = log_inputs() if args.logs else inputs()
    corpus_path = CORPUS.with_name('public_facet_logs_5_0_6_cases.json.gz') if args.logs else CORPUS
    original = observe(args.rust, values)
    if args.logs:
        for value, result in zip(values, original):
            assert 'error' not in result and result['followup_error'] is None
            if value['logging']:
                assert result['logs'] and value['secret'] not in json.dumps(result['logs'], ensure_ascii=False)
                assert value['secret'] in json.dumps(result['followup_logs'], ensure_ascii=False)
            else:
                assert not result['logs'] and not result['followup_logs']
    corpus = dict(baseline=BASELINE, note='Actual unchanged public runtime algorithms; observer adds intent input/error fields only.',
                  cases=[dict(input=value, expected=result) for value, result in zip(values, original)])
    if args.check:
        with gzip.open(corpus_path, 'rt') as source:
            retained = json.load(source)
        if not same(retained, corpus):
            raise SystemExit('Public facet corpus differs from fresh original execution')
    elif not corpus_path.exists():
        corpus_path.write_bytes(gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n').encode(), mtime=0))
    else:
        raise SystemExit('Use --check to preserve the existing original corpus')
    print(f'Original public facets: {len(values)} observations reproduce', flush=True)
    if args.cpp:
        native = observe(args.cpp, values)
        failed = [i for i, (a, b) in enumerate(zip(original, native)) if not same(a, b)]
        print(f'Public facets: {len(values) - len(failed)}/{len(values)} exact original/native matches')
        if failed:
            evidence = dict(baseline=BASELINE, total=len(values), differences=len(failed),
                            failed_indices=failed, original_corpus_sha256=hashlib.sha256(corpus_path.read_bytes()).hexdigest(),
                            examples=[dict(index=i, input=values[i], original=original[i], cpp=native[i]) for i in failed[:12]])
            path = ROOT / 'docs/parity' / ('public_facet_logs_5_0_6_initial_gaps.json' if args.logs
                                          else 'public_facets_5_0_6_initial_gaps.json')
            if not path.exists():
                path.write_text(json.dumps(evidence, indent=2) + '\n')
            raise SystemExit(1)


if __name__ == '__main__':
    main()
