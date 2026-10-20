#!/usr/bin/env python3
"""Check business outcomes and failure boundaries in the complete keyword graph corpus."""
import gzip
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_relation_graph_save_5_0_6 import identity, inputs


def main():
    path = ROOT / 'tests/parity/relation_graph_save_5_0_6_cases.json.gz'
    corpus = json.loads(gzip.decompress(path.read_bytes()))
    for key, value in identity().items():
        assert corpus[key] == value
    cases = corpus['cases']
    assert [row['input'] for row in cases] == inputs() and len(cases) == 608
    def result(**changes):
        wanted = dict(target='owner', minimal=False, selection='both', op='mixed',
                      composition='include', fault='none', policy='none', audit_fail=False)
        wanted.update(changes)
        return next(row['expected'] for row in cases if row['input'] == wanted)
    def row(output, table, identity):
        return next(value for value in output['db_after'][table] if value['id'] == identity)
    mixed = result()
    assert mixed['error'] is None
    assert row(mixed, 'entry_data', 20)['name'] == 'Updated class member'
    assert row(mixed, 'entry_data', 20)['version'] == 2
    assert row(mixed, 'entry_data', 21)['version'] == -2
    assert row(mixed, 'entry_data', 1001)['name'] == 'New class member'
    assert row(mixed, 'profile_data', 30)['name'] == 'Updated profile'
    assert len(mixed['events']) == 7
    # Named source relation selection includes this deleted child. Its actual
    # signed-version update is retained, rather than repaired in the comparator.
    assert row(mixed, 'entry_data', 22)['version'] == 0
    independent = result(composition='none')
    assert independent['db_before']['entry_data'] == independent['db_after']['entry_data']
    assert independent['db_before']['profile_data'] == independent['db_after']['profile_data']
    assert len(independent['events']) == 1
    assert result(composition='attach')['db_after'] == mixed['db_after']
    for fault in ['root_conflict', 'second_trigger', 'insert_trigger']:
        failed = result(fault=fault)
        assert failed['error'] is not None and failed['db_before'] == failed['db_after']
        assert not failed['events'], fault
    rejected = result(policy='deny')
    assert rejected['error'].startswith('request policy error: [MUTATION POLICY DENIED]')
    assert rejected['db_before'] == rejected['db_after'] and not rejected['events']
    assert rejected['plans'] and all(log['operation'] == 'Select' for log in rejected['logs'])
    committed = result(audit_fail=True)
    assert committed['error'].startswith('AUDIT_DELIVERY_AFTER_COMMIT:')
    assert committed['db_after'] == mixed['db_after'] and committed['events']
    sparse = result(minimal=True, op='root')
    assert sparse['error'].startswith('check failed:')
    assert sparse['db_before'] == sparse['db_after'] and not sparse['events']
    forward = result(target='entry')
    assert forward['error'] is None
    assert row(forward, 'entry_data', 20)['namespace'] == 1001
    assert row(forward, 'owner_data', 1001)['name'] == 'New namespace owner'
    print('Verified 608 complete source outcomes and mixed-save/composition/rollback/policy/audit/load-state business boundaries')


if __name__ == '__main__':
    main()
