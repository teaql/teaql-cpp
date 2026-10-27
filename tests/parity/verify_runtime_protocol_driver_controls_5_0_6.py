#!/usr/bin/env python3
"""Reject private-source prefix, profile, dependency and output tampering."""
import argparse
import copy
import gzip
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from runtime_protocol_driver_provenance_5_0_6 import archive, validate


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
        changed['registry'] = copy.deepcopy(contract)
        try:
            validate(changed, contract)
        except (AssertionError, KeyError, StopIteration, ValueError, OSError):
            controls.append(dict(name=name, rejected=True))
        else:
            raise AssertionError('Corrupt private-runtime evidence accepted: ' + name)

    reject('missing_matrix', lambda r, c: r['results'].pop())
    reject('missing_archive_file', lambda r, c: r['archive_files'].pop('teaql-runtime/src/context.rs'))
    reject('changed_source_recipe', lambda r, c: r['source_files'].update(
        {c['observers'][0]['path']: '0' * 64}))
    reject('changed_compiled_prefix', lambda r, c: r['compiled_source_hashes'].update(
        {'src/data_service/base.rs': '0' * 64}))
    reject('missing_original_test_body', lambda r, c: r['view']['source_files_sha256'].pop(
        'src/commit_audit.rs'))
    reject('changed_appended_observer', lambda r, c: next(iter(
        r['view']['observers'].values())).update(text=''))
    reject('changed_runtime_manifest', lambda r, c: r['view'].update(manifest=''))
    reject('changed_dependency_lock', lambda r, c: r['view'].update(lock=''))
    reject('changed_omitted_dependencies', lambda r, c: r['view'].update(omitted_dev_dependencies=[]))
    reject('shared_target', lambda r, c: r['cargo_environment'].update(CARGO_TARGET_DIR='/tmp/shared'))
    reject('failed_build', lambda r, c: r['steps'][r['profiles']['debug']['compile_step']].update(exit_code=1))
    reject('unlocked_build', lambda r, c: r['steps'][r['profiles']['debug']['compile_step']]['command'].remove('--locked'))
    reject('release_compiled_as_debug', lambda r, c: r['steps'][r['profiles']['release']['compile_step']]['command'].remove('--release'))
    reject('wrong_release_assertions', lambda r, c: r['profiles']['release']['artifact']['profile'].update(debug_assertions=True))
    reject('missing_release_profile', lambda r, c: r['profiles'].pop('release'))
    reject('substituted_test_binary', lambda r, c: r['profiles']['debug'].update(binary='/tmp/replaced'))
    reject('changed_test_listing', lambda r, c: r['profiles']['debug']['test_names'].pop())
    reject('missing_original_statement_test', lambda r, c: next(s for s in r['steps']
        if s['label'] == 'original_statement_identity').update(stdout=''))
    reject('missing_ndebug_probe_flag', lambda r, c: r['native_counter_compile_modes']
        ['teaql_runtime_mutation_protocol_release_probe'].update(command='cc /teaql_runtime_mutation_protocol_release_probe.dir/probe.cpp'))
    reject('ndebug_flag_on_debug_probe', lambda r, c: r['native_counter_compile_modes']
        ['teaql_runtime_mutation_protocol_probe'].update(command='cc -DNDEBUG /teaql_runtime_mutation_protocol_probe.dir/probe.cpp'))

    def source_step(record):
        return record['steps'][record['results'][0]['source_step']]

    reject('empty_source_test_output', lambda r, c: source_step(r).update(stdout=''))

    def output(record, edit, native=False):
        result = record['results'][0]
        if native:
            step = record['steps'][result['native_step']]
            values = archive.json_lines(step['stdout'])
            edit(values)
            step['stdout'] = ''.join(json.dumps(v) + '\n' for v in values)
        else:
            edit(result['file_output'])

    reject('missing_source_value', lambda r, c: output(r, lambda v: v.pop()))
    reject('missing_native_value', lambda r, c: output(r, lambda v: v.pop(), True))
    reject('extra_native_value', lambda r, c: output(r, lambda v: v.append(v[0]), True))
    reject('changed_native_payload', lambda r, c: r['steps'][r['results'][0]['native_step']].update(stdin_sha256='0' * 64))

    def boolean(value):
        if isinstance(value, dict):
            for key, child in value.items():
                if type(child) is bool:
                    value[key] = int(child)
                    return True
                if boolean(child):
                    return True
        if isinstance(value, list):
            return any(boolean(child) for child in value)
        return False

    def type_confusion(record, contract):
        def edit(values):
            assert boolean(values)
        output(record, edit, True)

    reject('boolean_is_not_number', type_confusion)
    result = dict(complete=True, whole_goal_complete=False, controls=controls,
                  execution_sha256=archive.digest(args.execution),
                  registry_sha256=archive.digest(args.contracts))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
    print(len(controls), 'private runtime source/profile/result controls reject.')


if __name__ == '__main__':
    main()
