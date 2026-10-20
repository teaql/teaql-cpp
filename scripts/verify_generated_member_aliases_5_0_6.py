#!/usr/bin/env python3
"""Build and qualify generated alias adapters with current G15 integration."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
from verify_shared_load_state_integration_5_0_6 import NATIVE, MODULE

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=['native', 'modules_native', 'sanitize'], required=True)
    parser.add_argument('--stage', choices=['build', 'test'], required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    module = args.profile == 'modules_native'
    prefix = 'module_' if module else ''
    if args.profile == 'sanitize':
        names = ['generated_member_alias_contracts', 'runtime_entity_expression_contracts',
                 'context_reference_contracts']
    else:
        names = list(MODULE if module else NATIVE)
        names += [prefix + x for x in ['generated_member_alias_contracts',
                                      'generated_member_alias_rust_parity',
                                      'runtime_entity_expression_contracts']]
    names = sorted(names)
    assert len(names) == len(set(names))
    build = ROOT / {'native':'build_native_current', 'modules_native':'build_modules_native',
                    'sanitize':'build_sanitize_current'}[args.profile]
    if args.stage == 'build':
        if args.record.exists():
            parser.error('Use a fresh record')
        record = dict(scope='Thin generated alias adapter and complete finite G15 integration selection',
                      complete=False, profile=args.profile, tests=names, steps=[])
    else:
        record = json.loads(gzip.decompress(args.record.read_bytes()))
        assert record['build_complete'] and not record.get('test_started')
        assert record['profile'] == args.profile and record['tests'] == names

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, environment=None):
        result = subprocess.run([str(x) for x in command], cwd=ROOT, text=True, env=environment,
                                capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command], exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        print('exit', result.returncode, ' '.join(str(x) for x in command[:5]), flush=True)
        result.check_returncode()
        return result.stdout

    if args.stage == 'build':
        run(['cmake', '--preset', args.profile])
        registry = json.loads(run(['ctest', '--preset', args.profile, '--show-only=json-v1']))
        registered = {t['name']:t for t in registry['tests']}
        assert set(names) <= registered.keys(), set(names)-registered.keys()
        record['registry_count'] = len(registered)
        contracts = json.loads((ROOT / 'modules/contracts.json').read_text())['contracts']
        binaries = set()
        for name in names:
            test = registered[name]
            if 'command' not in test:
                original = name.removeprefix('module_') if module else name
                choices = [c for c in contracts if any(case['name']==original for case in c.get('cases', []))]
                assert len(choices)==1, name
                test['command'] = [str(build / ('modules' if module else '') /
                                       (('teaql_module_' if module else 'teaql_')+choices[0]['stem']))]
            for argument in test['command']:
                path = Path(argument)
                if path.parent in (build, build / 'modules') and path.name.startswith(('teaql_', 'libteaql_')):
                    binaries.add(path)
        record['registered_tests'] = {name:registered[name] for name in names}
        record['targets'] = sorted({p.name.removeprefix('lib').removesuffix('.so') for p in binaries})
        run(['cmake','--build','--preset',args.profile,'--parallel','2','--target',*record['targets']])
        record['executables'] = {str(p):digest(p) for p in sorted(binaries)}
        record['build_complete'] = True
        save()
        return
    for name, expected in record['executables'].items():
        assert digest(Path(name))==expected, name
    record['test_started'] = True
    save()
    environment = dict(os.environ)
    if args.profile == 'sanitize':
        environment.update(ASAN_OPTIONS='detect_leaks=0:abort_on_error=1', UBSAN_OPTIONS='halt_on_error=1')
        record['sanitizer_environment'] = {key:environment[key] for key in ['ASAN_OPTIONS','UBSAN_OPTIONS']}
    with tempfile.TemporaryDirectory(prefix='teaql_generated_member_alias_') as directory:
        selection = Path(directory) / 'tests.txt'
        selection.write_text('\n'.join(names)+'\n')
        output = run(['ctest','--preset',args.profile,'--output-on-failure','--parallel','2',
                      '--tests-from-file',selection], environment)
    if not re.search(rf'100% tests passed out of {len(names)}\b', output):
        raise RuntimeError('Incomplete integration result')
    record['passed'] = len(names)
    rust_environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506',
                            CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    run(['cargo','build','--offline','--locked','--manifest-path','tests/parity/rust_field_layout/Cargo.toml',
         '--bin','generated_member_alias_5_0_6'], rust_environment)
    cpp = build / ('modules' if module else '') / (('teaql_module_' if module else 'teaql_')+'generated_member_alias_probe')
    comparison = args.record.with_name(args.record.name.replace('.json.gz','_source.json.gz'))
    run(['python3','tests/parity/compare_generated_member_aliases_5_0_6.py','--cpp',cpp,
         '--rust','/tmp/teaql_cpp_field_layout_rust_506/debug/generated_member_alias_5_0_6',
         '--record',comparison], environment)
    record['actual_source_comparison'] = dict(path=str(comparison), sha256=digest(comparison), count=260)
    output = run(['cmake','--build','--preset',args.profile,'--parallel','2','--target',*record['targets']])
    if 'no work to do' not in output:
        raise RuntimeError('Build inputs changed during qualification')
    for name, expected in record['executables'].items():
        assert digest(Path(name))==expected, name
    if args.profile == 'native':
        log = build / 'Testing/Temporary/LastTest.log'
        record['ctest_terminal_log'] = log.read_text()
        contracts = re.findall(r'Verified (\d+) positive and (\d+) negative generated API compiler contracts', log.read_text())
        assert len(contracts)==1, 'Missing terminal full compiler contract result'
        record['compiler'] = dict(positive=int(contracts[0][0]), negative=int(contracts[0][1]))
    record['complete'] = True
    save()
    print('Qualified', args.profile, len(names), 'checks and260 actual generated source outcomes', flush=True)


if __name__ == '__main__':
    main()
