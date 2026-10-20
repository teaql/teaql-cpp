#!/usr/bin/env python3
"""Replay the six original flat-identity tests and their handwritten C++ counterparts."""
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
    parser.add_argument('--generated', action='store_true',
                        help='Also reproduce the SDK and verify real generated hydration')
    args = parser.parse_args()
    if args.record.exists() or args.record.suffix != '.gz':
        parser.error('Use a fresh .gz receipt')
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / 'scripts'))
    from reference_runtime_view_5_0_6 import ReferenceRuntimeView
    from run_native_integration_epoch_5_0_6 import capture_inputs, references
    paths = ['teaql-runtime/tests/flat_identity_graph.rs']
    digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    record = dict(complete=False, source_promoted=False, inputs=capture_inputs(),
                  references=references(), steps=[], profiles={},
                  source_sha256={p: digest(root / '.reference/teaql-rs' / p) for p in paths},
                  controller_source=Path(__file__).read_text(),
                  fixture=(root / 'tests/parity/teaql_runtime/flat_identity_graph.cpp').read_text(),
                  scope='Six source scenarios; handwritten model adapter is not generated macro acceptance')
    if args.generated:
        record['generated_fixture'] = (root / 'tests/generator/ledger_composition_tests.cpp').read_text()
        record['scope'] = 'Six original scenarios plus real generated Owner/Platform hydration isolation'
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
        if args.generated:
            for repository in [root.parent / 'teaql_code_gen_cpp', root / '.reference/teaql_code_gen']:
                run(['mvn', '-q', '-o', '-f', repository / 'pom.xml', '-pl', 'generator',
                     '-am', '-DskipTests', 'package'])
            run([sys.executable, root / 'scripts/generate_relation_identity_contracts_5_0_6.py', '--check'])
            record['generated_sdk_reproduced'] = True
        with tempfile.TemporaryDirectory(prefix='teaql_flat_identity_506_') as temporary:
            work = Path(temporary)
            view = ReferenceRuntimeView(root, work / 'runtime', {}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                          view.directory / 'Cargo.toml', '-j2', '--test', 'flat_identity_graph',
                          '--', '--nocapture', '--test-threads=1'])
            names = re.findall(r'^test ([a-z0-9_]+) \.\.\. ok$', output, re.M)
            expected = re.findall(r'^fn ([a-z0-9_]+)\(\)',
                                  (view.directory / 'tests/flat_identity_graph.rs').read_text(), re.M)
            assert len(expected) == 6 and sorted(names) == sorted(expected)
            assert '6 passed; 0 failed' in output
            record['original'] = dict(tests=names, original_tests=6, provenance=view.provenance())
            for profile, imported in [('native', False), ('modules_native', True)]:
                build = root / ('build_modules_native' if imported else 'build_native_current')
                target = ('teaql_module_' if imported else 'teaql_') + 'original_flat_graph_tests'
                test = ('module_' if imported else '') + 'original_runtime_flat_identity_graph_contracts'
                targets = [target]
                tests = [test]
                if args.generated:
                    targets.append(('teaql_module_' if imported else 'teaql_') + 'ledger_composition_tests')
                    tests.append(('module_' if imported else '') + 'generated_ledger_composition_contracts')
                run(['cmake', '--preset', profile])
                command = ['cmake', '--build', '--preset', profile, '--parallel', '4', '--target', *targets]
                run(command)
                if imported:
                    run([sys.executable, root / 'scripts/verify_module_consumers.py', '--build-dir', build])
                junit = work / (profile + '.xml')
                run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
                     '--output-junit', junit, '-R', '^(' + '|'.join(tests) + ')$'])
                tree = ET.parse(junit).getroot()
                assert sorted(x.attrib['name'] for x in tree.findall('.//testcase')) == sorted(tests)
                assert not any(tree.findall('.//' + tag) for tag in ['failure', 'error', 'skipped'])
                binary = build / ('modules/' if imported else '') / target
                output = run([binary])
                assert 'FLAT_IDENTITY_CONTRACTS=6' in output
                generated_binary = None
                if args.generated:
                    generated_binary = build / ('modules/' if imported else '') / targets[1]
                    assert 'GENERATED_FLAT_IDENTITY_ISOLATION=passed' in run([generated_binary])
                assert 'ninja: no work to do.' in run(command)
                record['profiles'][profile] = dict(tests=tests, scenarios=6,
                    generated_binary_sha256=digest(generated_binary) if generated_binary else None,
                    binary_sha256=digest(binary), junit=junit.read_text(), no_work=True)
                save()
            view.verify()
        assert capture_inputs() == record['inputs'] and references() == record['references']
        record.update(complete=True, unchanged_inputs=True)
        save()
        print('Flat identity contracts: original6; native/modules pass; generated SDK:', args.generated)
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
