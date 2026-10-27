#!/usr/bin/env python3
"""Reject missing or substituted pagination source, dependencies and observations."""
import argparse
import copy
import gzip
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from pagination_driver_provenance_5_0_6 import archive, validate


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path, required=True)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    assert args.record is None or not args.record.exists()
    original, registry = archive.read(args.execution), archive.read(args.contracts)
    validate(original, registry)
    controls = []

    def reject(name, edit):
        changed, contract = copy.deepcopy(original), copy.deepcopy(registry)
        edit(changed, contract)
        assert changed != original or contract != registry
        try:
            validate(changed, contract)
        except (AssertionError, KeyError, StopIteration, ValueError):
            controls.append(dict(name=name, rejected=True))
        else:
            raise AssertionError('Corrupt pagination evidence accepted: ' + name)

    reject('missing_result_matrix', lambda r, c: r['results'].pop())
    reject('missing_archive_source', lambda r, c: r['archive_files'].pop('teaql-runtime/src/context.rs'))
    reject('changed_generated_model', lambda r, c: r['source_files'].update({
        c['generated_fixture'] + '/lib/src/order/mod.rs': '0' * 64}))
    reject('changed_input_recipe', lambda r, c: c['contracts'][0].update(input_recipe_sha256='0' * 64))
    reject('incomplete_source_files', lambda r, c: r['source_files'].pop(next(iter(r['source_files']))))
    reject('changed_dependency_lock', lambda r, c: r.update(compiled_lock=''))
    reject('shared_cargo_target', lambda r, c: r['cargo_environment'].update(CARGO_TARGET_DIR='/tmp/shared'))
    reject('unlocked_build', lambda r, c: r['steps'][r['build_step']]['command'].remove('--locked'))
    reject('failed_build', lambda r, c: r['steps'][r['build_step']].update(exit_code=1))
    reject('partial_bin_build', lambda r, c: r['steps'][r['build_step']]['command'].remove('--bins'))

    def metadata(record, edit):
        step = record['steps'][record['metadata_step']]
        value = json.loads(step['stdout'])
        edit(value)
        step['stdout'] = record['project_metadata'] = json.dumps(value)

    def package(value, name):
        return next(p for p in value['packages'] if p['name'] == name)

    reject('substituted_original_target', lambda r, c: metadata(r, lambda v:
        package(v, c['package'])['targets'][0].update(src_path='/tmp/replaced.rs')))
    reject('unowned_generated_dependency', lambda r, c: metadata(r, lambda v:
        package(v, 'cpp-dynamic-json-core').update(manifest_path='/tmp/replaced/Cargo.toml')))
    reject('different_runtime_version', lambda r, c: metadata(r, lambda v:
        package(v, 'teaql-runtime').update(version='5.0.5')))

    def step(record, native=False):
        return record['steps'][record['results'][0]['native_step' if native else 'source_step']]

    reject('changed_source_payload', lambda r, c: step(r).update(stdin_sha256='0' * 64))
    reject('substituted_source_executable', lambda r, c: step(r)['command'].__setitem__(0, '/tmp/replaced'))
    reject('empty_original_output', lambda r, c: step(r).update(stdout=''))

    def output(record, edit):
        selected = step(record, True)
        values = archive.json_lines(selected['stdout'])
        edit(values)
        selected['stdout'] = ''.join(json.dumps(v) + '\n' for v in values)

    reject('missing_native_result', lambda r, c: output(r, lambda v: v.pop()))
    reject('extra_native_result', lambda r, c: output(r, lambda v: v.append(v[0])))

    def corrupt_boolean(value):
        if isinstance(value, dict):
            for key, child in value.items():
                if type(child) is bool:
                    value[key] = int(child)
                    return True
                if corrupt_boolean(child):
                    return True
        if isinstance(value, list):
            return any(corrupt_boolean(child) for child in value)
        return False

    def boolean_control(record, contract):
        def change(values):
            assert corrupt_boolean(values)
        output(record, change)

    reject('boolean_is_not_number', boolean_control)
    result = dict(complete=True, whole_goal_complete=False, controls=controls,
        execution_sha256=archive.digest(args.execution), registry_sha256=archive.digest(args.contracts))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
    print(len(controls), 'pagination provenance/result controls reject; original receipt passes.')


if __name__ == '__main__':
    main()
