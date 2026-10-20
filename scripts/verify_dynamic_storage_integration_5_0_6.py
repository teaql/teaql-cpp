#!/usr/bin/env python3
"""Build and run the finite G16 definitions/provider/store/stream/Save selection."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
NATIVE = '''dynamic_fields_contracts dynamic_fields_rust_parity
dynamic_carrier_contracts dynamic_carrier_rust_parity
readonly_schema_hash_5_0_6_rust_parity generated_dynamic_field_contracts
generated_dynamic_carrier_rust_parity generated_readonly_property_contracts
generated_readonly_properties_rust_parity dynamic_storage_contracts
dynamic_storage_rust_parity sqlite_dynamic_storage_contracts
sqlite_dynamic_storage_rust_parity sqlite_dynamic_storage_boundaries_rust_parity
dynamic_provider_contracts dynamic_provider_rust_parity dynamic_ledger_contracts
dynamic_ledger_rust_parity dynamic_preflight_contracts dynamic_governance_rust_parity
dynamic_graph_save_contracts dynamic_graph_debug_rust_parity
dynamic_typed_save_audit_rust_parity dynamic_save_audit_rust_parity
dynamic_audit_projection_rust_parity commit_audit_transaction_contracts
transaction_scope_context_contracts dynamic_query_contracts dynamic_query_rust_parity
dynamic_relation_contracts dynamic_relation_rust_parity
generated_dynamic_json_privacy_rust_parity generated_dynamic_json_privacy_sink_contracts
generated_dynamic_json_facets_rust_parity generated_dynamic_json_persistent_rust_parity
generated_dynamic_json_composition_rust_parity generated_dynamic_json_inherited_sql_rust_parity
generated_dynamic_json_domains_rust_parity generated_dynamic_json_scalar_rust_parity
generated_dynamic_json_inheritance_rust_parity dynamic_json_execution_rust_parity
dynamic_json_policy_rust_parity graph_saver_context_contracts
generated_json_hydration_rust_parity loaded_row_graph_context_contracts
relation_graph_save_rust_parity'''.split()
# Their shared mechanics and imported generated carrier contracts are included.
# These two model-specific observers are qualified by the native source gate;
# neither has a canonical imported observer registration at this checkpoint.
NATIVE_ONLY = {'generated_dynamic_carrier_rust_parity',
               'generated_readonly_properties_rust_parity'}
MODULE = ['module_' + name for name in NATIVE if name not in NATIVE_ONLY]
MODULE.append('module_import_contracts')


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
    assert len(names) == len(set(names))
    build = ROOT / ('build_modules_native' if module else 'build_native_current')
    record = dict(scope='Finite DS01-DS16 integration. Source/model acceptance and '
                        'broader G14/G17 privacy/provenance remain separate obligations.',
                  profile=args.profile, tests=names, steps=[], complete=False,
                  runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'])
    if args.stage == 'build':
        assert not args.record.exists(), 'Preserve evidence; select a fresh record'
    else:
        record = json.loads(gzip.decompress(args.record.read_bytes()))
        assert record['build_complete'] and not record.get('test_started')
        assert record['profile'] == args.profile and record['tests'] == names

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
        registry = json.loads(run('registry', ['ctest', '--preset', args.profile,
                                             '--show-only=json-v1']))
        registered = {test['name']: test for test in registry['tests']}
        assert set(names) <= registered.keys(), set(names) - registered.keys()
        # CTest omits a direct command when its executable has not been built.
        # Resolve only canonical contracts, then confirm the actual registry
        # commands after a successful build. Comparator commands already exist
        # because their command is Python, even before their probes are built.
        contracts = json.loads((ROOT / 'modules/contracts.json').read_text())['contracts']
        record['prebuild_missing_commands'] = []
        for name in names:
            if 'command' in registered[name]:
                continue
            case_name = name.removeprefix('module_') if module else name
            candidates = [(contract, case) for contract in contracts
                          for case in contract.get('cases', []) if case['name'] == case_name]
            assert len(candidates) == 1, (name, candidates)
            contract, case = candidates[0]
            prefix = 'teaql_module_' if module else 'teaql_'
            directory = build / 'modules' if module else build
            registered[name]['command'] = [str(directory / (prefix + contract['stem'])),
                                            *case['arguments']]
            record['prebuild_missing_commands'].append(name)
        record['registered_tests'] = {name: registered[name] for name in names}
        binaries = set()
        for name in names:
            for argument in registered[name]['command']:
                path = Path(argument)
                if path.parent in (build, build / 'modules') and path.name.startswith('teaql_'):
                    binaries.add(path)
        targets = sorted(path.name for path in binaries)
        record['targets'] = targets
        run('build', ['cmake', '--build', '--preset', args.profile, '--parallel', '2',
                      '--target', *targets])
        actual = json.loads(run('built_registry', ['ctest', '--preset', args.profile,
                                                  '--show-only=json-v1']))
        actual = {test['name']: test for test in actual['tests']}
        for name in names:
            assert actual[name]['command'] == registered[name]['command'], name
        record['registered_tests'] = {name: actual[name] for name in names}
        record['executables'] = {str(path.relative_to(ROOT)): digest(path)
                                 for path in sorted(binaries)}
        record['build_complete'] = True
        save()
    else:
        for name, expected in record['executables'].items():
            assert digest(ROOT / name) == expected, name
        record['test_started'] = True
        save()
        with tempfile.TemporaryDirectory(prefix='teaql_dynamic_storage_integration_') as directory:
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
