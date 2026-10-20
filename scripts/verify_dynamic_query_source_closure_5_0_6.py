#!/usr/bin/env python3
"""Reconcile all 23 dynamic-query source tests after complete SDK adapter integration."""
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
                  fixture=(root / 'tests/generator/generated_graph_serialization_tests.cpp').read_text(),
                  scope='Complete original test execution plus generated graph serialization and retained caller regressions; whole-source review remains separate')
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
        integration = root / 'docs/parity/generated_embedded_graph_sdk_integration_5_0_6.json.gz'
        accepted = json.loads(gzip.decompress(integration.read_bytes()))
        assert accepted['complete'] and set(accepted['profiles']) == {'native', 'modules_native'}
        prior = accepted['inputs']
        current = record['inputs']
        annotations = {'runtime:docs/cpp_generator_parity_ledger.json',
                       'runtime:docs/parity/v5_0_6/source_dispositions.json'}
        record['prior_annotation_updates'] = {}
        for path, expected in prior.items():
            if path == 'runtime:teaql_runtime/tests/dynamic_relation_tests.cpp':
                continue
            if path in annotations:
                # Acceptance annotations evolve after a terminal gate. They are
                # still frozen by this invocation's complete input guard below.
                record['prior_annotation_updates'][path] = dict(before=expected, current=current[path])
                continue
            assert current.get(path) == expected, ('SDK integration input changed', path)
        record['sdk_integration'] = dict(path=str(integration.relative_to(root)), sha256=digest(integration))
        record['generated_sdk_reproduced'] = True
        java_report = root.parent / 'teaql_code_gen_cpp/generator/target/surefire-reports/TEST-com.skynet.codegenerator.CppGenerationTest.xml'
        java = ET.parse(java_report).getroot()
        assert int(java.attrib['tests']) > 0 and all(int(java.attrib.get(k, 0)) == 0 for k in ['failures', 'errors', 'skipped'])
        record['java'] = dict(attributes=java.attrib, report=java_report.read_text())
        with tempfile.TemporaryDirectory(prefix='teaql_dynamic_query_source_closure_506_') as temporary:
            work = Path(temporary)
            view = ReferenceRuntimeView(root, work / 'runtime', {}, run)
            source_text = (view.directory / 'tests/dynamic_field_query.rs').read_text()
            tests = re.findall(r'#\[(?:tokio::)?test\]\s*(?:async\s+)?fn\s+(\w+)', source_text)
            assert len(tests) == 23
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                          view.directory / 'Cargo.toml', '-j2', '--test', 'dynamic_field_query',
                          '--', '--show-output', '--test-threads=1'])
            assert '23 passed; 0 failed' in output
            assert all(f'test {test} ... ok' in output for test in tests)
            record['original'] = dict(tests=tests, original_tests=23, provenance=view.provenance())
            stems = ['generated_graph_serialization', 'dynamic_relation', 'generated_readonly_property']
            for profile, imported in [('native', False), ('modules_native', True)]:
                build = root / {'native': 'build_native_current', 'modules_native': 'build_modules_native',
                                'sanitize': 'build_sanitize_current'}[profile]
                prefix = 'teaql_module_' if imported else 'teaql_'
                targets = [prefix + stem + '_tests' for stem in stems]
                names = [('module_' if imported else '') + stem + '_contracts' for stem in stems]
                run(['cmake', '--preset', profile])
                command = ['cmake', '--build', '--preset', profile, '--parallel', '4', '--target', *targets]
                run(command)
                if imported:
                    run([sys.executable, root / 'scripts/verify_module_consumers.py', '--build-dir', build])
                junit = work / (profile + '.xml')
                run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
                     '--output-junit', junit, '-R', '^(' + '|'.join(names) + ')$'])
                tree = ET.parse(junit).getroot()
                assert sorted(x.attrib['name'] for x in tree.findall('.//testcase')) == sorted(names)
                assert not any(tree.findall('.//' + tag) for tag in ['failure', 'error', 'skipped'])
                binaries = {}
                for stem, target in zip(stems, targets):
                    binary = build / ('modules/' if imported else '') / target
                    output = run([binary])
                    if stem == 'generated_graph_serialization':
                        assert 'RUNTIME_EMBEDDED_RELATIONS=passed' in output
                        assert 'GENERATED_BORROWED_GRAPH_CASES=3;NESTED_RELATION_CASES=2' in output
                        assert 'GENERATED_GRAPH_COPY_CONTROL=passed;BORROWED_COPIES=0' in output
                    elif stem == 'dynamic_relation':
                        for marker in ['GENERATED_READONLY_STREAM_CASES=3;LONG_ROWS=1003;LONG_CHUNKS=14',
                                       'DYNAMIC_CHILD_SOURCE_CASES=7', 'GENERATED_DYNAMIC_LEDGER_CASES=2',
                                       'GENERATED_ROOT_DYNAMIC_BOUNDARY_CASES=7', 'GENERATED_TYPED_DYNAMIC_BATCH_CASES=2',
                                       'GRAPH_READ_METADATA_CASES=1;PLAIN_CARRIER_DEFAULT_CASES=1']:
                            assert marker in output
                    else:
                        for marker in ['GENERATED_READONLY_ALLOCATION_CASES=2;READ_COUNTS=1,100,10000;POSITIVE_CONTROLS=passed',
                                       'JSON_RUNTIME_STATE_CONTRACTS=2;REJECTIONS=14', 'GENERATED_ENTITY_SERIALIZATION_CASES=2']:
                            assert marker in output
                        for family in ['readonly_schema', 'readonly_values']:
                            for reads in [1, 100, 10000]:
                                assert f'{family},{reads},0,0,0,0' in output.splitlines()
                    binaries[target] = digest(binary)
                assert 'ninja: no work to do.' in run(command)
                record['profiles'][profile] = dict(tests=names, graph_cases=5,
                    binaries=binaries, junit=junit.read_text(), no_work=True)
                save()
            view.verify()
        assert capture_inputs() == record['inputs'] and references() == record['references']
        record.update(complete=True, unchanged_inputs=True)
        save()
        print('Complete original23 and all focused counterparts; native3/module3; exact native projection recorded; source review remains separate')
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
