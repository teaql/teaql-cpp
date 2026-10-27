#!/usr/bin/env python3
"""Check complete forward-owner reads, distinct JSON identity and corruptions."""
import argparse
import copy
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_generated_relation_identity_5_0_6 import adapted
from compare_values import json_lines, same

NAMES = {'control', 'entity', 'table', 'data_service', 'combined'}
BASE_INPUTS = [dict(zip(['entity', 'selected', 'foreign_id', 'cached', 'owner', 'view', 'embedded'], values))
    for values in itertools.product(['item', 'child'], [False, True], ['0', '100'], [False, True],
                                    ['none', 'raw', 'canonical', 'trait', 'both'],
                                    ['present', 'empty', 'wrong_type'], [False, True])]
INPUTS = BASE_INPUTS + [dict(v, construction='row') for v in BASE_INPUTS]
FIELDS = {'raw', 'identity', 'getter', 'eval', 'expression', 'json', 'kernel', 'sql_count'}
DIAGNOSTIC = {'original_expr_with_broken_point', 'root', 'access_path', 'break_point',
              'missing_preload', 'suggested_fix', 'nested_fix', 'severity', 'human_message'}


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate(record):
    assert record['complete'] is True and same(record['inputs'], INPUTS)
    assert len(record['cases']) == 5 and {c['name'] for c in record['cases']} == NAMES
    coverage = dict(values=0, not_loaded=0, embedded_values=0, distinct_getter_json=0, ordinary_rows=0)
    for case in record['cases']:
        assert not case['differences'] and case['registry_identity_matches'] is True
        assert len(case['original']) == len(case['cpp']) == len(INPUTS)
        for input_value, source, native in zip(INPUTS, case['original'], case['cpp']):
            assert same(adapted(source), native), (case['name'], input_value)
            for result in [source, native]:
                assert set(result) == FIELDS
                assert type(result['sql_count']) is int and result['sql_count'] == 0
                assert result['raw'] == input_value['entity']
                canonical = 'Item' if input_value['entity'] == 'item' else 'Child'
                override = case['name'] in {'entity', 'combined'} and canonical == 'Item'
                assert result['identity'] == ('Other' if override else canonical)
                assert result['getter'] is None or type(result['getter']) is int
                if result['getter'] is None:
                    assert result['eval'] == dict(state='NotLoaded', node='platform', path='platform')
                    assert set(result['expression']) == {'kind', 'diagnostic'}
                    assert result['expression']['kind'] == 'not_loaded'
                    diagnostic = result['expression']['diagnostic']
                    assert set(diagnostic) == DIAGNOSTIC
                    assert diagnostic['root'] == canonical + '(id=101)'
                    assert diagnostic['access_path'] == diagnostic['missing_preload'] == ['platform']
                    assert diagnostic['break_point'] == 'platform' and diagnostic['severity'] == 'error'
                    assert diagnostic['nested_fix'] is None
                    if result is source:
                        assert diagnostic['human_message'] == '访问 ' + canonical + '.platform 时缺少预加载。请在查询中加入 "select_platform()"'
                else:
                    getter = result['getter']
                    labels = {77: 'EMBEDDED', 100: 'CACHE', 200: 'MODEL', 300: 'TRAIT'}
                    assert getter in labels
                    assert same(result['eval'], dict(state='Value', id=getter))
                    assert result['expression'] == dict(kind='value', value='Some("' + labels[getter] + '")')
                assert set(result['kernel']) == {'raw', 'canonical', 'trait'}
                assert same(result['kernel']['raw'], result['kernel']['canonical'])
                if not override:
                    assert same(result['kernel']['raw'], result['kernel']['trait'])
                for edge in result['kernel'].values():
                    assert edge['state'] in {'absent', 'empty', 'present'}
                    assert set(edge) == ({'state', 'id'} if edge['state'] == 'present' else {'state'})
                    if edge['state'] == 'present':
                        assert type(edge['id']) is int and edge['id'] in {200, 300}
                assert type(result['json']) is dict
                assert result['json']['id'] == 101 and result['json']['version'] == 1
                assert result['json']['name'] == 'OWNER'
                if input_value.get('construction') == 'row' and input_value['embedded']:
                    assert result['getter'] == 77, 'Compact-row local object must precede named graph availability'
            coverage['not_loaded' if source['getter'] is None else 'values'] += 1
            coverage['embedded_values'] += int(source['getter'] == 77)
            coverage['ordinary_rows'] += int(input_value.get('construction') == 'row')
            coverage['distinct_getter_json'] += int(source['getter'] is not None and 'platform' in source['json']
                                                    and source['getter'] != source['json']['platform']['id'])
    assert coverage['values'] + coverage['not_loaded'] == 4800
    assert coverage['ordinary_rows'] == 2400 and coverage['embedded_values'] == 1200
    assert coverage['distinct_getter_json'] > 0, 'Do not merge getter and JSON owner responsibilities'
    return dict(observations=4800, **coverage)


