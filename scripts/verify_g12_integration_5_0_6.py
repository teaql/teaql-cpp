#!/usr/bin/env python3
"""Build and execute the finite G12 integration selection, preserving every corpus."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def load(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=['native', 'modules_native'], required=True)
    parser.add_argument('--stage', choices=['build', 'test'], required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    module = args.profile == 'modules_native'
    previous = load(ROOT / ('docs/parity/g12_integrated_module_5_0_6.json.gz' if module else
                            'docs/parity/g12_integrated_native_5_0_6.json.gz'))
    selection = previous['selection']
    tests = {row['name'] for row in selection['tests']}
    targets = set(selection['targets'])
    prefix = 'module_' if module else ''
    additions = [('business_id_environment_tests', 'business_id_environment_contracts'),
                 ('runtime_initialization_tests', 'runtime_initialization_contracts'),
                 ('generated_business_environment_probe', 'generated_business_environment_startup_rust_parity'),
                 ('generated_inherited_business_lifetime_tests', 'generated_inherited_business_lifetime_contracts')]
    for target, test in additions:
        targets.add('teaql_' + prefix + target)
        tests.add(prefix + test)
    if module:
        targets.add('teaql_module_generated_i18n_tests')
        tests.update(['module_generated_i18n_contracts', 'module_generated_i18n_rust_parity'])
    else:
        targets.add('teaql_i18n_tests')
        tests.update(['i18n_context_contracts', 'i18n_rust_parity'])
    assert len(tests) == (40 if module else 51)
    record = dict(scope='Final finite G12 integration; source review and other capabilities remain separate',
                  profile=args.profile, runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  tests=sorted(tests), targets=sorted(targets), steps=[], complete=False)
    if args.stage == 'build':
        assert not args.record.exists(), 'Preserve earlier evidence; select a new record'
    else:
        record = load(args.record)
        assert record['build_complete'] and not record.get('test_started')
        assert record['tests'] == sorted(tests) and record['targets'] == sorted(targets)

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))

    def run(name, command):
        result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        record['steps'].append(dict(name=name, command=command,
                                    exit_code=result.returncode, output=result.stdout))
        save()
        assert result.returncode == 0, (name, result.stdout[-7000:])
        print(name, 'exit zero', flush=True)
        return result.stdout

    if args.stage == 'build':
        run('configure', ['cmake', '--preset', args.profile])
        run('build', ['cmake', '--build', '--preset', args.profile, '--parallel', '4',
                      '--target', *sorted(targets)])
        registry = json.loads(run('registry', ['ctest', '--preset', args.profile,
                                               '--show-only=json-v1']))
        registered = {row['name']: row for row in registry['tests']}
        assert tests <= registered.keys()
        record['registered_tests'] = {name: registered[name] for name in sorted(tests)}
        record['executables'] = {}
        build = ROOT / ('build_modules_native' if module else 'build_native_current')
        for target in targets:
            path = build / ('modules' if module and target.startswith('teaql_module_') else '') / target
            if path.is_file():
                record['executables'][str(path.relative_to(ROOT))] = hashlib.sha256(path.read_bytes()).hexdigest()
        record['build_complete'] = True
        save()
    else:
        for name, expected in record['executables'].items():
            assert hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == expected, name
        record['test_started'] = True
        record['scheduling'] = ('Run the unchanged whole 2304-case bootstrap test alone under its original '
                                'timeout, then the remaining complete tests with two workers. No corpus '
                                'partition, timeout override or source/result normalization.')
        save()
        bootstrap = prefix + 'bootstrap_extensions_rust_parity'
        run('whole_bootstrap', ['ctest', '--preset', args.profile, '--output-on-failure',
                               '--parallel', '1', '-R', '^' + bootstrap + '$'])
        with tempfile.TemporaryDirectory(prefix='teaql_g12_test_selection_') as directory:
            names = Path(directory) / 'tests.txt'
            names.write_text('\n'.join(sorted(tests - {bootstrap})) + '\n')
            run('remaining_tests', ['ctest', '--preset', args.profile, '--output-on-failure',
                                    '--parallel', '2', '--tests-from-file', str(names)])
        for name, expected in record['executables'].items():
            assert hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == expected, name
        record['complete'] = True
        save()
        print(f'All {len(tests)} selected G12 tests passed in recorded stages.', flush=True)


if __name__ == '__main__':
    main()
