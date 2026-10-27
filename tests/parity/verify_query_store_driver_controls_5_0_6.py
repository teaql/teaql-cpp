#!/usr/bin/env python3
"""Reject extraction, reduced-Context, dependency and incomplete-output substitutions."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_query_store_driver_contracts_5_0_6 import read, validate, digest, registry_identities
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
            raise AssertionError('Corrupt source evidence accepted: ' + name)

    reject('different_baseline', lambda r: r['registry'].update(runtime_commit='0' * 40))
    reject('missing_reference_source', lambda r: r['reference_source_files'].pop(next(iter(r['reference_source_files']))))
    reject('changed_reference_source', lambda r: r['reference_source_files'].update({next(iter(r['reference_source_files'])): '0' * 64}))
    reject('shared_target', lambda r: r['cargo_environment'].update(CARGO_TARGET_DIR='/tmp/shared_target'))
    reject('missing_extraction', lambda r: r['extracted_files'].pop('query_stores.rs'))

    def rehash_extraction(r):
        value = r['extracted_files']['id_set_build_lock.rs']
        value['text'] += '\n// changed algorithm\n'
        value['sha256'] = hashlib.sha256(value['text'].encode()).hexdigest()

    reject('self_consistently_rehashed_extraction', rehash_extraction)

    def change_adapter(r):
        r['observer_adapter_source'] += '\n# changed observer\n'
        r['source_execution']['verifier_sha256'] = hashlib.sha256(r['observer_adapter_source'].encode()).hexdigest()

    reject('self_consistently_rehashed_adapter', change_adapter)
    reject('changed_manifest', lambda r: r['project_files'].update({'Cargo.toml': r['project_files']['Cargo.toml'] + '\n# changed\n'}))

    def metadata(r, edit):
        data = json.loads(r['project_metadata'])
        edit(data)
        r['project_metadata'] = json.dumps(data)
        next(s for s in r['steps'] if s['label'] == 'metadata')['stdout'] = r['project_metadata']

    def wrong_target(data):
        package = next(p for p in data['packages'] if p['manifest_path'] == str(ROOT / registry['project']))
        next(t for t in package['targets'] if t['name'] == registry['contracts'][0]['target'])['src_path'] = '/tmp/replaced.rs'

    reject('different_compiled_driver', lambda r: metadata(r, wrong_target))
    reject('unowned_local_dependency', lambda r: metadata(r, lambda data: next(p for p in data['packages'] if p['name'] == 'teaql-core').update(manifest_path='/tmp/replaced/Cargo.toml')))

    def build(r):
        return next(s for s in r['source_execution']['steps'] if s['command'][:2] == ['cargo', 'build'])

    reject('unlocked_compilation', lambda r: build(r)['command'].remove('--locked'))
    reject('failed_compilation', lambda r: build(r).update(exit_code=1))
    reject('missing_matrix', lambda r: r['source_execution']['matrices'].pop())

    def result_step(r, native=False):
        binary = (ROOT / 'build_native_current' / registry['contracts'][0]['native_target']
                  if native else Path(r['owned_directory']) / 'target/debug' / registry['contracts'][0]['target'])
        return next(s for s in r['source_execution']['steps'] if s['command'] == [str(binary)])

    reject('changed_payload', lambda r: result_step(r).update(stdin_sha256='0' * 64))

    def output(r, native, edit):
        step = result_step(r, native)
        values = json_lines(step['stdout'])
        edit(values)
        step['stdout'] = ''.join(json.dumps(v, ensure_ascii=True) + '\n' for v in values)

    reject('missing_original_result', lambda r: output(r, False, lambda values: values.pop()))
    reject('duplicate_original_result', lambda r: output(r, False, lambda values: values.append(values[0])))
    reject('missing_native_result', lambda r: output(r, True, lambda values: values.pop()))

    def replace_boolean(value):
        if isinstance(value, dict):
            for key, child in value.items():
                if type(child) is bool:
                    value[key] = int(child)
                    return True
                if replace_boolean(child):
                    return True
        if isinstance(value, list):
            for index, child in enumerate(value):
                if type(child) is bool:
                    value[index] = int(child)
                    return True
                if replace_boolean(child):
                    return True
        return False

    def corrupt_boolean(values):
        assert replace_boolean(values)

    reject('native_boolean_is_not_number', lambda r: output(r, True, corrupt_boolean))
    reject('partial_actual_context', lambda r: r['source_execution']['actual_context']['outputs'].pop())

    def raw_context(r):
        value = r['source_execution']['actual_context_files']
        outputs = json.loads(value['outputs'])
        outputs.pop()
        value['outputs'] = json.dumps(outputs)
        value['outputs_sha256'] = hashlib.sha256(value['outputs'].encode()).hexdigest()

    reject('rehashed_partial_context_file', raw_context)
    reject('reduced_shell_cannot_replace_context', lambda r: r['source_execution']['actual_context'].pop('source_view'))
    reject('missing_context_test', lambda r: r['source_execution']['actual_context'].update(contracts_passed=1))
    reject('changed_original_runtime_prefix', lambda r: r['source_execution']['actual_context']['source_view']['source_files_sha256'].update({'src/context/pagination.rs': '0' * 64}))

    def observer(r):
        value = r['source_execution']['actual_context']['source_view']['observers']['src/context/pagination.rs']
        value['text'] += '\n// substituted observer\n'
        value['sha256'] = hashlib.sha256(value['text'].encode()).hexdigest()

    reject('rehashed_context_observer', observer)
    reject('changed_view_manifest', lambda r: r['source_execution']['actual_context']['source_view'].update(manifest='replaced'))
    reject('unreviewed_omitted_dependency', lambda r: r['source_execution']['actual_context']['source_view']['omitted_dev_dependencies'].append('serde_json'))
    reject('wrong_context_input_path', lambda r: r['source_execution']['actual_context_files']['environment'].update(TEAQL_STORE_CONTEXT_INPUTS='/tmp/other.json'))
    for name in ['rust_field_layout', 'rust_query_optimization_5_0_6']:
        lock = ROOT / 'tests/parity' / name / 'Cargo.lock'
        assert not registry_identities(lock) <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
        controls.append(dict(name='actual_legacy_lock_' + name, rejected=True, lock_sha256=digest(lock)))
    result = dict(complete=True, whole_goal_complete=False, controls=controls,
                  registry_sha256=digest(args.contracts), execution_sha256=digest(args.execution))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
    print(len(controls), 'effective source/extraction/Context/result controls reject; untouched evidence passes.')


if __name__ == '__main__':
    main()