def provenance(record):
    assert record['input_hashes_before'] == record['input_hashes_after']
    archive_path = ROOT / 'docs/parity/container_metadata_compiled_final_5_0_6.json.gz'
    assert record['archive_sha256'] == digest(archive_path)
    archive = {c['case']: c for c in read(archive_path)['cases']}
    reference = record
    if record.get('reused_source'):
        pointer = record['reused_source']
        assert digest(ROOT / pointer['path']) == pointer['sha256']
        reference = read(ROOT / pointer['path'])
    assert reference['complete'] is True and same(reference['inputs'], INPUTS)
    assert reference['rust_main'] == record['rust_main'] == (ROOT / 'tests/parity/generated_relation_identity_probe.rs').read_text()
    assert record['cpp_main'] == (ROOT / 'tests/parity/generated_relation_identity_probe.cpp').read_text()
    prefix = read(ROOT / 'docs/parity/generated_relation_identity_full_before_5_0_6.json.gz')
    assert prefix['complete'] is True and same(prefix['inputs'], BASE_INPUTS)
    for case in record['cases']:
        expected = archive[case['name']]
        assert case['model'] == expected['model']
        assert case['original_sdk_hashes'] == {p: v['sha256'] for p, v in expected['files'].items() if p.startswith('original/lib/')}
        reused = next(c for c in reference['cases'] if c['name'] == case['name'])
        assert same(reused['original'], case['original'])
        original_prefix = next(c for c in prefix['cases'] if c['name'] == case['name'])
        assert same(original_prefix['original'], case['original'][:len(BASE_INPUTS)]), 'Original JSON constructor prefix changed'
        for label in ['rust_build', 'rust_run']:
            steps = [s for s in reference['steps'] if s['label'] == case['name'] + '/' + label]
            assert len(steps) == 1 and steps[0]['exit_code'] == 0
            if label == 'rust_run':
                assert same(json_lines(steps[0]['stdout']), case['original'])
        for label in ['cpp_generate', 'cpp_compile', 'cpp_run']:
            steps = [s for s in record['steps'] if s['label'] == case['name'] + '/' + label]
            assert len(steps) == 1 and steps[0]['exit_code'] == 0
            if label == 'cpp_run':
                assert same(json_lines(steps[0]['stdout']), case['cpp'])


def controls(record):
    rejected = []
    def reject(name, mutate):
        changed = copy.deepcopy(record)
        mutate(changed)
        try:
            validate(changed)
        except (AssertionError, KeyError, TypeError, IndexError):
            rejected.append(name)
        else:
            raise AssertionError('Accepted effective corruption: ' + name)
    def diagnostic(d, language='cpp'):
        return next(r['expression']['diagnostic'] for c in d['cases'] for r in c[language]
                    if r['expression']['kind'] == 'not_loaded')
    def distinct(d):
        return next(r for c in d['cases'] for r in c['cpp'] if r['getter'] is not None and
                    'platform' in r['json'] and r['getter'] != r['json']['platform']['id'])
    reject('missing_sdk', lambda d: d['cases'].pop())
    reject('missing_read', lambda d: d['cases'][0]['cpp'].pop())
    reject('sql_boolean', lambda d: d['cases'][0]['cpp'][0].update(sql_count=False))
    reject('implicit_sql', lambda d: d['cases'][0]['cpp'][0].update(sql_count=1))
    reject('wrong_getter_owner', lambda d: distinct(d).update(getter=300))
    reject('collapse_json_into_getter', lambda d: distinct(d)['json']['platform'].update(id=200))
    reject('drop_eval_path', lambda d: next(r['eval'] for c in d['cases'] for r in c['cpp'] if r['getter'] is None).pop('path'))
    reject('wrong_diagnostic_root', lambda d: diagnostic(d).update(root='Other(id=101)'))
    reject('drop_diagnostic_field', lambda d: diagnostic(d).pop('missing_preload'))
    reject('wrong_source_human_message', lambda d: diagnostic(d, 'original').update(human_message='unverified'))
    reject('added_result_field', lambda d: d['cases'][0]['cpp'][0].update(extra=None))
    reject('collapse_kernel_owner', lambda d: next(r['kernel'] for c in d['cases'] for r in c['cpp']
             if r['kernel']['raw'] != r['kernel']['trait']).update(trait={'state':'empty'}))
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('receipt', type=Path)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve earlier receipts'
    value = read(args.receipt)
    provenance(value)
    coverage = validate(value)
    negative = controls(value)
    result = dict(complete=True, receipt=str(args.receipt), receipt_sha256=digest(args.receipt),
                  coverage=coverage, rejected_controls=negative, verifier_sha256=digest(Path(__file__).resolve()))
    args.record.write_bytes(gzip.compress(json.dumps(result).encode(),mtime=0))
    print(coverage, '; rejected controls:', len(negative))


if __name__ == '__main__':
    main()
