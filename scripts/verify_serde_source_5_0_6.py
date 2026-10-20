#!/usr/bin/env python3
"""Qualify the complete trimming source using retained full observations and ownership tests."""
import argparse
from collections import Counter
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
    parser.add_argument('--profile', choices=['native', 'modules_native'], required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False, complete_generator_goal=False, profile=args.profile,
                  scope='Complete serde_utils.rs JSON adapter responsibility; other presentation sources and whole phases separate', steps=[])

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, environment=None, payload=None):
        result = subprocess.run([str(x) for x in command], cwd=ROOT, env=environment,
                                input=payload, text=True, capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command], exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        print('exit', result.returncode, ' '.join(str(x) for x in command[:5]), flush=True)
        result.check_returncode()
        return result.stdout

    run(['python3', 'scripts/verify_upstream.py'])
    project = ROOT / 'tests/parity/rust_core_baselines_5_0_6'
    reference = ROOT / '.reference/teaql-rs'
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock')
    record['exact_reference_registry_identities'] = True
    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    original = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                    reference / 'teaql-core/Cargo.toml', '--lib', 'serde_utils::tests::'], environment)
    assert '4 passed; 0 failed' in original
    environment['CARGO_TARGET_DIR'] = '/tmp/teaql_cpp_core_baselines_506'
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project / 'Cargo.toml',
         '--bin', 'presentation_api_oracle'], environment)
    module = args.profile == 'modules_native'
    prefix = 'teaql_module_' if module else 'teaql_'
    stems = ['presentation_api_probe']
    tests = ['presentation_api_rust_parity']
    if not module:
        stems += ['value_model_tests', 'original_core_functional_tests']
        tests += ['value_model_contracts', 'original_core_functional_contracts']
    targets = [prefix + stem for stem in stems]
    run(['cmake', '--build', '--preset', args.profile, '--parallel', '2', '--target', *targets])
    pattern = '^(' + '|'.join(('module_' if module else '') + name for name in tests) + ')$'
    output = run(['ctest', '--preset', args.profile, '--output-on-failure', '-R', pattern])
    assert re.search(rf'100% tests passed out of {len(tests)}\b', output)
    record['passed'] = len(tests)
    corpus = ROOT / 'tests/parity/presentation_api_baseline_5_0_6_cases.json.gz'
    matrix = json.loads(gzip.decompress(corpus.read_bytes()))
    assert matrix['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert len(matrix['cases']) == 675
    assert Counter(case['input']['kind'] for case in matrix['cases'])['trim'] == 145
    inputs = ''.join(json.dumps(case['input'], ensure_ascii=True) + '\n' for case in matrix['cases'])
    build = ROOT / ('build_modules_native' if module else 'build_native_current')
    cpp = build / ('modules' if module else '') / (prefix + 'presentation_api_probe')
    rust = Path('/tmp/teaql_cpp_core_baselines_506/debug/presentation_api_oracle')
    for binary in [rust, cpp]:
        stdout = run([binary.resolve()], dict(os.environ, TEAQL_LOG_ENDPOINT='off'), inputs)
        assert stdout.endswith('\n')
        actual = [json.loads(line) for line in stdout.split('\n')[:-1]]
        assert same(actual, [case['expected'] for case in matrix['cases']])
    assert 'no work to do' in run(['cmake', '--build', '--preset', args.profile, '--parallel', '2', '--target', *targets])
    paths = [reference / 'teaql-core/src/serde_utils.rs', ROOT / 'teaql_core/include/teaql/core/serde_utils.hpp',
             ROOT / 'teaql_core/include/teaql/core/detail/text.hpp', ROOT / 'teaql_core/tests/value_model_tests.cpp',
             ROOT / 'tests/parity/teaql_core/functional_contracts.cpp', ROOT / 'tests/parity/presentation_api_probe.cpp',
             ROOT / 'tests/parity/rust_values/src/bin/presentation_api_oracle.rs', project / 'Cargo.toml',
             project / 'Cargo.lock', reference / 'Cargo.lock', corpus, rust, cpp, Path(__file__).resolve()]
    record['hashes'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    record.update(complete=True, complete_source_functions=4, original_unit_tests=4,
                  complete_observations=675, trimming_observations=145)
    save()
    print('Qualified', args.profile, len(tests), 'checks;675 complete observations including145 trimming cases and4 original tests')


if __name__ == '__main__':
    main()
