#!/usr/bin/env python3
"""Qualify complete core telemetry through unchanged source and canonical consumers."""
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh receipt; retain historical evidence')
    record = dict(complete=False, complete_generator_goal=False,
                  scope='Complete core telemetry.rs; Context callers and external exporters separate',
                  steps=[], profiles=[], matrices=[])

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, environment=None, payload=None):
        result = subprocess.run([str(x) for x in command], cwd=ROOT, env=environment,
                                input=payload, text=True, capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command], exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        print('exit', result.returncode, ' '.join(str(x) for x in command[:6]), flush=True)
        result.check_returncode()
        return result.stdout

    def observe(binary, values):
        text = run([binary.resolve()], dict(os.environ, TEAQL_LOG_ENDPOINT='off'),
                   ''.join(json.dumps(value, ensure_ascii=True) + '\n' for value in values))
        assert text.endswith('\n')
        result = [json.loads(line) for line in text.split('\n')[:-1]]
        assert len(result) == len(values)
        return result

    run(['python3', 'scripts/verify_upstream.py'])
    project = ROOT / 'tests/parity/rust_telemetry_reference_5_0_6'
    reference = ROOT / '.reference/teaql-rs'
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock')
    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project / 'Cargo.toml', '--bins'], environment)
    output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', project / 'Cargo.toml', '--lib'], environment)
    assert '3 passed; 0 failed' in output
    record['original_unit_tests'] = 3
    target = Path(environment['CARGO_TARGET_DIR']) / 'debug'
    paths = [reference / 'teaql-runtime/src/telemetry.rs', reference / 'Cargo.lock',
             project / 'Cargo.toml', project / 'Cargo.lock', project / 'lib.rs',
             ROOT / 'modules/contracts.json', ROOT / 'CMakePresets.json', Path(__file__).resolve()]
    source = reference / 'teaql-runtime/src/telemetry.rs'
    old = subprocess.check_output(['git', '-C', reference, 'show',
                                  '62940ae2b88e7acc32763853b8b4685a01c7e90a:teaql-runtime/src/telemetry.rs'])
    assert source.read_bytes() == old
    record['old_current_source_identical'] = True
    matrices = []
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    for stem in ['telemetry', 'telemetry_model_scope']:
        corpus = ROOT / 'tests/parity' / (stem + '_baseline_5_0_6_cases.json.gz')
        matrix = json.loads(gzip.decompress(corpus.read_bytes()))
        assert matrix['baseline'] == baseline
        inputs = [case['input'] for case in matrix['cases']]
        expected = [case['expected'] for case in matrix['cases']]
        binary = target / (stem + '_oracle')
        original = observe(binary, inputs)
        assert same(original, expected), stem
        matrices.append((stem, inputs, expected))
        record['matrices'].append(dict(name=stem, observations=len(inputs),
                                       complete_original_outputs=original))
        paths += [corpus, binary, ROOT / 'tests/parity/rust_values/src/bin' / (stem + '_oracle.rs')]
    owned = [dict(propagation=propagation, text=text) for propagation in [False, True]
             for text in ['', 'owned completion', 'unicode \u03bc\U0001f9ca', 'x' * 4096]]
    owned_binary = target / 'telemetry_owned_output_oracle'
    observed = observe(owned_binary, owned)
    assert observed == [dict(text=value['text']) for value in owned]
    record['original_nonclone_outputs'] = dict(inputs=owned, outputs=observed, calls=len(owned),
        boundary='Source generic result compilation/execution; C++ ownership assertions are separate canonical tests')
    paths += [owned_binary, ROOT / 'tests/parity/rust_values/src/bin/telemetry_owned_output_oracle.rs']
    for profile in ['native', 'modules_native']:
        module = profile == 'modules_native'
        if module:
            run(['cmake', '--preset', profile])
        prefix = 'teaql_module_' if module else 'teaql_'
        targets = [prefix + stem for stem in ['telemetry_tests', 'telemetry_probe', 'telemetry_model_scope_probe']]
        tests = ['telemetry_contracts', 'telemetry_rust_parity', 'telemetry_model_scope_rust_parity']
        if not module:
            targets.append('teaql_original_log_telemetry_tests')
            tests.append('original_runtime_log_telemetry_contracts')
        run(['cmake', '--build', '--preset', profile, '--parallel', '2', '--target', *targets])
        pattern = '^(' + '|'.join(('module_' if module else '') + name for name in tests) + ')$'
        result = run(['ctest', '--preset', profile, '--output-on-failure', '-R', pattern])
        assert re.search(rf'100% tests passed out of {len(tests)}\b', result)
        build = ROOT / ('build_modules_native/modules' if module else 'build_native_current')
        for stem, inputs, expected in matrices:
            binary = build / (prefix + stem + '_probe')
            assert same(observe(binary, inputs), expected), (profile, stem)
            paths.append(binary)
        assert 'no work to do' in run(['cmake', '--build', '--preset', profile, '--parallel', '2', '--target', *targets])
        record['profiles'].append(dict(name=profile, passed=len(tests), observations=1142))
    run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', 'build_modules_native'])
    paths += [ROOT / name for name in ['teaql_runtime/include/teaql/runtime/telemetry.hpp',
             'teaql_runtime/include/teaql/runtime/telemetry_debug.hpp', 'teaql_runtime/src/telemetry.cpp',
             'teaql_runtime/tests/telemetry_tests.cpp', 'tests/parity/telemetry_probe.cpp',
             'tests/parity/telemetry_model_scope_probe.cpp', 'tests/parity/teaql_runtime/log_telemetry.cpp',
             'tests/support/move_only_error_fixture.hpp']]
    record['hashes'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    record.update(complete=True, observations=1142, exact_reference_registry_identities=True,
                  normalization='none', original_sources_modified=False, production_cpp_modified=False)
    save()
    print('Complete core telemetry source gates pass; broader capabilities remain open', flush=True)


if __name__ == '__main__':
    main()
