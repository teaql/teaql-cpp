#!/usr/bin/env python3
"""Bind the cross-context metadata corpus to actual locked Rust execution."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path

from compare_values import json_lines, same


ROOT = Path(__file__).resolve().parents[2]
PRODUCER = ROOT.parent / 'teaql_code_gen_cpp'
FIXTURE = ROOT / 'tests/parity/audit_cross_context_composition_5_0_6_inputs.json'
EXECUTION = ROOT / 'docs/parity/audit_cross_context_composition_maven_5_0_6.json.gz'
CORPUS = PRODUCER / 'generator/src/test/resources/cpp_cross_context_composition_metadata_5_0_6.json.gz'
CALLERS = ROOT / 'docs/parity/audit_cross_context_callers_5_0_6.json.gz'
CALLER_NAMES = (
    'cross_match_local_item_direct_audit_value_max_len',
    'cross_try_block_local_call_argument_audit_value_max_len_int',
    'cross_malformed_match_arm_audit_value_max_len',
)


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()) if path.suffix == '.gz'
                      else path.read_bytes())


def validate(fixture, execution, corpus, callers):
    runtime = read(ROOT / 'upstream.lock.json')['commit']
    generator = read(PRODUCER / 'cpp_generator.lock.json')['generator_baseline_commit']
    require(fixture['schema'] == corpus['schema'] == 1, 'Corpus schema')
    require(fixture['runtime_commit'] == corpus['runtime_commit'] == runtime,
            'Locked runtime revision')
    require(fixture['generator_commit'] == corpus['generator_commit'] == generator,
            'Locked generator revision')
    require(fixture['construction'] == dict(base_expressions=10, contexts=4, keys=2,
                                            malformed_expressions=6), 'Finite input construction')
    require(len(fixture['cases']) == len(corpus['cases']) == 92, 'Exact case count')
    require(len({row['name'] for row in fixture['cases']}) == 92, 'Unique case names')
    require(corpus['source_receipt'] == str(EXECUTION.relative_to(ROOT))
            and corpus['source_receipt_sha256'] == digest(EXECUTION),
            'Exact immutable source execution')
    require(execution['complete'] is True and execution['execution_complete'] is True
            and execution['whole_goal_complete'] is False, 'Complete bounded execution')
    require(execution['retained_count'] == 80185 and execution['new_count'] == 92
            and len(execution['cases']) == 80277, 'Retained and new source scope')
    require(not execution['differences'] and not execution['witness_differences']
            and not execution['retained_source_differences'], 'Actual source agreement')
    require(execution['input_hashes_before'] == execution['input_hashes_after'],
            'Unchanged execution inputs')
    for name, expected in execution['input_hashes_before'].items():
        require(digest(Path(name)) == expected, 'Current source input changed: ' + name)
    require(execution['owned_directory_removed'] is True
            and not Path(execution['owned_directory']).exists(), 'Owned build removed')
    steps = execution['steps']
    require([step['label'] for step in steps] == [
        'upstream', 'rust_build', 'rust_run', 'classpath', 'java_build', 'java_run']
            and all(step['exit_code'] == 0 for step in steps), 'Actual process sequence')
    original = json_lines(steps[2]['stdout'])
    current = json_lines(steps[5]['stdout'])
    require(len(original) == len(current) == 80277, 'Complete raw result cardinality')
    selected = execution['cases'][80185:]
    require(all(row['retained'] is False for row in selected), 'New case partition')
    for index, (item, row, source, java) in enumerate(zip(
            fixture['cases'], selected, original[80185:], current[80185:], strict=True)):
        require((item['name'], item['input']) == (row['name'], row['input']),
                'Input binding at ' + str(index))
        require(same(row['source_witness'], source['witness'])
                and same(row['original'], source['metadata']) and same(row['java'], java),
                'Raw source/result binding at ' + str(index))
        require(row['match'] is True, 'Original/current mismatch at ' + str(index))
        if 'witness' in item:
            require(row['source_witness'] == {'variant': item['witness']['variant']},
                    'Actual locked syntax witness at ' + str(index))
        require(same(corpus['cases'][index], dict(name=row['name'], input=row['input'],
                                                  expected=row['original'])),
                'Permanent expected result at ' + str(index))
    require(sum(row['original']['parsed'] is True for row in selected) == 90
            and sum(row['original']['parsed'] is False for row in selected) == 2,
            'Partial-effect and refusal partition')
    require(callers['complete'] is True and callers['inputs_unchanged'] is True
            and callers['candidate_overlays'] == {} and callers['binding_redactions'] is False,
            'Fresh unmodified generated callers')
    require(callers['runtime_commit'] == runtime and callers['generator_commit'] == generator,
            'Generated caller reference revisions')
    require(callers['input_hashes_before'] == callers['input_hashes_after']
            and callers['input_hashes_before'][str(CORPUS)] == digest(CORPUS),
            'Generated caller input epoch')
    for name, expected in callers['input_hashes_before'].items():
        require(digest(Path(name)) == expected, 'Current generated caller input changed: ' + name)
    require(callers['owned_directory_removed'] is True
            and not Path(callers['owned_directory']).exists(), 'Generated caller build removed')
    require(len(callers['steps']) == 23
            and all(step['exit_code'] == 0 for step in callers['steps']),
            'Complete original/C++ generated build and execution')
    require([case['name'] for case in callers['cases']] == list(CALLER_NAMES),
            'Three exact generated models')
    native = json.loads(next(step for step in callers['steps']
                             if step['label'] == 'native_run')['stdout'])
    require(len(native) == 3, 'Three native generated result groups')
    for index, case in enumerate(callers['cases']):
        selected_case = next(row for row in corpus['cases'] if row['name'] == case['name'])
        original_step = next(step for step in callers['steps']
                             if step['label'] == case['name'] + '_original_run')
        require(case['input'] == selected_case['input'] and case['match'] is True,
                'Generated model input and result match')
        require(same(case['original'], json.loads(original_step['stdout']))
                and same(case['cpp'], native[index])
                and same(case['original'], case['cpp']),
                'Actual original/native generated results')
        require(len(case['original']) == 3
                and sum(len(item['policies']) for item in case['original']) == 14,
                'Complete generated descriptor and field-policy surface')
        item = case['original'][1]
        require((item['entity'], item['limit']) ==
                (('Item', '13') if 'malformed' in case['name'] else ('After', '31')),
                'Partial or later generated metadata effect')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None:
        require(not args.record.exists(), 'Existing control record')
    fixture, execution, corpus, callers = read(FIXTURE), read(EXECUTION), read(CORPUS), read(CALLERS)
    validate(fixture, execution, corpus, callers)
    controls = []

    def reject(name, edited_fixture=fixture, edited_execution=execution,
               edited_corpus=corpus, edited_callers=callers):
        try:
            validate(edited_fixture, edited_execution, edited_corpus, edited_callers)
        except (RuntimeError, KeyError, IndexError, ValueError, TypeError):
            controls.append(name)
        else:
            raise RuntimeError('Corrupt cross-context evidence accepted: ' + name)

    changed = dict(execution, complete=False)
    reject('incomplete_execution', edited_execution=changed)
    changed = dict(execution, new_count=91)
    reject('missing_new_count', edited_execution=changed)
    changed = dict(execution, cases=execution['cases'][:-1])
    reject('truncated_execution', edited_execution=changed)
    changed = dict(execution, input_hashes_after={})
    reject('changed_input_epoch', edited_execution=changed)
    changed = dict(execution, steps=[*execution['steps']])
    changed['steps'][2] = dict(changed['steps'][2], stdout='')
    reject('empty_rust_output', edited_execution=changed)
    changed = dict(execution, steps=[*execution['steps']])
    changed['steps'][5] = dict(changed['steps'][5], stdout='')
    reject('empty_java_output', edited_execution=changed)
    changed = dict(corpus, source_receipt_sha256='0' * 64)
    reject('changed_source_receipt', edited_corpus=changed)
    changed = dict(corpus, cases=copy.deepcopy(corpus['cases']))
    changed['cases'][0]['expected']['limit'] = '30'
    reject('changed_expected_limit', edited_corpus=changed)
    changed = dict(corpus, cases=corpus['cases'][:-1])
    reject('truncated_corpus', edited_corpus=changed)
    changed = dict(fixture, cases=copy.deepcopy(fixture['cases']))
    changed['cases'][0]['input'] += ', audit_value_max_len=32'
    reject('changed_model_input', edited_fixture=changed)
    changed = dict(callers, complete=False)
    reject('incomplete_generated_callers', edited_callers=changed)
    changed = dict(callers, cases=[*callers['cases']])
    changed['cases'][0] = dict(changed['cases'][0], match=False)
    reject('changed_generated_match', edited_callers=changed)
    changed = dict(callers, steps=[*callers['steps']])
    final_step = next(index for index, step in enumerate(changed['steps'])
                      if step['label'] == 'native_run')
    changed['steps'][final_step] = dict(changed['steps'][final_step], stdout='[]')
    reject('empty_generated_native_output', edited_callers=changed)
    changed = dict(callers, input_hashes_after={})
    reject('changed_generated_input_epoch', edited_callers=changed)
    require(len(controls) == 14, 'Fourteen effective corruption controls')
    result = dict(complete=True, whole_goal_complete=False, cases=92,
                  accepted_metadata=90, source_refusals=2,
                  generated_models=3, generated_descriptors=9, generated_field_policies=42,
                  controls=controls,
                  fixture_sha256=digest(FIXTURE), execution_sha256=digest(EXECUTION),
                  corpus_sha256=digest(CORPUS), callers_sha256=digest(CALLERS))
    if args.record is not None:
        args.record.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(), mtime=0))
    print('92/92 metadata cases, 3/3 generated callers and 14/14 corruptions verified')


if __name__ == '__main__':
    main()
