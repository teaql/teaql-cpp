#!/usr/bin/env python3
"""Reject incomplete private-function evidence, including scalar Boolean confusion."""
import argparse
import copy
import gzip
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from private_key_classifier_driver_provenance_5_0_6 import archive, validate


def main(validator=validate, extra_controls=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path, required=True)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    assert args.record is None or not args.record.exists()
    original, registry = archive.read(args.execution), archive.read(args.contracts)
    validator(original, registry)
    controls = []

    def reject(name, edit):
        changed, contract = copy.deepcopy(original), copy.deepcopy(registry)
        edit(changed, contract)
        changed['registry'] = copy.deepcopy(contract)
        assert changed != original or contract != registry
        try:
            validator(changed, contract)
        except (AssertionError, KeyError, StopIteration, ValueError, OSError):
            controls.append(dict(name=name, rejected=True))
        else:
            raise AssertionError('Corrupt generated-model evidence accepted: ' + name)

    reject('missing_result_matrix', lambda r, c: r['results'].pop())
    reject('missing_archive_source', lambda r, c: r['archive_files'].pop('teaql-runtime/src/context.rs'))
    def generated_source(record, contract, index):
        prefix = contract['generated_fixtures'][index] + '/lib/src/'
        path = next(n for n in record['source_files'] if n.startswith(prefix))
        record['source_files'][path] = '0' * 64

    if registry['generated_fixtures']:
        reject('changed_generated_model', lambda r, c: generated_source(r, c, 0))
    else:
        reject('changed_compiled_driver', lambda r, c: r['source_files'].update({
            c['contracts'][0]['driver']: '0' * 64}))
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
    def generated_package(index):
        path = str(Path(original['owned_directory']) / registry['generated_fixtures'][index]
                   / 'lib/Cargo.toml')
        return next(p['name'] for p in json.loads(original['project_metadata'])['packages']
                    if p['manifest_path'] == path)

    dependency = generated_package(0) if registry['generated_fixtures'] else 'teaql-runtime'
    reject('unowned_generated_dependency' if registry['generated_fixtures'] else
           'unowned_runtime_dependency', lambda r, c: metadata(r, lambda v:
        package(v, dependency).update(manifest_path='/tmp/replaced/Cargo.toml')))
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
            for index, child in enumerate(value):
                if type(child) is bool:
                    value[index] = int(child)
                    return True
                if corrupt_boolean(child):
                    return True
        return False

    def boolean_control(record, contract):
        for result in record['results']:
            selected = record['steps'][result['native_step']]
            values = archive.json_lines(selected['stdout'])
            if corrupt_boolean(values):
                selected['stdout'] = ''.join(json.dumps(v) + '\n' for v in values)
                return
        raise AssertionError('No Boolean value in the complete native result matrices')

    reject('boolean_is_not_number', boolean_control)

    if any(c.get('corpus_source_root') == 'project' for c in registry['contracts']):
        reject('wrong_project_source_root', lambda r, c:
            next(x for x in c['contracts'] if x.get('corpus_source_root') == 'project')
            .update(corpus_source_root='runtime'))
    if any(c['input_builder']['kind'] == 'fixture' for c in registry['contracts']):
        reject('outputs_are_not_input_fixture', lambda r, c:
            next(x for x in c['contracts'] if x['input_builder']['kind'] == 'fixture')
            ['input_builder'].update(name='original'))
    if any(c['input_builder']['kind'] == 'expression' for c in registry['contracts']):
        reject('changed_input_expression', lambda r, c:
            next(x for x in c['contracts'] if x['input_builder']['kind'] == 'expression')
            ['input_builder'].update(ast_sha256='0' * 64))
    if len(registry['generated_fixtures']) > 1:
        dependency = generated_package(1)
        reject('unowned_second_generated_dependency', lambda r, c: metadata(r, lambda v:
            package(v, dependency).update(manifest_path='/tmp/replaced/Cargo.toml')))
        reject('changed_second_generated_source', lambda r, c: generated_source(r, c, 1))
    if extra_controls is not None:
        extra_controls(reject, original, registry)
    result = dict(complete=True, whole_goal_complete=False, controls=controls,
        execution_sha256=archive.digest(args.execution), registry_sha256=archive.digest(args.contracts))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
    print(len(controls), 'generated-model provenance/result controls reject; original receipt passes.')


def extra_controls(reject, original, registry):
    reject('changed_base_verifier', lambda r, c: c.update(base_verifier_sha256='0' * 64))
    reject('missing_source_recipe', lambda r, c: c['source_recipes'].pop())
    reject('private_fixture_is_not_public_execution', lambda r, c:
           c['source_recipes'][0].update(scope='public_query_execution'))
    reject('wrong_key_abi', lambda r, c: c['source_recipes'][0].update(abi_bits=32))
    reject('missing_key_slice', lambda r, c: c['source_recipes'][0]['pieces'].pop())
    reject('changed_key_source', lambda r, c:
           c['source_recipes'][0]['pieces'][0].update(source_sha256='0' * 64))
    reject('changed_compiled_key_source', lambda r, c:
           c['source_recipes'][0].update(compiled_source=''))
    reject('wrong_extracted_classifier', lambda r, c:
           c['source_recipes'][1].update(extracted_sha256='0' * 64))
    reject('missing_classifier_artifact', lambda r, c: c['artifact_fixtures'].clear())
    reject('classifier_artifact_is_not_compiled_sdk', lambda r, c:
           c['generated_fixtures'].append(c['artifact_fixtures'][0]))

    def custom_build(record, contract, remove):
        import json
        metadata = json.loads(record['project_metadata'])
        package = next(p for p in metadata['packages'] if p['name'] == contract['package'])
        target = next(t for t in package['targets'] if t['kind'] == ['custom-build'])
        if remove:
            package['targets'].remove(target)
        else:
            target['src_path'] = '/tmp/substituted_build.rs'
        record['project_metadata'] = json.dumps(metadata)
        record['steps'][record['metadata_step']]['stdout'] = record['project_metadata']

    reject('missing_custom_build_target', lambda r, c: custom_build(r, c, True))
    reject('substituted_custom_build_target', lambda r, c: custom_build(r, c, False))


if __name__ == "__main__":
    main(validator=validate, extra_controls=extra_controls)
