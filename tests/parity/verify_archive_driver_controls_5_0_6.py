#!/usr/bin/env python3
"""Reject incomplete archive/project/source/recipe and raw execution bindings."""
import argparse
import copy
import gzip
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from archive_driver_provenance_5_0_6 import read, validate, digest, registry_identities
from compare_values import json_lines


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path, required=True)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    assert args.record is None or not args.record.exists()
    original, registry = read(args.execution), read(args.contracts)
    validate(original, registry)
    controls = []

    def reject(name, edit):
        changed = copy.deepcopy(original)
        edit(changed)
        assert changed != original
        try:
            validate(changed, registry)
        except (AssertionError, KeyError, StopIteration, ValueError):
            controls.append(dict(name=name, rejected=True))
        else:
            raise AssertionError('Corrupt archive driver evidence accepted: ' + name)

    reject('different_baseline', lambda r: r['registry'].update(runtime_commit='0' * 40))
    reject('missing_result_matrix', lambda r: r['results'].pop())
    reject('shared_target', lambda r: r['cargo_environment'].update(CARGO_TARGET_DIR='/tmp/shared_target'))
    reject('missing_archive_file', lambda r: r['archive_files'].pop(next(iter(r['archive_files']))))
    reject('changed_original_runtime', lambda r: r['archive_files'].update({'teaql-sql/src/executor.rs': '0' * 64}))
    reject('missing_copied_source', lambda r: r['source_files'].pop(next(iter(r['source_files']))))

    def change_support(r):
        r['source_files'][registry['support_files'][0]] = '0' * 64

    reject('changed_shared_source', change_support)
    reject('changed_compiled_manifest', lambda r: r['source_files'].update({registry['project']: '0' * 64}))
    reject('changed_input_helper', lambda r: r['input_helpers'].update({next(iter(registry['input_helpers'])): '0' * 64}))
    reject('changed_compiled_lock', lambda r: r.update(compiled_lock=''))
    reject('unlocked_build', lambda r: r['steps'][r['build_step']]['command'].remove('--locked'))
    reject('failed_build', lambda r: r['steps'][r['build_step']].update(exit_code=1))
    reject('partial_target_build', lambda r: r['steps'][r['build_step']]['command'].remove('--bins'))
    reject('failed_metadata', lambda r: r['steps'][r['metadata_step']].update(exit_code=1))

    def metadata(r, edit):
        step = r['steps'][r['metadata_step']]
        data = json.loads(step['stdout'])
        edit(data)
        step['stdout'] = r['project_metadata'] = json.dumps(data)

    def package(data):
        return next(p for p in data['packages'] if p['name'] == registry['package'])

    reject('missing_compiled_target', lambda r: metadata(r, lambda d: package(d)['targets'].pop()))
    reject('substituted_target', lambda r: metadata(r, lambda d: package(d)['targets'][0].update(src_path='/tmp/replaced.rs')))
    reject('unowned_local_dependency', lambda r: metadata(r, lambda d: next(p for p in d['packages'] if p['name'] == 'teaql-runtime').update(manifest_path='/tmp/replaced/Cargo.toml')))
    reject('wrong_runtime_version', lambda r: metadata(r, lambda d: next(p for p in d['packages'] if p['name'] == 'teaql-runtime').update(version='5.0.5')))

    def step(r, native=False):
        return r['steps'][r['results'][0]['native_step' if native else 'source_step']]

    reject('changed_payload', lambda r: step(r).update(stdin_sha256='0' * 64))
    reject('different_compiled_binary', lambda r: step(r)['command'].__setitem__(0, '/tmp/replaced'))
    reject('empty_original_output', lambda r: step(r).update(stdout=''))

    def output(r, native, edit):
        selected = step(r, native)
        values = json_lines(selected['stdout'])
        edit(values)
        selected['stdout'] = ''.join(json.dumps(v, ensure_ascii=True) + '\n' for v in values)

    reject('missing_native_result', lambda r: output(r, True, lambda values: values.pop()))
    reject('duplicate_native_result', lambda r: output(r, True, lambda values: values.append(values[0])))

    def replace_boolean(value):
        if isinstance(value, dict):
            for key, child in value.items():
                if type(child) is bool:
                    value[key] = int(child)
                    return True
                if replace_boolean(child):
                    return True
        if isinstance(value, list):
            for child in value:
                if replace_boolean(child):
                    return True
        return False

    def corrupt_boolean(r):
        for result in r['results']:
            selected = r['steps'][result['native_step']]
            values = json_lines(selected['stdout'])
            if replace_boolean(values):
                selected['stdout'] = ''.join(json.dumps(v, ensure_ascii=True) + '\n' for v in values)
                return
        raise AssertionError('No Boolean witness in the complete native observations')

    reject('native_boolean_is_not_number', corrupt_boolean)
    legacy = ROOT / 'tests/parity/rust_sql_protocol_5_0_6/Cargo.toml'
    assert not registry_identities(legacy.with_name('Cargo.lock')) <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    controls.append(dict(name='actual_legacy_sql_protocol_lock', rejected=True,
                         lock_sha256=digest(legacy.with_name('Cargo.lock'))))
    result = dict(complete=True, whole_goal_complete=False, controls=controls,
                  execution_sha256=digest(args.execution), registry_sha256=digest(args.contracts))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
    print(len(controls), 'archive/source/project/result controls reject; untouched evidence passes.')


if __name__ == '__main__':
    main()
