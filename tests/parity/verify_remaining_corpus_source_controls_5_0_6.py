#!/usr/bin/env python3
"""Reject effective dependency and raw-output corruptions of the fresh seven-corpus record."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import re
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_remaining_corpus_source_contracts_5_0_6 import (
    EXECUTION, REGISTRY, digest, read, validate_common, validate_contract)
from compare_values import json_lines


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    assert args.record is None or not args.record.exists()
    execution = read(EXECUTION)
    registry = json.loads(REGISTRY.read_text())
    contracts = {c['name']: c for c in registry['contracts']}
    results = {c['name']: c for c in execution['contracts']}
    paths = [Path(__file__), REGISTRY, EXECUTION,
             ROOT / 'scripts/verify_remaining_corpus_source_contracts_5_0_6.py',
             ROOT / 'scripts/corpus_input_contracts_5_0_6.py']
    hashes = lambda: {str(p): digest(p) for p in paths}
    record = dict(complete=False, scope=__doc__, controls=[], input_hashes_before=hashes())
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_remaining_corpus_controls_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            validate_common(execution, registry, owned)
            for name, contract in contracts.items():
                validate_contract(contract, results[name], execution)

            def reject(name, mutate, selected=None):
                changed = copy.deepcopy(execution)
                mutate(changed)
                assert changed != execution
                try:
                    if selected is None:
                        validate_common(changed, registry, owned)
                    else:
                        result = next(c for c in changed['contracts'] if c['name'] == selected)
                        validate_contract(contracts[selected], result, changed)
                except (AssertionError, KeyError, StopIteration, ValueError):
                    record['controls'].append(dict(name=name, contract=selected, rejected=True))
                else:
                    raise AssertionError('Corrupt source evidence accepted: ' + name)

            reject('different_runtime_baseline', lambda r: r['registry'].update(runtime_commit='0' * 40))
            reject('missing_reference_source', lambda r: r['reference_source_files'].pop(next(iter(r['reference_source_files']))))
            reject('changed_reference_source', lambda r: r['reference_source_files'].update({next(iter(r['reference_source_files'])): '0' * 64}))
            reject('shared_target_is_not_fresh_owned_build', lambda r: r['cargo_environment'].update(CARGO_TARGET_DIR='/tmp/shared_target'))
            project = next(iter(execution['projects']))
            build_index = execution['projects'][project]['build_step']
            reject('unlocked_original_compilation', lambda r: r['steps'][build_index]['command'].remove('--locked'))
            reject('failed_original_compilation', lambda r: r['steps'][build_index].update(exit_code=1))
            reject('unaccounted_dependency_override', lambda r: r['projects'][project].update(
                manifest=r['projects'][project]['manifest'] + '\n[patch.crates-io]\n'))

            def wrong_checksum(r):
                view = r['runtime_view']
                view['lock'] = re.sub(r'checksum = "[a-f0-9]+"', 'checksum = "' + '0' * 64 + '"', view['lock'], count=1)

            reject('different_registry_checksum', wrong_checksum)
            reject('changed_original_runtime_prefix', lambda r: r['runtime_view']['source_files_sha256'].update({
                next(iter(r['runtime_view']['source_files_sha256'])): '0' * 64}))

            def wrong_observer(r):
                observer = r['runtime_view']['observers']['src/generated_support.rs']
                observer['text'] += '\n// not the pinned source appendix\n'
                observer['sha256'] = hashlib.sha256(observer['text'].encode()).hexdigest()

            reject('self_consistently_rehashed_observer', wrong_observer)

            def change_metadata(r, modify):
                step = r['steps'][r['projects'][project]['metadata_step']]
                value = json.loads(step['stdout'])
                modify(value)
                step['stdout'] = json.dumps(value)

            def wrong_target(value):
                package = next(p for p in value['packages'] if p['manifest_path'] == str(ROOT / project))
                next(t for t in package['targets'] if t['name'] == 'inmemory_source_506')['src_path'] = '/tmp/unrelated.rs'

            reject('different_compiled_source_target', lambda r: change_metadata(r, wrong_target))

            def wrong_local_dependency(value):
                package = next(p for p in value['packages'] if p['name'] == 'teaql-core')
                package['manifest_path'] = '/tmp/unrelated_local_dependency/Cargo.toml'

            reject('unaccounted_local_dependency', lambda r: change_metadata(r, wrong_local_dependency))

            def item(r, name):
                return next(c for c in r['contracts'] if c['name'] == name)

            def output(r, selected, side, change):
                result = item(r, selected)
                index = result['reference']['step'] if side == 'reference' else result['native_step']
                step = r['steps'][index]
                values = json_lines(step['stdout'])
                change(values, result)
                step['stdout'] = ''.join(json.dumps(v, ensure_ascii=True) + '\n' for v in values)

            constructor = 'row_constructor'
            reject('missing_original_record', lambda r: output(r, constructor, 'reference', lambda v, c: v.pop()), constructor)
            reject('duplicated_original_record', lambda r: output(r, constructor, 'reference', lambda v, c: v.append(v[0])), constructor)
            reject('missing_original_member', lambda r: output(r, constructor, 'reference', lambda v, c: v[0].pop('row')), constructor)
            reject('changed_original_value', lambda r: output(r, constructor, 'reference', lambda v, c: v[0].update(row='corrupt')), constructor)

            def change_original_and_summary(values, result):
                values[0]['row'] = 'corrupt'
                result['reference']['observations'][0]['row'] = 'corrupt'

            reject('changed_original_and_summary_still_need_corpus', lambda r: output(
                r, constructor, 'reference', change_original_and_summary), constructor)
            reject('missing_native_record', lambda r: output(r, constructor, 'native', lambda v, c: v.pop()), constructor)
            reject('duplicated_native_record', lambda r: output(r, constructor, 'native', lambda v, c: v.append(v[0])), constructor)
            reject('missing_native_member', lambda r: output(r, constructor, 'native', lambda v, c: v[0].pop('row')), constructor)

            def bool_to_integer(values, result):
                entry = next(v for v in values if type(v['privacy']) is bool)
                entry['privacy'] = int(entry['privacy'])

            privacy = 'mutation_intent_privacy'
            reject('native_boolean_is_not_integer', lambda r: output(r, privacy, 'native', bool_to_integer), privacy)
            reject('different_native_input_payload', lambda r: r['steps'][item(r, constructor)['native_step']].update(stdin_sha256='0' * 64), constructor)
            support = 'generated_support_source'
            reject('different_source_file_output_digest', lambda r: item(r, support)['reference'].update(output_sha256='0' * 64), support)

            def change_source_file(r):
                original = item(r, support)['reference']
                values = json.loads(original['raw_output'])
                values[0]['base'] = 'corrupt'
                original['raw_output'] = json.dumps(values)
                original['output_sha256'] = hashlib.sha256(original['raw_output'].encode()).hexdigest()

            reject('rehashed_file_output_still_needs_observations', change_source_file, support)
            assert len(record['controls']) == 24
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['input_hashes_after'] = hashes()
        assert record['input_hashes_before'] == record['input_hashes_after']
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Seven untouched source bindings pass; 24 effective compile/dependency/raw-output corruptions reject')


if __name__ == '__main__':
    main()
