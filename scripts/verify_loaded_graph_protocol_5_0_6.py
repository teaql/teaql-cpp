#!/usr/bin/env python3
"""Replay original graph reconstruction with exact locked runtime dependencies."""
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
    parser.add_argument('--cpp', type=Path)
    parser.add_argument('--module', type=Path)
    parser.add_argument('--write-corpus', action='store_true')
    parser.add_argument('--check-gates', action='store_true')
    parser.add_argument('--expect-differences', action='store_true')
    args = parser.parse_args()
    if args.record.exists() or args.record.suffix != '.gz':
        parser.error('Use a fresh .gz receipt; preserve earlier evidence')
    if args.check_gates and (args.cpp is None or args.module is None or args.expect_differences):
        parser.error('Installed gates require both matching frontends')
    root = Path(__file__).resolve().parents[1]
    sys.path[:0] = [str(root / 'scripts'), str(root / 'tests/parity')]
    from compare_values import observe, same
    from loaded_graph_protocol_inputs_5_0_6 import inputs
    from reference_runtime_view_5_0_6 import ReferenceRuntimeView
    from run_native_integration_epoch_5_0_6 import capture_inputs, references
    digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    driver = root / 'tests/parity/rust_private_save_5_0_6/src/bin/loaded_row_graph_5_0_6.rs'
    fixture_values = root / 'tests/parity/rust_values/src/fixture_value.rs'
    record = dict(complete=False, source_promoted=False, inputs=capture_inputs(), references=references(),
                  steps=[], profiles={}, driver_sha256=digest(driver), driver_text=driver.read_text(),
                  fixture_values_sha256=digest(fixture_values), controller_source=Path(__file__).read_text())
    environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off',
                       CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.parent.mkdir(parents=True, exist_ok=True)
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command):
        result = subprocess.run([str(x) for x in command], cwd=root, env=environment,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command], exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    try:
        run([sys.executable, root / 'scripts/verify_upstream.py'])
        rows = inputs()
        legacy_path = root / 'tests/parity/loaded_row_graph_5_0_6_cases.json.gz'
        legacy = json.loads(gzip.decompress(legacy_path.read_bytes()))
        assert legacy['baseline'] == record['references']['teaql-rs']
        assert len(legacy['cases']) == 2032
        assert same(rows[:2032], [case['input'] for case in legacy['cases']])
        record['retained_corpus_sha256'] = digest(legacy_path)
        with tempfile.TemporaryDirectory(prefix='teaql_loaded_graph_protocol_506_') as temporary:
            work = Path(temporary)
            input_path = work / 'inputs.json'
            input_path.write_text(json.dumps(rows) + '\n')
            observer = work / 'observer.rs'
            old_path = '#[path="../../../rust_values/src/fixture_value.rs"]'
            original = driver.read_text()
            assert original.count(old_path) == 1
            adapted = original.replace(old_path, '#[path=' + json.dumps(str(fixture_values)) + ']')
            # Preserve all fixture functions, including the unused original main.
            # Only path spelling, a crate alias and the appended test entry differ.
            adapted += '\nuse crate as teaql_runtime;\n'
            adapted += '''
#[test]
fn replay_retained_and_shared_identity_controls() {
    let inputs: Vec<J> = serde_json::from_str(include_str!(''' + json.dumps(str(input_path)) + ''')).unwrap();
    for input in inputs {
        println!("TEAQL_LOADED_GRAPH_JSON {}", evaluate(&input));
    }
}
'''
            observer.write_text(adapted)
            bridge = work / 'bridge.rs'
            bridge.write_text('''
#[cfg(test)]
impl<'a, E> EntityDataService<'a, E>
where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {
    pub(crate) fn __cpp_reconstruct_loaded_graph(&self, entity: &str, rows: &mut [CompactRow])
        -> Result<crate::EntityRuntimeState, teaql_core::EntityError> {
        self.attach_flat_relation_graph(entity, rows)
    }
}
#[cfg(test)]
#[allow(dead_code)]
#[path = ''' + json.dumps(str(observer)) + ''']
mod cpp_loaded_graph_observer;
''')
            record['adapted_fixture_text'] = adapted
            view = ReferenceRuntimeView(root, work / 'runtime', {'src/data_service/resolved.rs': bridge}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', view.directory / 'Cargo.toml',
                          '-j2', '--lib', 'data_service::resolved::', '--', '--nocapture', '--test-threads=1'])
            names = re.findall(r'^test (\S+) \.\.\.', output, flags=re.M)
            assert names == [
                'data_service::resolved::cpp_loaded_graph_observer::replay_retained_and_shared_identity_controls',
                'data_service::resolved::future_size_tests::ordinary_typed_query_future_has_bounded_heap_frame']
            assert '2 passed; 0 failed' in output
            expected = [json.loads(line.split('TEAQL_LOADED_GRAPH_JSON ', 1)[1])
                        for line in output.split('\n') if 'TEAQL_LOADED_GRAPH_JSON ' in line]
            assert len(expected) == len(rows) == 3568
            assert same(expected[:2032], [case['expected'] for case in legacy['cases']])
            record['original'] = dict(tests=names, observations=expected, runtime_view=view.provenance())
            path = root / 'tests/parity/loaded_graph_protocol_5_0_6_cases.json.gz'
            corpus = dict(baseline=record['references']['teaql-rs'], retained_cases=2032,
                          cases=[dict(input=i, expected=e) for i, e in zip(rows, expected)])
            if args.write_corpus:
                assert not path.exists()
                path.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
                record['generated_inputs'] = {'runtime:' + str(path.relative_to(root)): digest(path)}
            else:
                assert same(corpus, json.loads(gzip.decompress(path.read_bytes())))
            for profile, binary in [('native', args.cpp), ('modules_native', args.module)]:
                if binary is None:
                    continue
                gate = {}
                if args.check_gates:
                    imported = profile == 'modules_native'
                    prefix = 'teaql_module_' if imported else 'teaql_'
                    build = root / ('build_modules_native' if imported else 'build_native_current')
                    assert binary.resolve() == build / ('modules' if imported else '') / (prefix + 'loaded_row_graph_probe')
                    run(['cmake', '--preset', profile])
                    command = ['cmake', '--build', '--preset', profile, '--parallel', '2', '--target', prefix + 'loaded_row_graph_probe']
                    run(command)
                    if imported:
                        run([sys.executable, root / 'scripts/verify_module_consumers.py', '--build-dir', build])
                    tests = ['loaded_graph_protocol_rust_parity', 'loaded_row_graph_rust_parity', 'loaded_row_graph_context_contracts']
                    if imported:
                        tests = ['module_' + name for name in tests]
                    junit = work / (profile + '.xml')
                    run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
                         '--output-junit', junit, '-R', '^(' + '|'.join(tests) + ')$'])
                    tree = ET.parse(junit).getroot()
                    assert sorted(case.attrib['name'] for case in tree.findall('.//testcase')) == sorted(tests)
                    assert not any(tree.findall('.//' + tag) for tag in ['failure', 'error', 'skipped'])
                    assert 'ninja: no work to do.' in run(command)
                    gate = dict(tests=tests, junit=junit.read_text(), post_build_no_work=True)
                actual = observe(str(binary.resolve()), rows)
                differences = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
                record['profiles'][profile] = dict(observations=actual, differing_indices=differences,
                    binary_sha256=digest(binary), gate=gate)
                save()
                assert bool(differences) == args.expect_differences, (profile, len(differences))
            view.verify()
        assert capture_inputs() == record['inputs'] | record.get('generated_inputs', {})
        assert references() == record['references'] and digest(driver) == record['driver_sha256']
        assert digest(fixture_values) == record['fixture_values_sha256']
        record.update(complete=True, unchanged_inputs=True, original_tests=1, added_caller_tests=1,
                      all_installed_frontends=not args.expect_differences and set(record['profiles']) == {'native', 'modules_native'})
        save()
        print('Loaded graph: original1/caller1, retained2032 + shared-identity1536; '
              + ', '.join(f'{p}:{len(v["differing_indices"])} differences' for p, v in record['profiles'].items()), flush=True)
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
