#!/usr/bin/env python3
"""Replay typed dynamic hydration and explicit duplicate-result retention through generated Product."""
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
    paths = ['teaql-runtime/tests/dynamic_field_query.rs']
    digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    record = dict(complete=False, source_promoted=False, inputs=capture_inputs(),
                  references=references(), steps=[], profiles={},
                  source_sha256={p: digest(root / '.reference/teaql-rs' / p) for p in paths},
                  controller_source=Path(__file__).read_text(),
                  fixture=(root / 'teaql_runtime/tests/dynamic_relation_tests.cpp').read_text(),
                  scope='Focused typed dynamic batching and explicit Context retention contracts; whole dynamic_field_query.rs remains pending')
    environment = dict(os.environ, CARGO_INCREMENTAL='0', TEAQL_LOG_ENDPOINT='off',
                       ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1',
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
        run([sys.executable, root / 'scripts/generate_examples.py', '--check', '--fixtures', 'model'])
        record['generated_sdk_reproduced'] = True
        with tempfile.TemporaryDirectory(prefix='teaql_generated_typed_dynamic_506_') as temporary:
            work = Path(temporary)
            view = ReferenceRuntimeView(root, work / 'runtime', {}, run)
            tests = [
                'typed_query_batches_once_and_preserves_private_null_missing_and_ledgers',
                'basic_typed_query_duplicates_and_result_lifetime_keep_the_same_contract']
            source_text = (view.directory / 'tests/dynamic_field_query.rs').read_text()
            for test in tests:
                assert '#[tokio::test]\nasync fn ' + test + '(' in source_text
                output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                              view.directory / 'Cargo.toml', '-j2', '--test', 'dynamic_field_query',
                              test, '--', '--exact', '--show-output', '--test-threads=1'])
                assert '1 passed; 0 failed' in output and f'test {test} ... ok' in output
            record['original'] = dict(tests=tests, original_tests=2, provenance=view.provenance())
            for profile, imported in [('native', False), ('modules_native', True), ('sanitize', False)]:
                build = root / {'native': 'build_native_current', 'modules_native': 'build_modules_native',
                                'sanitize': 'build_sanitize_current'}[profile]
                target = ('teaql_module_' if imported else 'teaql_') + 'dynamic_relation_tests'
                test = ('module_' if imported else '') + 'dynamic_relation_contracts'
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
                assert 'GENERATED_READONLY_STREAM_CASES=3;LONG_ROWS=1003;LONG_CHUNKS=14' in output
                assert 'DYNAMIC_CHILD_SOURCE_CASES=7' in output
                assert 'GENERATED_DYNAMIC_LEDGER_CASES=2' in output
                assert 'GENERATED_ROOT_DYNAMIC_BOUNDARY_CASES=7' in output
                assert 'GENERATED_TYPED_DYNAMIC_BATCH_CASES=2' in output
                assert 'ninja: no work to do.' in run(command)
                record['profiles'][profile] = dict(test=test, scenarios=2,
                    binary_sha256=digest(binary), junit=junit.read_text(), no_work=True)
                save()
            view.verify()
        assert capture_inputs() == record['inputs'] and references() == record['references']
        record.update(complete=True, unchanged_inputs=True)
        save()
        print('Typed dynamic batching/retention: original2, generated cases2, native1/module1/sanitize1; whole source remains pending')
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
