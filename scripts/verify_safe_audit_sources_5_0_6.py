#!/usr/bin/env python3
"""Qualify complete safe-target and bootstrap-audit source tests against the C++ runtime."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists() or args.record.suffix != '.gz':
        parser.error('Use a fresh .gz receipt')
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / 'scripts'))
    from reference_runtime_view_5_0_6 import ReferenceRuntimeView
    from run_native_integration_epoch_5_0_6 import capture_inputs, references
    paths = ['teaql-runtime/tests/safe_audit_target.rs',
             'teaql-runtime/tests/bootstrap_safe_audit.rs']
    digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    record = dict(complete=False, source_promoted=False, inputs=capture_inputs(),
                  references=references(), steps=[], profiles={},
                  source_sha256={p: digest(root / '.reference/teaql-rs' / p) for p in paths},
                  controller_source=Path(__file__).read_text(),
                  fixture=(root / 'teaql_runtime/tests/event_model_tests.cpp').read_text(),
                  scope='Complete safe-target four-test and bootstrap-attribution one-test integration sources')
    environment = dict(os.environ, CARGO_INCREMENTAL='0', TEAQL_LOG_ENDPOINT='off',
                       CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.parent.mkdir(parents=True, exist_ok=True)
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command):
        command = [str(x) for x in command]
        result = subprocess.run(command, cwd=root, env=environment, text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    try:
        run([sys.executable, root / 'scripts/verify_upstream.py'])
        with tempfile.TemporaryDirectory(prefix='teaql_safe_audit_sources_506_') as temporary:
            work = Path(temporary)
            view = ReferenceRuntimeView(root, work / 'runtime', {}, run)
            original = {}
            for source, count in [('safe_audit_target', 4), ('bootstrap_safe_audit', 1)]:
                output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                              view.directory / 'Cargo.toml', '-j2', '--test', source,
                              '--', '--show-output', '--test-threads=1'])
                names = re.findall(r'^test ([a-z0-9_]+) \.\.\. ok$', output, re.M)
                expected = re.findall(r'^#\[test\]\s*\nfn ([a-z0-9_]+)\(\)',
                                      (view.directory / 'tests' / (source + '.rs')).read_text(), re.M)
                assert len(expected) == count and sorted(names) == sorted(expected)
                assert f'{count} passed; 0 failed' in output
                original[source] = names
            record['original'] = dict(tests=original, original_tests=5, provenance=view.provenance())
            for profile, imported in [('native', False), ('modules_native', True)]:
                build = root / ('build_modules_native' if imported else 'build_native_current')
                target = ('teaql_module_' if imported else 'teaql_') + 'event_model_tests'
                test = ('module_' if imported else '') + 'event_model_contracts'
                run(['cmake', '--preset', profile])
                command = ['cmake', '--build', '--preset', profile, '--parallel', '4', '--target', target]
                run(command)
                if imported:
                    run([sys.executable, root / 'scripts/verify_module_consumers.py', '--build-dir', build])
                junit = work / (profile + '.xml')
                run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
                     '--output-junit', junit, '-R', '^' + test + '$'])
                tree = ET.parse(junit).getroot()
                assert [x.attrib['name'] for x in tree.findall('.//testcase')] == [test]
                assert not any(tree.findall('.//' + tag) for tag in ['failure', 'error', 'skipped'])
                binary = build / ('modules/' if imported else '') / target
                output = run([binary])
                assert 'SAFE_AUDIT_SOURCE_CONTRACTS=5' in output
                assert 'ninja: no work to do.' in run(command)
                record['profiles'][profile] = dict(test=test, scenarios=5,
                    binary_sha256=digest(binary), junit=junit.read_text(), no_work=True)
                save()
            view.verify()
        assert capture_inputs() == record['inputs'] and references() == record['references']
        record.update(complete=True, unchanged_inputs=True)
        save()
        print('Safe-audit sources: original4+1, native1/module1; all source helpers and assertions')
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
