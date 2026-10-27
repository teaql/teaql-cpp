#!/usr/bin/env python3
"""Check complete canonical E-root observations and effective corruptions."""
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
from verify_generated_identity_expression_5_0_6 import adapted
from compare_values import json_lines, same

NAMES = {'control', 'entity', 'table', 'data_service', 'combined'}
INPUTS = [dict(zip(['entity', 'state', 'path', 'action'], values)) for values in itertools.product(
    ['item', 'child'], ['new_empty', 'new_name', 'new_note_null', 'json_empty', 'json_null',
                        'json_full', 'json_relation_missing_name', 'json_relation_named'],
    ['id', 'name', 'note', 'version', 'platform_id', 'platform_name'], ['eval', 'unwrap'])]
DIAGNOSTIC = {'original_expr_with_broken_point', 'root', 'access_path', 'break_point',
              'missing_preload', 'suggested_fix', 'nested_fix', 'severity', 'human_message'}


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate(record):
    assert record['complete'] is True
    assert same(record['inputs'], INPUTS)
    assert len(record['cases']) == len(NAMES) and {c['name'] for c in record['cases']} == NAMES
    coverage = dict(not_loaded=0, value=0, null=0, prepare_error=0)
    for case in record['cases']:
        assert not case['differences'] and case['registry_identity_matches'] is True
        assert len(case['original']) == len(case['cpp']) == len(INPUTS)
        for input_value, source, native in zip(INPUTS, case['original'], case['cpp']):
            assert same(adapted(source), native), (case['name'], input_value)
            for value in [source, native]:
                assert type(value['prepared']) is bool and type(value['sql_count']) is int
                assert value['sql_count'] == 0, 'E must not perform implicit database reads'
                if not value['prepared']:
                    assert set(value) == {'prepared', 'error', 'debug', 'sql_count'}
                    assert type(value['error']) is str and type(value['debug']) is str
                    assert input_value['state'] == 'json_null', 'Unexpected loss of loaded-state coverage'
                    assert 'JSON_ENTITY_INPUT: invalid field type: platform_id' in value['error']
                    continue
                assert set(value) == {'prepared', 'identity', 'id', 'read', 'sql_count'}
                assert type(value['identity']) is str and type(value['id']) is int
                canonical = 'Item' if input_value['entity'] == 'item' else 'Child'
                overridden = case['name'] in {'entity', 'combined'} and canonical == 'Item'
                assert value['identity'] == ('Other' if overridden else canonical)
                result = value['read']
                if result['kind'] == 'not_loaded':
                    assert set(result) == {'kind', 'diagnostic'}
                    diagnostic = result['diagnostic']
                    assert set(diagnostic) == DIAGNOSTIC
                    assert diagnostic['root'] == f"{canonical}(id={value['id']})"
                    assert diagnostic['severity'] == 'error'
                    assert all(type(diagnostic[key]) is list and diagnostic[key] and
                               all(type(part) is str for part in diagnostic[key])
                               for key in ['access_path', 'missing_preload'])
                    if value is source:
                        # Source human text has unescaped internal selection quotes.
                        # Validate it before the explicit English presentation adapter.
                        message = ('访问 ' + canonical + '.' + '.'.join(diagnostic['access_path'])
                                   + ' 时缺少预加载。请在查询中加入 "' + diagnostic['suggested_fix'] + '"')
                        if diagnostic['nested_fix'] is not None:
                            message += ' 或完整嵌套加载 "' + diagnostic['nested_fix'] + '"'
                        assert diagnostic['human_message'] == message
                elif result['kind'] == 'value':
                    assert set(result) == {'kind', 'value'} and type(result['value']) is str
                else:
                    assert result['kind'] == 'null' and set(result) == {'kind', 'message'}
                    assert input_value['action'] == 'unwrap'
            coverage[source['read']['kind'] if source['prepared'] else 'prepare_error'] += 1
    assert coverage == dict(not_loaded=500, value=330, null=10, prepare_error=120)
    return dict(observations=960, **coverage)


def provenance(record):
    assert record['input_hashes_before'] == record['input_hashes_after']
    archive_path = ROOT / 'docs/parity/container_metadata_compiled_final_5_0_6.json.gz'
    assert record['archive_sha256'] == digest(archive_path)
    archive = {c['case']: c for c in read(archive_path)['cases']}
    pointer = record.get('reused_source')
    reference = read(ROOT / pointer['path']) if pointer else record
    if pointer:
        assert digest(ROOT / pointer['path']) == pointer['sha256']
    assert reference['complete'] is True and reference['rust_main'] == record['rust_main']
    assert record['rust_main'] == (ROOT / 'tests/parity/generated_identity_expression_probe.rs').read_text()
    assert record['cpp_main'] == (ROOT / 'tests/parity/generated_identity_expression_probe.cpp').read_text()
    assert same(reference['inputs'], INPUTS)
    for parent in reference.get('parents', []):
        assert digest(ROOT / parent['path']) == parent['sha256']
    for case in record['cases']:
        original = archive[case['name']]
        assert case['model'] == original['model']
        assert case['original_sdk_hashes'] == {path: value['sha256'] for path, value in original['files'].items()
                                                if path.startswith('original/lib/')}
        expected = next(c for c in reference['cases'] if c['name'] == case['name'])
        assert same(expected['original'], case['original'])
        for label in ['rust_build', 'rust_run']:
            steps = [step for step in reference['steps'] if step['label'] == case['name'] + '/' + label]
            assert len(steps) == 1 and steps[0]['exit_code'] == 0
            if label == 'rust_run':
                assert same(json_lines(steps[0]['stdout']), case['original'])
        for label in ['cpp_generate', 'cpp_compile', 'cpp_run']:
            steps = [step for step in record['steps'] if step['label'] == case['name'] + '/' + label]
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
    def diagnostic(value, language='cpp'):
        return next(result['read']['diagnostic'] for case in value['cases'] for result in case[language]
                    if result['prepared'] and result['read']['kind'] == 'not_loaded')
    reject('missing_sdk', lambda d: d['cases'].pop())
    reject('missing_result', lambda d: d['cases'][0]['cpp'].pop())
    reject('boolean_as_sql_count', lambda d: d['cases'][0]['cpp'][0].update(sql_count=False))
    reject('implicit_sql', lambda d: d['cases'][0]['cpp'][0].update(sql_count=1))
    reject('wrong_canonical_root', lambda d: diagnostic(d).update(root='Other(id=1001)'))
    reject('dropped_diagnostic_field', lambda d: diagnostic(d).pop('missing_preload'))
    reject('wrong_native_entry', lambda d: diagnostic(d).update(original_expr_with_broken_point='E::other(id=1001)'))
    reject('changed_source_human_text', lambda d: diagnostic(d, 'original').update(human_message='unverified text'))
    reject('extra_result_responsibility', lambda d: d['cases'][0]['cpp'][0].update(extra=0))
    reject('wrong_null_message', lambda d: next(result['read'] for case in d['cases'] for result in case['cpp']
                                               if result['prepared'] and result['read']['kind'] == 'null').update(message='NULL'))
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
    args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
    print(coverage, '; rejected controls:', len(negative))


if __name__ == '__main__':
    main()
