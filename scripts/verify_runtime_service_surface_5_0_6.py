#!/usr/bin/env python3
"""Qualify the runtime service module, boundary enum and original future regression."""
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
    paths = ['teaql-runtime/src/data_service/mod.rs',
             'teaql-runtime/src/data_service/executor.rs',
             'teaql-runtime/src/data_service/query_future_tests.rs']
    digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    record = dict(complete=False, source_promoted=False, inputs=capture_inputs(),
                  references=references(), steps=[], profiles={},
                  source_sha256={p: digest(root / '.reference/teaql-rs' / p) for p in paths},
                  controller_source=Path(__file__).read_text(),
                  fixture=(root / 'tests/parity/runtime_query_future_probe.cpp').read_text(),
                  observer=(root / 'tests/parity/runtime_service_surface_source_5_0_6.rs').read_text())
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
        with tempfile.TemporaryDirectory(prefix='teaql_service_surface_506_') as temporary:
            work = Path(temporary)
            view = ReferenceRuntimeView(root, work / 'runtime', {
                'src/data_service/mod.rs': root / 'tests/parity/runtime_service_surface_source_5_0_6.rs'
            }, run)
            tests = ['data_service::cpp_service_surface_observer::boundaries_and_exports',
                     'data_service::resolved::future_size_tests::ordinary_typed_query_future_has_bounded_heap_frame']
            outputs = []
            for test in tests:
                output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                              view.directory / 'Cargo.toml', '-j2', '--lib', test,
                              '--', '--exact', '--nocapture', '--test-threads=1'])
                assert '1 passed; 0 failed' in output and test in output
                outputs.append(output)
            assert 'SERVICE_BOUNDARIES=Started,AlreadyActive,Unsupported' in outputs[0]
            frame = int(re.search(r'ORDINARY_QUERY_HEAP_FRAME_BYTES=(\d+)', outputs[1]).group(1))
            assert frame <= 8192
            record['original'] = dict(tests=tests, original_tests=1, added_tests=1,
                                      heap_frame_bytes=frame, provenance=view.provenance())
            for profile, imported in [('native', False), ('modules_native', True)]:
                build = root / ('build_modules_native' if imported else 'build_native_current')
                target = ('teaql_module_' if imported else 'teaql_') + 'runtime_query_future_probe'
                test = ('module_' if imported else '') + 'runtime_query_future_contracts'
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
                native_frame = int(re.search(r'ORDINARY_TYPED_QUERY_ARENA_FRAME_BYTES=(\d+)', output).group(1))
                assert native_frame <= 8192
                assert 'SERVICE_BOUNDARIES=Started,AlreadyActive,Unsupported' in output
                assert 'ninja: no work to do.' in run(command)
                record['profiles'][profile] = dict(test=test, arena_frame_bytes=native_frame,
                    binary_sha256=digest(binary), junit=junit.read_text(), no_work=True)
                save()
            view.verify()
        assert capture_inputs() == record['inputs'] and references() == record['references']
        record.update(complete=True, unchanged_inputs=True)
        save()
        print('Service surface and future contracts: original1/observer1, native1/module1')
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
