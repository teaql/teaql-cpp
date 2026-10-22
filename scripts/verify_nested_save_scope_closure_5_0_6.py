#!/usr/bin/env python3
"""Run focused native, module and lifetime consumers for nested Save scopes."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
PROFILES = {
    'native': ('build_native_current', 'teaql_', [
        'graph_fix_tests', 'graph_fix_probe', 'audited_auto_save_tests',
        'audited_auto_save_probe', 'audited_save_tests', 'audited_save_probe'], [
        'graph_fix_context_contracts', 'graph_fix_rust_parity',
        'audited_auto_save_context_contracts', 'audited_auto_save_rust_parity',
        'audited_save_context_contracts', 'audited_save_rust_parity']),
    'modules_native': ('build_modules_native', 'teaql_module_', [
        'graph_fix_tests', 'graph_fix_probe'], [
        'module_graph_fix_context_contracts', 'module_graph_fix_rust_parity']),
    'sanitize': ('build_sanitize_current', 'teaql_', [
        'graph_fix_tests', 'audited_auto_save_tests'], [
        'graph_fix_context_contracts', 'audited_auto_save_context_contracts']),
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inputs():
    paths = {ROOT / relative for relative in (
        'CMakePresets.json', 'tests/CMakeLists.txt', 'modules/CMakeLists.txt',
        'modules/contracts.json', 'upstream.lock.json', 'toolchains.lock.json',
        'tests/support/nested_save_scope_fixture.hpp',
        'tests/support/audited_save_fixture.hpp',
        'tests/support/checker_fixture.hpp',
        'teaql_runtime/tests/graph_fix_tests.cpp',
        'teaql_provider_sqlite/tests/audited_auto_save_tests.cpp')}
    for base in ('teaql_runtime/include', 'teaql_runtime/src'):
        paths.update(p for p in (ROOT / base).rglob('*') if p.is_file())
    paths.add(Path(__file__).resolve())
    return {str(p.relative_to(ROOT)): digest(p) for p in sorted(paths)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=PROFILES, required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        raise SystemExit('Preserve the existing receipt')
    directory, prefix, targets, tests = PROFILES[args.profile]
    env = dict(os.environ, TEAQL_LOG_ENDPOINT='off')
    if args.profile == 'sanitize':
        env.update(ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
                   UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    receipt = dict(complete=False, profile=args.profile, steps=[], tests=tests,
                   sources=inputs(), controller=Path(__file__).read_text(),
                   sanitizer_leak_scan=False, source_promoted=False,
                   free_bytes=os.statvfs(ROOT).f_bavail * os.statvfs(ROOT).f_frsize)

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))

    def run(command):
        command = list(map(str, command))
        print('Running:', ' '.join(command), flush=True)
        result = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True)
        receipt['steps'].append(dict(command=command, exit_code=result.returncode,
                                     stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix='teaql_nested_scope_gate_') as temporary:
            run(['cmake', '--build', '--preset', args.profile, '--target',
                 *[prefix + name for name in targets], '-j', '2'])
            pattern = '^(' + '|'.join(map(re.escape, tests)) + ')$'
            selection = json.loads(run(['ctest', '--test-dir', directory,
                                        '--show-only=json-v1', '-R', pattern]))
            assert sorted(t['name'] for t in selection['tests']) == sorted(tests)
            junit = Path(temporary) / 'tests.xml'
            run(['ctest', '--test-dir', directory, '-R', pattern,
                 '--output-on-failure', '--output-junit', junit])
            receipt['junit'] = junit.read_text()
            cases = list(ET.fromstring(receipt['junit']).iter('testcase'))
            assert sorted(c.attrib['name'] for c in cases) == sorted(tests)
            assert all(c.find('failure') is None and c.find('error') is None and
                       c.find('skipped') is None for c in cases)
            receipt['tests_passed'] = len(cases)
            no_work = run(['cmake', '--build', '--preset', args.profile, '--target',
                           *[prefix + name for name in targets], '-j', '2'])
            assert 'ninja: no work to do.' in no_work
            if args.profile == 'modules_native':
                run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', directory])
            run(['python3', 'scripts/verify_upstream.py'])
            run(['python3', 'scripts/verify_module_paths.py'])
            assert inputs() == receipt['sources'], 'Inputs changed during this gate'
            receipt['disposable_bytes_removed'] = sum(p.stat().st_size for p in
                Path(temporary).rglob('*') if p.is_file())
        receipt['disposable_outputs_removed'] = True
        receipt['complete'] = True
    except BaseException as error:
        receipt['error'] = repr(error)
        raise
    finally:
        save()
    print(f'{args.profile}: {len(tests)}/{len(tests)} passed; stable inputs', flush=True)


if __name__ == '__main__':
    main()
