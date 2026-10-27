#!/usr/bin/env python3
"""Validate complete generated Save observations and strict negative controls."""
import argparse
from contextlib import closing
import copy
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import sqlite3
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same

NAMES = {'control', 'entity', 'table', 'data_service', 'combined'}
FIELDS = {'outcome', 'calls', 'plans', 'events', 'logs', 'database', 'fixes'}
LOG_FIELDS = {'log_context', 'operation', 'comment', 'purpose', 'audit_reason',
              'trace_path', 'sql', 'params', 'params_debug', 'debug_sql',
              'pretty_sql', 'result_count', 'result_type', 'affected_rows', 'result_summary'}
INPUTS = [dict(operation=op, policy=policy, failure=failure, logging=logging)
          for op, policy, failure, logging in itertools.product(
              ['create', 'create_child', 'update'], ['allow', 'deny', 'none'],
              ['none', 'id', 'provider', 'audit'], [True, False])]


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def initial_database():
    source = (ROOT / 'tests/parity/generated_identity_save_probe.rs').read_text()
    sql = re.search(r'const SCHEMA: &str = r#"(.*?)"#;', source, re.S).group(1)
    with closing(sqlite3.connect(':memory:')) as connection:
        connection.row_factory = sqlite3.Row
        connection.executescript(sql)
        return {name: [dict(row) for row in connection.execute(f'SELECT * FROM {name} ORDER BY id')]
                for name in ['platform_data', 'business_items', 'other_table', 'business_children']}


def validate(record):
    assert record['complete']
    assert same(record['inputs'], INPUTS), 'Incomplete or changed input matrix'
    assert len(record['cases']) == len(NAMES)
    assert {case['name'] for case in record['cases']} == NAMES
    baseline = initial_database()
    total = 0
    coverage = dict(successes=0, plans=0, events=0, logs=0, committed_audit_errors=0)
    for case in record['cases']:
        assert not case['differences'], 'Receipt reports unresolved differences'
        assert case['registry_identity_matches'] is True
        assert len(case['original']) == len(case['cpp']) == len(INPUTS)
        for input_value, source, native in zip(INPUTS, case['original'], case['cpp']):
            assert same(source, native), (case['name'], input_value, 'Source/native difference')
            for result in [source, native]:
                assert set(result) == FIELDS, 'Dropped or added result responsibility'
                assert type(result['outcome']['ok']) is bool
                assert type(result['fixes']) is str
                assert all(type(result[key]) is list for key in ['plans', 'events', 'logs', 'calls'])
                assert all(set(log) == LOG_FIELDS for log in result['logs'])
                publication = json.dumps({'events': result['events'], 'logs': result['logs']})
                for canary in ['PRIVATE-NEW-SENTENCE', 'PRIVATE-OLD-SENTENCE', 'CREDENTIAL-SENTINEL']:
                    assert canary not in publication, ('Unsafe publication', canary)
                if not input_value['logging']:
                    assert not result['logs'], 'SQL logging disabled must still execute governance'
                if input_value['policy'] == 'deny' or input_value['failure'] == 'provider':
                    assert same(result['database'], baseline), 'Denied/failed writes changed storage'
                    assert not result['events'], 'No committed audit after rejection/rollback'
                if input_value['policy'] == 'none':
                    assert not result['plans'], 'Absent customer registry cannot run the fixture policy'
                if input_value['operation'] == 'update':
                    assert not result['calls'], 'Authoritatively loaded updates do not allocate an ID'
                else:
                    assert result['calls'] and result['calls'][0] == (
                        'child' if input_value['operation'] == 'create_child' else 'item')
                if result['outcome']['ok']:
                    assert type(result['outcome']['entity']['id']) is int
                    assert result['outcome']['entity']['id'] > 0
                    assert not same(result['database'], baseline)
                    assert result['events'], 'Successful changed Save must reach the configured sink'
                if input_value['failure'] == 'audit' and result['events']:
                    assert not result['outcome']['ok']
                    assert not same(result['database'], baseline), 'Delivery failure cannot undo commit'
            coverage['successes'] += int(source['outcome']['ok'])
            for key in ['plans', 'events', 'logs']:
                coverage[key] += len(source[key])
            coverage['committed_audit_errors'] += int(input_value['failure'] == 'audit' and bool(source['events']))
            total += 1
    assert total == 360 and all(coverage.values()), 'Empty semantic coverage'
    return dict(observations=total, **coverage)


