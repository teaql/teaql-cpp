#!/usr/bin/env python3
"""Reject archive, bridge, project, extraction and partial-result substitutions."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_private_runtime_driver_contracts_5_0_6 import read, validate, digest
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
            raise AssertionError('Corrupt private-runtime evidence accepted: ' + name)

    reject('different_baseline', lambda r: r['registry'].update(runtime_commit='0' * 40))
    reject('partial_results', lambda r: r['results'].pop())
    reject('shared_target', lambda r: r['cargo_environment'].update(CARGO_TARGET_DIR='/tmp/shared_target'))
    reject('missing_archive_file', lambda r: r['archive_files'].pop(next(iter(r['archive_files']))))
    reject('changed_archive_prefix', lambda r: r['archive_files']['teaql-runtime/src/entity_save.rs'].update(original_sha256='0' * 64))
    reject('changed_compiled_archive', lambda r: r['archive_files']['teaql-runtime/src/entity_save.rs'].update(compiled_sha256='0' * 64))
    reject('changed_bridge', lambda r: r['appendices'].update({'teaql-runtime/src/context.rs': 'replaced'}))
    reject('missing_builder_helper', lambda r: r['builder_inputs'].pop(next(iter(r['builder_inputs']))))
    reject('changed_manifest', lambda r: r['project_files'].update({'Cargo.toml': 'replaced'}))
    reject('changed_build_script', lambda r: r['project_files'].update({'build.rs': 'replaced'}))
    reject('replaced_source_link', lambda r: r['source_links'].update(src='/tmp/replaced'))
    reject('changed_fixture', lambda r: r['fixture_inputs'].update({'tests/parity/rust_values/src/fixture_value.rs': '0' * 64}))
    reject('unlocked_build', lambda r: r['steps'][r['build_step']]['command'].remove('--locked'))
    reject('failed_build', lambda r: r['steps'][r['build_step']].update(exit_code=1))
    reject('partial_target_build', lambda r: r['steps'][r['build_step']]['command'].remove('--bins'))
    reject('empty_compiled_lock', lambda r: r.update(compiled_lock=''))

    def metadata(r, edit):
        step = r['steps'][r['metadata_step']]
        data = json.loads(step['stdout'])
        edit(data)
        step['stdout'] = json.dumps(data)

    def source_package(data):
        return next(p for p in data['packages'] if p['name'] == 'teaql_cpp_private_save_oracle')

    reject('missing_compiled_target', lambda r: metadata(r, lambda data: source_package(data)['targets'].pop()))
    reject('substituted_target_path', lambda r: r['resolved_target_paths'].update({registry['contracts'][0]['target']: '/tmp/replaced.rs'}))
    reject('unowned_local_dependency', lambda r: metadata(r, lambda data: next(p for p in data['packages'] if p['name'] == 'teaql-runtime').update(manifest_path='/tmp/replaced/Cargo.toml')))
    reject('missing_extraction', lambda r: r['extracted_files'].pop('relation_execution.rs'))

    def rehash_extraction(r):
        value = r['extracted_files']['relation_execution.rs']
        value['text'] += '\n// substituted method\n'
        value['sha256'] = hashlib.sha256(value['text'].encode()).hexdigest()

    reject('rehashed_extraction', rehash_extraction)

    def result_step(r, native=False):
        return r['steps'][r['results'][0]['native_step' if native else 'source_step']]

    reject('changed_payload', lambda r: result_step(r).update(stdin_sha256='0' * 64))
    reject('different_compiled_binary', lambda r: result_step(r)['command'].__setitem__(0, '/tmp/replaced'))

    def output(r, native, edit):
        step = result_step(r, native)
        values = json_lines(step['stdout'])
        edit(values)
        step['stdout'] = ''.join(json.dumps(v, ensure_ascii=True) + '\n' for v in values)

    reject('missing_original_result', lambda r: output(r, False, lambda values: values.pop()))
    reject('duplicate_original_result', lambda r: output(r, False, lambda values: values.append(values[0])))
    reject('empty_native_output', lambda r: result_step(r, True).update(stdout=''))

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
            step = r['steps'][result['native_step']]
            values = json_lines(step['stdout'])
            if replace_boolean(values):
                step['stdout'] = ''.join(json.dumps(v, ensure_ascii=True) + '\n' for v in values)
                return
        raise AssertionError('No Boolean witness in the complete native observations')

    reject('native_boolean_is_not_number', corrupt_boolean)

    def change_adapter_identity(r, name):
        key = 'runtime:tests/parity/' + name
        r['input_hashes_before'][key] = r['input_hashes_after'][key] = '0' * 64

    for name in ['replay_layered_query_intent_5_0_6.py', 'replay_relation_hydration_intent_5_0_6.py']:
        reject('changed_historical_adapter_' + name, lambda r, name=name: change_adapter_identity(r, name))
    result = dict(complete=True, whole_goal_complete=False, controls=controls,
                  execution_sha256=digest(args.execution), registry_sha256=digest(args.contracts))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
    print(len(controls), 'archive/project/extraction/result controls reject; untouched evidence passes.')


if __name__ == '__main__':
    main()
