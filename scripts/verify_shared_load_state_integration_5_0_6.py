#!/usr/bin/env python3
"""Build and run the finite G15 kernel/model/composition acceptance selection."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
NATIVE = '''field_layout_contracts field_layout_rust_parity
loaded_snapshot_contracts loaded_snapshot_rust_parity
dynamic_carrier_contracts dynamic_carrier_rust_parity original_snapshot_rust_parity
initialized_snapshot_cache_rust_parity readonly_schema_hash_5_0_6_rust_parity
compact_row_api_contracts compact_row_api_rust_parity generated_api_contracts
generated_dynamic_field_contracts generated_dynamic_carrier_rust_parity
generated_readonly_property_contracts generated_readonly_properties_rust_parity
entity_helper_entries_context_contracts entity_helper_entries_rust_parity
generated_entity_helpers_rust_parity generated_optional_fields_rust_parity
entity_constructor_helpers_rust_parity recursive_attachment_helpers_rust_parity
generated_checker_contracts checker_context_contracts generated_checker_flow_rust_parity
generated_checker_commit_rust_parity entity_runtime_contracts entity_runtime_model_contracts
entity_runtime_model_rust_parity model_debug_contracts model_debug_rust_parity
typed_aggregate_context_contracts typed_aggregate_scope_rust_parity
loaded_row_graph_context_contracts loaded_row_graph_rust_parity
dynamic_typed_save_audit_rust_parity generated_json_hydration_rust_parity
generated_json_column_alias_rust_parity generated_dynamic_json_facets_rust_parity
generated_api_type_errors example_generated_query_expression'''.split()
MODULE = '''module_field_layout_contracts module_field_layout_rust_parity
module_loaded_snapshot_contracts module_loaded_snapshot_rust_parity
module_dynamic_carrier_contracts module_dynamic_carrier_rust_parity
module_original_snapshot_rust_parity module_initialized_snapshot_cache_rust_parity
module_readonly_schema_hash_5_0_6_rust_parity module_compact_row_api_contracts
module_compact_row_api_rust_parity module_generated_dynamic_field_contracts
module_generated_readonly_property_contracts module_entity_helper_entries_context_contracts
module_entity_helper_entries_rust_parity module_entity_constructor_helpers_rust_parity
module_recursive_attachment_helpers_rust_parity module_entity_runtime_model_contracts
module_entity_runtime_rust_parity module_model_debug_contracts
module_typed_aggregate_context_contracts module_typed_aggregate_scope_rust_parity
module_loaded_row_graph_context_contracts module_loaded_row_graph_rust_parity
module_dynamic_typed_save_audit_rust_parity module_generated_json_hydration_rust_parity
module_generated_json_column_alias_rust_parity module_generated_dynamic_json_facets_rust_parity
module_generated_query_expression'''.split()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=['native', 'modules_native'], required=True)
    parser.add_argument('--stage', choices=['build', 'test'], required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    module = args.profile == 'modules_native'
    names = sorted(MODULE if module else NATIVE)
    assert len(names) == len(set(names)) == (29 if module else 41)
    build = ROOT / ('build_modules_native' if module else 'build_native_current')
    record = dict(scope='Finite G15 LS01-LS14 relevant integration; complete source/model '
                        'review remains a separate acceptance obligation',
                  profile=args.profile, tests=names, steps=[], complete=False,
                  runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'])
    if args.stage == 'build':
        assert not args.record.exists(), 'Preserve existing evidence; choose a fresh record'
    else:
        record = json.loads(gzip.decompress(args.record.read_bytes()))
        assert record['build_complete'] and not record.get('test_started')
        assert record['tests'] == names and record['profile'] == args.profile

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))

    def run(name, command):
        result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        record['steps'].append(dict(name=name, command=command, exit_code=result.returncode,
                                    output=result.stdout))
        save()
        print(name, 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout

    if args.stage == 'build':
        run('configure', ['cmake', '--preset', args.profile])
        registry = json.loads(run('registry', ['ctest', '--preset', args.profile, '--show-only=json-v1']))
        registered = {test['name']: test for test in registry['tests']}
        assert set(names) <= registered.keys(), set(names) - registered.keys()
        record['registered_tests'] = {name: registered[name] for name in names}
        binaries = set()
        for name in names:
            for argument in registered[name]['command']:
                path = Path(argument)
                if path.parent in (build, build / 'modules'):
                    if path.name.startswith('teaql_') or path.name.startswith('libteaql_'):
                        binaries.add(path)
        targets = sorted({p.name.removeprefix('lib').removesuffix('.so') for p in binaries})
        record['targets'] = targets
        run('build', ['cmake', '--build', '--preset', args.profile, '--parallel', '2', '--target', *targets])
        record['executables'] = {str(p.relative_to(ROOT)): digest(p) for p in sorted(binaries)}
        record['build_complete'] = True
        save()
    else:
        for name, expected in record['executables'].items():
            assert digest(ROOT / name) == expected, name
        record['test_started'] = True
        save()
        with tempfile.TemporaryDirectory(prefix='teaql_shared_state_tests_') as directory:
            path = Path(directory) / 'tests.txt'
            path.write_text('\n'.join(names) + '\n')
            run('tests', ['ctest', '--preset', args.profile, '--output-on-failure',
                           '--parallel', '2', '--tests-from-file', str(path)])
        for name, expected in record['executables'].items():
            assert digest(ROOT / name) == expected, name
        record['complete'] = True
        save()


if __name__ == '__main__':
    main()
