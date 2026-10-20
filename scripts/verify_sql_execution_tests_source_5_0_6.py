#!/usr/bin/env python3
"""Replay all locked SQL query/mutation diagnostic tests and their semantic ports."""
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

from reference_sql_view_5_0_6 import ReferenceSqlView

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
FAMILIES = (
    ('execution_tests.rs', 'diagnostic_tests', 'original_query_execution_tests', 6),
    ('mutation_execution_tests.rs', 'mutation_diagnostic_tests', 'original_mutation_execution_tests', 21),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--profiles', nargs='+', choices=('native', 'modules_native', 'sanitize'),
                        default=['native', 'modules_native'])
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False, full_goal_accepted=False, whole_executor_accepted=False,
                  steps=[], original_families=[], profiles={})
    env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_diagnostics_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
               TEAQL_LOG_ENDPOINT='off', ASAN_OPTIONS='detect_leaks=0',
               UBSAN_OPTIONS='halt_on_error=1')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command):
        command = [str(v) for v in command]
        result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    run(['python3', 'scripts/verify_upstream.py'])
    with tempfile.TemporaryDirectory(prefix='teaql_sql_execution_tests_506_') as temporary:
        # Existing access-only diagnostic observer is inert in these original tests.
        # No test, production source prefix, or original dependency is removed.
        view = ReferenceSqlView(ROOT, Path(temporary) / 'sql',
                                PARITY / 'sql_diagnostics_source_observer_5_0_6.rs', run)
        command = ['cargo', 'test', '--offline', '--locked', '--manifest-path',
                   view.directory / 'Cargo.toml', '--lib']
        registry = run(command + ['--', '--list'])
        names = re.findall(r'^([^ ]+): test$', registry, re.M)
        for source, module, stem, count in FAMILIES:
            expected = re.findall(r'#\[(?:tokio::)?test\]\s*(?:async\s+)?fn\s+(\w+)',
                                  (view.source / 'src' / source).read_text())
            wanted = ['executor::' + module + '::' + name for name in expected]
            assert len(wanted) == count
            assert {name for name in names if name.startswith('executor::' + module + '::')} == set(wanted)
            result = run(command + ['executor::' + module + '::', '--', '--nocapture'])
            assert f'{count} passed; 0 failed' in result
            record['original_families'].append(dict(source='teaql-sql/src/' + source,
                sha256=digest(view.source / 'src' / source), tests=wanted, passed=count,
                cpp_port='tests/parity/teaql_sql/' + source.replace('.rs', '.cpp')))
        record['original_source_view'] = view.provenance()
        record['original_test_registry'] = names
        save()
    for profile in args.profiles:
        module = profile == 'modules_native'
        build = dict(native='build_native_current', modules_native='build_modules_native',
                     sanitize='build_sanitize_current')[profile]
        prefix = 'teaql_module_' if module else 'teaql_'
        targets = [prefix + family[2] for family in FAMILIES]
        run(['cmake', '--preset', profile])
        run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        names = [('module_' if module else '') + 'original_sql_' + name + '_execution_contracts'
                 for name in ('query', 'mutation')]
        junit = args.record.with_name(args.record.name + '.' + profile + '.xml')
        run(['ctest', '--preset', profile, '--no-tests=error', '--output-on-failure',
             '--output-junit', junit, '-R', '^(' + '|'.join(names) + ')$'])
        checks = ET.parse(junit).getroot().findall('.//testcase')
        assert len(checks) == 2 and {check.attrib['name'] for check in checks} == set(names)
        assert not any(check.find(tag) is not None for check in checks for tag in ('failure', 'error', 'skipped'))
        stable = run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        assert 'ninja: no work to do.' in stable
        binaries = {target: digest(ROOT / build / ('modules/' + target if module else target))
                    for target in targets}
        record['profiles'][profile] = dict(passed=2, junit=junit.read_text(), binary_sha256=binaries,
                                          no_work_rebuild=True,
                                          leak_scanning=False if profile == 'sanitize' else None)
        print(profile, '2/2 semantic ports; original 27/27', flush=True)
    record['cpp_inputs_sha256'] = {str(path.relative_to(ROOT)): digest(path) for path in (
        PARITY / 'teaql_sql/execution_fixture.hpp', PARITY / 'teaql_sql/execution_tests.cpp',
        PARITY / 'teaql_sql/mutation_execution_tests.cpp')}
    record.update(complete=True, original_tests_passed=27,
                  source_test_review='docs/sql_execution_tests_source_review_5_0_6.md',
                  coverage='All six query and twenty-one mutation original test bodies; production executor qualification remains independent.')
    save()


if __name__ == '__main__':
    main()
