#!/usr/bin/env python3
"""Reject legacy locks and corruptions for a registered declared-driver cohort."""
import argparse
import copy
import gzip
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_declared_driver_contracts_5_0_6 import check_dependency_lock, digest, read, validate
from compare_values import json_lines


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path, required=True)
    args = parser.parse_args()
    assert args.record is None or not args.record.exists()
    execution_path, registry_path = args.execution.resolve(), args.contracts.resolve()
    execution, registry = read(execution_path), read(registry_path)
    paths = [Path(__file__), execution_path, registry_path, ROOT / 'scripts/verify_declared_driver_contracts_5_0_6.py']
    hashes = lambda: {str(p): digest(p) for p in paths}
    record = dict(complete=False, scope=__doc__, controls=[], input_hashes_before=hashes())
    validate(execution, registry)

    def reject(name, mutate):
        changed = copy.deepcopy(execution)
        mutate(changed)
        assert changed != execution
        try:
            validate(changed, registry)
        except (AssertionError, KeyError, StopIteration, ValueError):
            record['controls'].append(dict(name=name, rejected=True))
        else:
            raise AssertionError('Corrupt declared-driver evidence accepted: ' + name)

    reject('different_runtime_baseline', lambda r: r['registry'].update(runtime_commit='0' * 40))
    reject('missing_reference_source', lambda r: r['reference_source_files'].pop(next(iter(r['reference_source_files']))))
    reject('changed_reference_source', lambda r: r['reference_source_files'].update({next(iter(r['reference_source_files'])): '0' * 64}))
    reject('shared_target_is_not_owned_build', lambda r: r['cargo_environment'].update(CARGO_TARGET_DIR='/tmp/shared_target'))
    project = next(iter(execution['projects']))
    build = execution['projects'][project]['build_step']
    reject('unlocked_compilation', lambda r: r['steps'][build]['command'].remove('--locked'))
    reject('failed_compilation', lambda r: r['steps'][build].update(exit_code=1))
    reject('manifest_override', lambda r: r['projects'][project].update(manifest=r['projects'][project]['manifest'] + '\n[patch.crates-io]\n'))
    reject('changed_locked_dependency_record', lambda r: r['projects'][project].update(lock=r['projects'][project]['lock'] + '\n# replaced\n'))

    def metadata(r, edit):
        step = r['steps'][r['projects'][project]['metadata_step']]
        value = json.loads(step['stdout'])
        edit(value)
        step['stdout'] = json.dumps(value)

    def wrong_target(value):
        package = next(p for p in value['packages'] if p['manifest_path'] == str(ROOT / project))
        selected = next(c['target'] for c in registry['contracts'] if c['project'] == project)
        next(t for t in package['targets'] if t['name'] == selected)['src_path'] = '/tmp/not_the_declared_driver.rs'

    reject('different_compiled_driver', lambda r: metadata(r, wrong_target))

    def wrong_local_dependency(value):
        next(p for p in value['packages'] if p['name'] == 'teaql-core')['manifest_path'] = '/tmp/replaced_core/Cargo.toml'

    reject('unaccounted_local_dependency', lambda r: metadata(r, wrong_local_dependency))
    reject('missing_original_matrix', lambda r: r['contracts'].pop())
    result = execution['contracts'][0]
    reject('changed_original_input_payload', lambda r: r['steps'][result['original_step']].update(stdin_sha256='0' * 64))

    def output(r, side, edit, index=0):
        item = r['contracts'][index]
        step = r['steps'][item[side + '_step']]
        values = json_lines(step['stdout'])
        edit(values)
        step['stdout'] = ''.join(json.dumps(v, ensure_ascii=True) + '\n' for v in values)

    reject('missing_original_record', lambda r: output(r, 'original', lambda v: v.pop()))
    reject('duplicate_original_record', lambda r: output(r, 'original', lambda v: v.append(v[0])))
    reject('missing_original_member', lambda r: output(r, 'original', lambda v: v[0].pop(next(iter(v[0])))))
    reject('missing_native_record', lambda r: output(r, 'native', lambda v: v.pop()))
    reject('duplicate_native_record', lambda r: output(r, 'native', lambda v: v.append(v[0])))

    def replace_bool(values):
        def visit(value):
            if isinstance(value, dict):
                for key, child in value.items():
                    if type(child) is bool:
                        value[key] = int(child)
                        return True
                    if visit(child):
                        return True
            if isinstance(value, list):
                for i, child in enumerate(value):
                    if type(child) is bool:
                        value[i] = int(child)
                        return True
                    if visit(child):
                        return True
            return False

        assert visit(values), 'Positive boolean field required'

    def has_bool(value):
        if type(value) is bool:
            return True
        if isinstance(value, dict):
            return any(has_bool(v) for v in value.values())
        if isinstance(value, list):
            return any(has_bool(v) for v in value)
        return False

    boolean_index = next(i for i, item in enumerate(execution['contracts'])
                         if has_bool(json_lines(execution['steps'][item['native_step']]['stdout'])))
    reject('native_boolean_is_not_integer', lambda r: output(r, 'native', replace_bool, boolean_index))
    for name in ('rust_field_layout', 'rust_sql_protocol_5_0_6'):
        lock = ROOT / 'tests/parity' / name / 'Cargo.lock'
        try:
            check_dependency_lock(lock, ROOT / '.reference/teaql-rs/Cargo.lock')
        except AssertionError:
            record['controls'].append(dict(name='actual_legacy_lock_' + name, rejected=True, lock_sha256=digest(lock)))
        else:
            raise AssertionError('Actual legacy lock unexpectedly qualifies: ' + name)
    assert len(record['controls']) == 20
    record['input_hashes_after'] = hashes()
    assert record['input_hashes_before'] == record['input_hashes_after']
    record['complete'] = True
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print(len(registry['contracts']), 'untouched bindings pass; 18 effective corruptions and two actual legacy locks reject')


if __name__ == '__main__':
    main()
