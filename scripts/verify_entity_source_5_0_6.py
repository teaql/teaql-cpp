#!/usr/bin/env python3
"""Qualify complete Entity trait responsibilities against the frozen core source."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same

MATRICES = [('entity_registration_api', 42), ('base_entity_api', 1481),
            ('entity_debug', 289), ('entity_defaults_api', 102), ('core_shape_presentation', 200)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=['native', 'modules_native', 'sanitize'], required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False, complete_generator_goal=False, profile=args.profile,
                  scope='Complete entity.rs public/default/protocol review; full caller/generator/G13/P7 acceptance separate', steps=[])

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, environment=None, payload=None):
        result = subprocess.run([str(x) for x in command], cwd=ROOT, env=environment, input=payload,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command], exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        print('exit', result.returncode, ' '.join(str(x) for x in command[:5]), flush=True)
        result.check_returncode()
        return result.stdout

    run(['python3', 'scripts/verify_upstream.py'])
    project = ROOT / 'tests/parity/rust_core_primitives_reference_5_0_6'
    original = ROOT / '.reference/teaql-rs'
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(original / 'Cargo.lock')
    record['reference_registry'] = dict(exact_versions_checksums=True, entries=len(registry_identities(project / 'Cargo.lock')))
    old_owned = ROOT / 'tests/parity/rust_values/src/bin/entity_owned_api_oracle.rs'
    new_owned = project / 'src/bin/entity_owned_api_reference_5_0_6.rs'
    assert old_owned.read_bytes() == new_owned.read_bytes()
    for stem, _ in MATRICES:
        wrapper = project / 'src/bin' / (stem + '_reference_5_0_6.rs')
        assert wrapper.read_text() == ('// Execute the unchanged historical observer against exact reference dependencies.\n'
                                       'include!("../../../rust_field_layout/src/bin/' + stem + '_5_0_6.rs");\n')
    rust_environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
                            CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    bins = ['entity_source_reference_5_0_6', 'entity_owned_api_reference_5_0_6'] + [stem + '_reference_5_0_6' for stem, _ in MATRICES]
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project / 'Cargo.toml',
         *[arg for name in bins for arg in ['--bin', name]]], rust_environment)
    run(['/tmp/teaql_cpp_core_baselines_506/debug/entity_owned_api_reference_5_0_6'], rust_environment)
    run(['cmake', '--preset', args.profile])
    module = args.profile == 'modules_native'
    prefix = 'teaql_module_' if module else 'teaql_'
    stems = ['entity_source_probe', 'entity_owned_api_tests', 'base_entity_api_tests', 'entity_debug_tests',
             'core_shape_presentation_tests', 'json_hydration_probe', 'context_reference_tests']
    names = ['entity_source_context_contracts', 'entity_owned_api_contracts', 'base_entity_api_contracts',
             'entity_debug_contracts', 'core_shape_presentation_contracts', 'json_hydration_context_contracts',
             'context_reference_contracts']
    if args.profile != 'sanitize':
        stems = ['entity_source_probe', 'entity_registration_api_tests', 'entity_registration_api_probe',
                 'base_entity_api_tests', 'base_entity_api_probe', 'entity_debug_tests', 'entity_debug_probe',
                 'entity_owned_api_tests', 'entity_defaults_api_probe', 'core_shape_presentation_tests',
                 'core_shape_presentation_probe', 'json_hydration_probe', 'request_trace_scope_tests']
        names = ['entity_source_context_contracts', 'entity_source_rust_parity',
                 'entity_registration_api_contracts', 'entity_registration_api_rust_parity',
                 'base_entity_api_contracts', 'base_entity_api_rust_parity', 'entity_debug_contracts',
                 'entity_debug_rust_parity', 'entity_owned_api_contracts', 'entity_defaults_api_rust_parity',
                 'core_shape_presentation_contracts', 'core_shape_presentation_rust_parity',
                 'json_hydration_context_contracts', 'request_trace_scope_contracts']
    stems.append('entity_expression_tests')
    names.append('entity_expression_contracts')
    targets = [prefix + stem for stem in stems]
    if module:
        targets.append('teaql_module_contracts')
        names.append('import_contracts')
    elif args.profile == 'native':
        targets.append('teaql_generated_query_expression')
        names.append('example_generated_query_expression')
    run(['cmake', '--build', '--preset', args.profile, '--parallel', '2', '--target', *targets])
    environment = dict(os.environ)
    if args.profile == 'sanitize':
        environment.update(ASAN_OPTIONS='detect_leaks=0:abort_on_error=1', UBSAN_OPTIONS='halt_on_error=1')
    record['environment'] = {key: environment[key] for key in ['ASAN_OPTIONS', 'UBSAN_OPTIONS'] if key in environment}
    pattern = '^(' + '|'.join(('module_' if module else '') + name for name in names) + ')$'
    output = run(['ctest', '--preset', args.profile, '--output-on-failure', '-R', pattern], environment)
    assert re.search(rf'100% tests passed out of {len(names)}\b', output)
    record['passed'] = len(names)
    build = ROOT / {'native': 'build_native_current', 'modules_native': 'build_modules_native',
                    'sanitize': 'build_sanitize_current'}[args.profile]
    binary_root = build / 'modules' if module else build
    rust_root = Path('/tmp/teaql_cpp_core_baselines_506/debug')
    comparison = args.record.with_name(args.record.name.replace('.json.gz', '_source.json.gz'))
    run(['python3', 'tests/parity/compare_entity_source_5_0_6.py', '--cpp', binary_root / (prefix + 'entity_source_probe'),
         '--rust', rust_root / 'entity_source_reference_5_0_6', '--record', comparison], environment)
    # The sanitizer profile also needs real source comparisons, not uninstrumented observer binaries.
    # Its additional probes are built only after the first terminal contract selection.
    if args.profile == 'sanitize':
        targets += [prefix + stem + '_probe' for stem, _ in MATRICES]
        run(['cmake', '--build', '--preset', args.profile, '--parallel', '2', '--target', *targets])
    for stem, count in MATRICES:
        path = ROOT / 'tests/parity' / (stem + '_5_0_6_cases.json.gz')
        if not path.exists():
            path = path.with_suffix('')
        raw = path.read_bytes()
        corpus = json.loads(gzip.decompress(raw) if path.suffix == '.gz' else raw)
        assert corpus['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
        assert len(corpus['cases']) == count
        inputs = [item['input'] for item in corpus['cases']]
        expected = [item['expected'] for item in corpus['cases']]
        payload = ''.join(json.dumps(item) + '\n' for item in inputs)
        for binary in [rust_root / (stem + '_reference_5_0_6'), binary_root / (prefix + stem + '_probe')]:
            output = run([binary], environment, payload)
            assert output.endswith('\n')
            actual = [json.loads(line) for line in output.split('\n')[:-1]]
            assert same(actual, expected), (stem, str(binary))
    assert 'no work to do' in run(['cmake', '--build', '--preset', args.profile, '--parallel', '2', '--target', *targets])
    paths = [ROOT / 'teaql_core/include/teaql/core/entity.hpp', ROOT / 'teaql_core/src/lib.cppm',
             ROOT / 'modules/tests/teaql_core_lookup.cpp', ROOT / 'docs/module_exports.json',
             ROOT / 'tests/parity/entity_source_probe.cpp', ROOT / 'tests/parity/compare_entity_source_5_0_6.py',
             ROOT / 'tests/parity/entity_source_5_0_6_cases.json.gz', original / 'teaql-core/src/entity.rs',
             project / 'Cargo.toml', project / 'Cargo.lock', Path(__file__).resolve(), comparison]
    paths += [project / 'src/bin' / (name + '.rs') for name in bins]
    paths += [ROOT / 'tests/parity/rust_field_layout/src/bin' / (stem + '_5_0_6.rs') for stem, _ in MATRICES]
    paths += [binary_root / (prefix + stem + '_probe') for stem, _ in MATRICES]
    paths += [binary_root / (prefix + 'entity_source_probe')]
    record['hashes'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    record.update(complete=True, observations=2344, source_corpora=6,
                  original_nonclone_entity_audited_boxed=True)
    save()
    print('Qualified', args.profile, len(names), 'contracts;2344 complete live original/native observations')


if __name__ == '__main__':
    main()