def verify_provenance(record):
    archive_path = ROOT / 'docs/parity/container_metadata_compiled_final_5_0_6.json.gz'
    assert record['archive_sha256'] == digest(archive_path)
    archive = {case['case']: case for case in read(archive_path)['cases']}
    if record.get('reused_source'):
        pointer = record['reused_source']
        reference_path = ROOT / pointer['path']
        assert digest(reference_path) == pointer['sha256']
        reference = read(reference_path)
        assert reference['complete'] and reference['rust_main'] == record['rust_main']
        assert same(reference['inputs'], record['inputs'])
        for parent in reference.get('parents', []):
            assert digest(ROOT / parent['path']) == parent['sha256']
    else:
        reference = record
    assert record.get('input_hashes_before') == record.get('input_hashes_after')
    for case in record['cases']:
        original = archive[case['name']]
        assert original['model'] == case['model']
        assert case['original_sdk_hashes'] == {path: value['sha256']
            for path, value in original['files'].items() if path.startswith('original/lib/')}
        expected = next(row for row in reference['cases'] if row['name'] == case['name'])
        assert same(case['original'], expected['original'])
        for label in ['rust_build', 'rust_run']:
            steps = [step for step in reference['steps'] if step['label'] == case['name'] + '/' + label]
            assert len(steps) == 1 and steps[0]['exit_code'] == 0
            if label == 'rust_run':
                assert same(json_lines(steps[0]['stdout']), case['original'])
        for label in ['cpp_compile', 'cpp_run']:
            steps = [step for step in record['steps'] if step['label'] == case['name'] + '/' + label]
            assert len(steps) == 1 and steps[0]['exit_code'] == 0
            if label == 'cpp_run':
                assert same(json_lines(steps[0]['stdout']), case['cpp'])


def controls(record):
    output = []

    def reject(name, mutate):
        changed = copy.deepcopy(record)
        mutate(changed)
        try:
            validate(changed)
        except (AssertionError, KeyError, TypeError, IndexError):
            output.append(name)
        else:
            raise AssertionError('Accepted effective mutation: ' + name)

    reject('missing_model', lambda d: d['cases'].pop())
    reject('missing_input_result', lambda d: d['cases'][0]['cpp'].pop())
    reject('duplicate_input_result', lambda d: d['cases'][0]['cpp'].append(copy.deepcopy(d['cases'][0]['cpp'][0])))
    control = next(index for index, case in enumerate(record['cases']) if case['name'] == 'control')
    def first(d):
        return d['cases'][control]['cpp'][0]
    reject('bool_to_integer', lambda d: first(d)['outcome'].__setitem__('ok', 1))
    reject('integer_to_float', lambda d: first(d)['outcome']['entity'].__setitem__('id', float(first(d)['outcome']['entity']['id'])))
    reject('missing_policy_plan', lambda d: first(d)['plans'].pop())
    reject('missing_safe_audit', lambda d: first(d)['events'].pop())
    reject('missing_sql_record', lambda d: first(d)['logs'].pop())
    reject('wrong_fix_identity', lambda d: first(d).__setitem__('fixes', first(d)['fixes'].replace('entity_type: "Item"', 'entity_type: "Other"')))
    def remove_both_fixes(d):
        for side in ['original', 'cpp']:
            d['cases'][control][side][0].pop('fixes')
    reject('unobserved_fix_field_on_both_sides', remove_both_fixes)
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('receipt', type=Path)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists()
    record = read(args.receipt)
    verify_provenance(record)
    coverage = validate(record)
    rejected = controls(record)
    result = dict(complete=True, scope=__doc__, receipt=str(args.receipt),
                  receipt_sha256=digest(args.receipt), coverage=coverage, rejected_mutations=rejected)
    args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
    print('Qualified', coverage, '; effective mutations rejected:', len(rejected))


if __name__ == '__main__':
    main()
